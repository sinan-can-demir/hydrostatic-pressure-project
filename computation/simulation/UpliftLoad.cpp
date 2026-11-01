#include "UpliftLoad.hpp"

namespace simulation {

namespace {

// One straight stretch of the head diagram: heads h0 at x0 and h1 at x1 (x from the heel).
struct Stretch {
    double x0, h0, x1, h1;
};

constexpr double kNearHeelFraction = 0.05; // Fig. 3-3: drains within 0.05 H1 of the upstream face

} // namespace

std::vector<ForceContribution> UpliftLoad::contributions(const model::Scenario& scenario) const {
    if (!scenario.uplift) return {};

    const double base = scenario.section.base_length().meters;
    const double heel_x = scenario.section.heel().x.meters;
    const double h1 = scenario.water_level.elevation.meters;
    const double h2 = scenario.tailwater_depth ? scenario.tailwater_depth->meters : 0.0;
    const double k = scenario.fluid.density.kg_per_m3 * scenario.fluid.gravity.m_per_s2;

    std::vector<Stretch> stretches;
    if (!scenario.uplift->drains) {
        stretches.push_back({0.0, h1, base, h2});
    } else {
        const model::Drains& d = *scenario.uplift->drains;
        const double x = d.distance_from_heel.meters;
        const double h_floor = d.gallery_elevation.meters > h2 ? d.gallery_elevation.meters : h2;
        const double residual = 1.0 - d.effectiveness;
        if (x <= kNearHeelFraction * h1) {
            stretches.push_back({0.0, residual * (h1 - h_floor) + h_floor, base, h2});
        } else {
            const double h3 = residual * (h1 - h_floor) * (base - x) / base + h_floor;
            stretches.push_back({0.0, h1, x, h3});
            stretches.push_back({x, h3, base, h2});
        }
    }

    // Area (head * length) and its first moment about the heel, summed over the trapezoids.
    double area = 0.0, first_moment = 0.0;
    for (const Stretch& s : stretches) {
        const double len = s.x1 - s.x0;
        if (len <= 0.0) continue;
        const double a = 0.5 * (s.h0 + s.h1) * len;
        // Centroid of a trapezoid with parallel sides h0 (at x0) and h1 (at x1).
        const double sum = s.h0 + s.h1;
        const double centroid = sum > 0.0 ? s.x0 + len * (s.h0 + 2.0 * s.h1) / (3.0 * sum) : s.x0;
        area += a;
        first_moment += a * centroid;
    }
    if (area <= 0.0) return {};

    const math::Point2 point{units::Length(heel_x + first_moment / area), units::Length(0.0)};
    return {ForceContribution{units::ForcePerWidth(0.0), units::ForcePerWidth(k * area), point, "uplift"}};
}

} // namespace simulation
