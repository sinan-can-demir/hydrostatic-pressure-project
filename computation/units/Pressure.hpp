/**
 * @file Pressure.hpp
 * @author Sinan Demir
 * @brief 
 * @version 0.1
 * @date 2026-01-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "Density.hpp"
#include "Acceleration.hpp"
#include "Length.hpp"

namespace units {

struct Pressure {
    double pascals;

    constexpr explicit Pressure(double p) : pascals(p) {}
};

// --------------------
// Allowed operations
// --------------------

constexpr Pressure operator+(Pressure a, Pressure b) {
    return Pressure(a.pascals + b.pascals);
}

constexpr Pressure operator-(Pressure a, Pressure b) {
    return Pressure(a.pascals - b.pascals);
}

constexpr Pressure operator*(Pressure p, double s) {
    return Pressure(p.pascals * s);
}

constexpr Pressure operator*(double s, Pressure p) {
    return Pressure(p.pascals * s);
}

// --------------------
// Physics-specific ops
// --------------------

constexpr Pressure hydrostatic(
    Density rho,
    Acceleration g,
    Length h
) {
    return Pressure(rho.kg_per_m3 * g.m_per_s2 * h.meters);
}

// --------------------
// Explicitly forbidden
// --------------------

Pressure operator*(Pressure, Length) = delete;
Pressure operator/(Pressure, Length) = delete;

} // namespace units