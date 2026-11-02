#include "UpliftLoad.hpp"

namespace simulation {

namespace {

// Break points of the head diagram: x from the heel (m) and head (m of water).
struct HeadPoint {
    double x, head;
};

constexpr double kNearHeelFraction = 0.05; // Fig. 3-3: drains within 0.05 H1 of the upstream face

std::vector<HeadPoint> head_points(const model::Scenario& scenario, const model::Uplift& uplift) {
    const double base = scenario.section.base_length().meters;
    const double h1 = scenario.water_level.elevation.meters;
    const double h2 = scenario.tailwater_depth ? scenario.tailwater_depth->meters : 0.0;

    if (!uplift.drains) return {{0.0, h1}, {base, h2}};

    const model::Drains& d = *uplift.drains;
    const double x = d.distance_from_heel.meters;
    const double h_floor = d.gallery_elevation.meters > h2 ? d.gallery_elevation.meters : h2;
    const double residual = 1.0 - d.effectiveness;
    if (x <= kNearHeelFraction * h1) return {{0.0, residual * (h1 - h_floor) + h_floor}, {base, h2}};
    const double h3 = residual * (h1 - h_floor) * (base - x) / base + h_floor;
    return {{0.0, h1}, {x, h3}, {base, h2}};
}

} // namespace

std::vector<UpliftSample> uplift_diagram(const model::Scenario& scenario) {
    if (!scenario.uplift) return {};
    const double heel_x = scenario.section.heel().x.meters;
    const double k = scenario.fluid.density.kg_per_m3 * scenario.fluid.gravity.m_per_s2;
    std::vector<UpliftSample> samples;
    for (const HeadPoint& p : head_points(scenario, *scenario.uplift)) {
        samples.push_back({units::Length(heel_x + p.x), units::Pressure(k * p.head)});
    }
    return samples;
}

std::vector<ForceContribution> UpliftLoad::contributions(const model::Scenario& scenario) const {
    if (!scenario.uplift) return {};

    const double heel_x = scenario.section.heel().x.meters;
    const double k = scenario.fluid.density.kg_per_m3 * scenario.fluid.gravity.m_per_s2;
    const std::vector<HeadPoint> points = head_points(scenario, *scenario.uplift);

    // Area (head * length) and its first moment about the heel, summed over the trapezoids.
    double area = 0.0, first_moment = 0.0;
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        const double x0 = points[i].x, h0 = points[i].head;
        const double len = points[i + 1].x - x0, h1 = points[i + 1].head;
        if (len <= 0.0) continue;
        const double a = 0.5 * (h0 + h1) * len;
        // Centroid of a trapezoid with parallel sides h0 (at x0) and h1 (at x0 + len).
        const double sum = h0 + h1;
        const double centroid = sum > 0.0 ? x0 + len * (h0 + 2.0 * h1) / (3.0 * sum) : x0;
        area += a;
        first_moment += a * centroid;
    }
    if (area <= 0.0) return {};

    const math::Point2 point{units::Length(heel_x + first_moment / area), units::Length(0.0)};
    return {ForceContribution{units::ForcePerWidth(0.0), units::ForcePerWidth(k * area), point, "uplift"}};
}

} // namespace simulation
