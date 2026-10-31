/**
 * @file Angle.hpp
 * @brief Plane angle, stored in radians.
 */

#pragma once

#include <cmath>
#include <numbers>

namespace units {

struct Angle {
    double radians;

    constexpr explicit Angle(double r) : radians(r) {}

    static constexpr Angle from_degrees(double d) { return Angle(d * std::numbers::pi / 180.0); }
    constexpr double degrees() const { return radians * 180.0 / std::numbers::pi; }
};

inline double tan(Angle a) { return std::tan(a.radians); }

} // namespace units
