/**
 * @file ForcePerWidth.hpp
 * @brief Force per unit width of a 2D section, in N/m.
 */

#pragma once

#include "Length.hpp"
#include "MomentPerWidth.hpp"

namespace units {

struct ForcePerWidth {
    bool operator==(const ForcePerWidth&) const = default;
    double n_per_m;

    constexpr explicit ForcePerWidth(double f) : n_per_m(f) {}
};

constexpr ForcePerWidth operator+(ForcePerWidth a, ForcePerWidth b) {
    return ForcePerWidth(a.n_per_m + b.n_per_m);
}
constexpr ForcePerWidth operator-(ForcePerWidth a, ForcePerWidth b) {
    return ForcePerWidth(a.n_per_m - b.n_per_m);
}
constexpr ForcePerWidth operator-(ForcePerWidth a) { return ForcePerWidth(-a.n_per_m); }
constexpr ForcePerWidth operator*(ForcePerWidth a, double s) { return ForcePerWidth(a.n_per_m * s); }
constexpr ForcePerWidth operator*(double s, ForcePerWidth a) { return ForcePerWidth(s * a.n_per_m); }

// Force times lever arm. The two types meet here so the headers form a DAG.
constexpr MomentPerWidth operator*(ForcePerWidth f, Length arm) {
    return MomentPerWidth(f.n_per_m * arm.meters);
}
constexpr MomentPerWidth operator*(Length arm, ForcePerWidth f) { return f * arm; }

// Moment divided by force gives the lever arm (e.g. resultant location, x_R = M / V).
constexpr Length operator/(MomentPerWidth m, ForcePerWidth f) { return Length(m.n_m_per_m / f.n_per_m); }

} // namespace units
