#include "Resultant.hpp"

#include <cmath>

namespace simulation {

namespace {

// Moment about the toe of a horizontal force at height y: fx pushes downstream, so a positive
// fx above the base turns the dam clockwise (negative).
double horizontal_moment(double fx, double y) { return -fx * y; }

// Moment of a vertical force at distance dx downstream of the toe.
double vertical_moment(double fy, double dx) { return fy * dx; }

MomentRole role_of(double moment) {
    if (moment > 0.0) return MomentRole::Resisting;
    if (moment < 0.0) return MomentRole::Overturning;
    return MomentRole::Neutral;
}

} // namespace

Resultant make_resultant(const std::vector<ForceContribution>& contributions,
                         const model::DamSection& section) {
    const double toe_x = section.toe().x.meters;
    const double base = section.base_length().meters;

    double sum_fx = 0.0, sum_fy = 0.0, moment = 0.0;
    for (const ForceContribution& c : contributions) {
        sum_fx += c.fx.n_per_m;
        sum_fy += c.fy.n_per_m;
        moment += horizontal_moment(c.fx.n_per_m, c.point.y.meters);
        moment += vertical_moment(c.fy.n_per_m, c.point.x.meters - toe_x);
    }

    const double v = -sum_fy;
    Resultant r{units::ForcePerWidth(v),
                units::ForcePerWidth(sum_fx),
                units::MomentPerWidth(moment),
                std::nullopt,
                std::nullopt,
                units::Length(base)};
    if (v > 0.0) {
        const double x = moment / v;
        r.x_from_toe = units::Length(x);
        r.eccentricity = units::Length(base / 2.0 - x);
    }
    return r;
}

Contact make_contact(const Resultant& resultant) {
    const Contact none{ContactState::None, units::Length(0.0), units::Pressure(0.0), units::Pressure(0.0)};
    if (!resultant.x_from_toe) return none;

    const double base = resultant.base_length.meters;
    const double x = resultant.x_from_toe->meters;
    const double v = resultant.vertical.n_per_m;
    if (x <= 0.0 || x >= base) return none;

    if (x >= base / 3.0 && x <= 2.0 * base / 3.0) {
        const double e = resultant.eccentricity->meters;
        const double mean = v / base;
        return {ContactState::Full, resultant.base_length, units::Pressure(mean * (1.0 + 6.0 * e / base)),
                units::Pressure(mean * (1.0 - 6.0 * e / base))};
    }
    if (x < base / 3.0) {
        const double a = 3.0 * x;
        return {ContactState::Partial, units::Length(a), units::Pressure(2.0 * v / a), units::Pressure(0.0)};
    }
    const double a = 3.0 * (base - x);
    return {ContactState::Partial, units::Length(a), units::Pressure(0.0), units::Pressure(2.0 * v / a)};
}

SlidingResult make_sliding(const Resultant& resultant, units::Angle friction_angle) {
    const double h = resultant.horizontal.n_per_m;
    if (h <= 0.0) return SlidingResult{std::nullopt};
    return SlidingResult{resultant.vertical.n_per_m * std::tan(friction_angle.radians) / h};
}

OverturningReport make_overturning(const std::vector<ForceContribution>& contributions,
                                   const model::DamSection& section) {
    const double toe_x = section.toe().x.meters;

    OverturningReport report{{}, units::MomentPerWidth(0.0), units::MomentPerWidth(0.0), std::nullopt};
    double resisting = 0.0, overturning = 0.0;
    auto add = [&](const ForceContribution& c, ForceDirection direction, double force, double arm,
                   double moment) {
        report.terms.push_back({c.source, direction, units::ForcePerWidth(force), units::Length(arm),
                                units::MomentPerWidth(moment), role_of(moment)});
        if (moment > 0.0) resisting += moment;
        if (moment < 0.0) overturning -= moment;
    };
    for (const ForceContribution& c : contributions) {
        if (c.fx.n_per_m != 0.0) {
            const double y = c.point.y.meters;
            add(c, ForceDirection::Horizontal, c.fx.n_per_m, std::abs(y), horizontal_moment(c.fx.n_per_m, y));
        }
        if (c.fy.n_per_m != 0.0) {
            const double dx = c.point.x.meters - toe_x;
            add(c, ForceDirection::Vertical, c.fy.n_per_m, std::abs(dx), vertical_moment(c.fy.n_per_m, dx));
        }
    }
    report.resisting = units::MomentPerWidth(resisting);
    report.overturning = units::MomentPerWidth(overturning);
    if (overturning > 0.0) report.ratio = resisting / overturning;
    return report;
}

} // namespace simulation
