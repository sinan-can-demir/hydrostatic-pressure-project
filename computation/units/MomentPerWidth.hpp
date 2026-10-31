/**
 * @file MomentPerWidth.hpp
 * @brief Moment per unit width of a 2D section, in N*m/m (dimensionally newtons).
 */

#pragma once

namespace units {

struct MomentPerWidth {
    bool operator==(const MomentPerWidth&) const = default;
    double n_m_per_m;

    constexpr explicit MomentPerWidth(double m) : n_m_per_m(m) {}
};

constexpr MomentPerWidth operator+(MomentPerWidth a, MomentPerWidth b) {
    return MomentPerWidth(a.n_m_per_m + b.n_m_per_m);
}
constexpr MomentPerWidth operator-(MomentPerWidth a, MomentPerWidth b) {
    return MomentPerWidth(a.n_m_per_m - b.n_m_per_m);
}
constexpr MomentPerWidth operator-(MomentPerWidth a) { return MomentPerWidth(-a.n_m_per_m); }
constexpr MomentPerWidth operator*(MomentPerWidth a, double s) { return MomentPerWidth(a.n_m_per_m * s); }
constexpr MomentPerWidth operator*(double s, MomentPerWidth a) { return MomentPerWidth(s * a.n_m_per_m); }

} // namespace units
