#include "WaterLoad.hpp"

#include <algorithm>
#include <cmath>

#include "FaceLoad.hpp"

namespace simulation {

namespace {

using detail::Pt;

// The part of the upstream face below the water level, heel first.
std::vector<Pt> wetted_face(const model::Scenario& scenario) {
    return detail::wetted_chain(scenario.section.upstream_face(), scenario.water_level.elevation.meters);
}

} // namespace

std::vector<ForceContribution> WaterLoad::contributions(const model::Scenario& scenario) const {
    const double hw = scenario.water_level.elevation.meters;
    const double k = scenario.fluid.density.kg_per_m3 * scenario.fluid.gravity.m_per_s2;
    return detail::face_contributions(wetted_face(scenario), hw, k, detail::Side::Upstream,
                                      "water (horizontal)", "water (vertical)");
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
        return math::Vector2{units::Length((b.y - a.y) / len), units::Length((a.x - b.x) / len)};
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
