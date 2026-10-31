#include "WaterLoad.hpp"

#include <algorithm>
#include <cmath>

namespace simulation {

namespace {

struct Pt {
    double x;
    double y;
};

// The part of the upstream face below the water level, heel first. Face elevation never
// decreases (FR-02), so the wetted part is a prefix of the face plus one clipped point.
std::vector<Pt> wetted_face(const model::Scenario& scenario) {
    const double hw = scenario.water_level.elevation.meters;
    const std::vector<math::Point2> face = scenario.section.upstream_face();

    std::vector<Pt> chain;
    for (std::size_t i = 0; i + 1 < face.size(); ++i) {
        const Pt a{face[i].x.meters, face[i].y.meters};
        const Pt b{face[i + 1].x.meters, face[i + 1].y.meters};
        if (a.y >= hw) break;
        if (chain.empty()) chain.push_back(a);
        if (b.y <= hw) {
            chain.push_back(b);
        } else {
            const double t = (hw - a.y) / (b.y - a.y);
            chain.push_back({a.x + t * (b.x - a.x), hw});
            break;
        }
    }
    return chain;
}

// Integral over t in [0,1] of q(t) * f(t), where q is linear from q0 to q1 and f is linear
// from f0 to f1: exact for the product of two linear functions.
double linear_product(double q0, double q1, double f0, double f1) {
    const double dq = q1 - q0;
    const double df = f1 - f0;
    return q0 * f0 + (q0 * df + dq * f0) / 2.0 + dq * df / 3.0;
}

} // namespace

std::vector<ForceContribution> WaterLoad::contributions(const model::Scenario& scenario) const {
    const double hw = scenario.water_level.elevation.meters;
    const double k = scenario.fluid.density.kg_per_m3 * scenario.fluid.gravity.m_per_s2;
    const std::vector<Pt> chain = wetted_face(scenario);

    // Per segment, with q the linear pressure and the force element q * (dy, -dx):
    // sums of q dy and q dx (the forces) and their first moments about x and y.
    double sum_qdy = 0.0, sum_qx_dy = 0.0, sum_qy_dy = 0.0;
    double sum_qdx = 0.0, sum_qx_dx = 0.0, sum_qy_dx = 0.0;
    for (std::size_t i = 0; i + 1 < chain.size(); ++i) {
        const Pt a = chain[i];
        const Pt b = chain[i + 1];
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double q0 = k * (hw - a.y);
        const double q1 = k * (hw - b.y);
        sum_qdy += dy * linear_product(q0, q1, 1.0, 1.0);
        sum_qx_dy += dy * linear_product(q0, q1, a.x, b.x);
        sum_qy_dy += dy * linear_product(q0, q1, a.y, b.y);
        sum_qdx += dx * linear_product(q0, q1, 1.0, 1.0);
        sum_qx_dx += dx * linear_product(q0, q1, a.x, b.x);
        sum_qy_dx += dx * linear_product(q0, q1, a.y, b.y);
    }

    std::vector<ForceContribution> result;
    if (sum_qdy != 0.0) {
        result.push_back(ForceContribution{
            units::ForcePerWidth(sum_qdy), units::ForcePerWidth(0.0),
            math::Point2{units::Length(sum_qx_dy / sum_qdy), units::Length(sum_qy_dy / sum_qdy)},
            "water (horizontal)"});
    }
    if (sum_qdx != 0.0) {
        result.push_back(ForceContribution{
            units::ForcePerWidth(0.0), units::ForcePerWidth(-sum_qdx),
            math::Point2{units::Length(sum_qx_dx / sum_qdx), units::Length(sum_qy_dx / sum_qdx)},
            "water (vertical)"});
    }
    return result;
}

PressureDistribution WaterLoad::pressure(const model::Scenario& scenario,
                                         std::optional<std::size_t> sample_count) const {
    const double hw = scenario.water_level.elevation.meters;
    const double k = scenario.fluid.density.kg_per_m3 * scenario.fluid.gravity.m_per_s2;
    const std::vector<Pt> chain = wetted_face(scenario);
    PressureDistribution out;
    if (chain.size() < 2) return out;

    // Unit normal into the dam for the segment starting at chain[i] (the last one is reused
    // for the final point).
    auto normal_of = [&](std::size_t segment) {
        const Pt a = chain[segment];
        const Pt b = chain[segment + 1];
        const double len = std::hypot(b.x - a.x, b.y - a.y);
        return math::Vector2{units::Length((b.y - a.y) / len), units::Length(-(b.x - a.x) / len)};
    };
    auto sample_at = [&](Pt p, std::size_t segment) {
        return PressureSample{math::Point2{units::Length(p.x), units::Length(p.y)},
                              units::Pressure(k * (hw - p.y)), normal_of(segment)};
    };

    const std::size_t segments = chain.size() - 1;
    if (!sample_count) {
        for (std::size_t i = 0; i < chain.size(); ++i) {
            out.samples.push_back(sample_at(chain[i], std::min(i, segments - 1)));
        }
        return out;
    }

    // Evenly spaced by arc length along the wetted face.
    const std::size_t n = std::max<std::size_t>(*sample_count, 2);
    std::vector<double> cumulative{0.0};
    for (std::size_t i = 0; i < segments; ++i) {
        cumulative.push_back(cumulative.back() +
                             std::hypot(chain[i + 1].x - chain[i].x, chain[i + 1].y - chain[i].y));
    }
    const double total = cumulative.back();
    std::size_t seg = 0;
    for (std::size_t j = 0; j < n; ++j) {
        const double s = total * static_cast<double>(j) / static_cast<double>(n - 1);
        while (seg + 1 < segments && s >= cumulative[seg + 1]) ++seg;
        const double t = (s - cumulative[seg]) / (cumulative[seg + 1] - cumulative[seg]);
        const Pt a = chain[seg];
        const Pt b = chain[seg + 1];
        out.samples.push_back(sample_at({a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)}, seg));
    }
    return out;
}

} // namespace simulation
