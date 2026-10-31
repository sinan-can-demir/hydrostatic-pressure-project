/**
 * @file UnitWeight.hpp
 * @brief Weight per unit volume (specific weight), in N/m3.
 */

#pragma once

#include "Area.hpp"
#include "ForcePerWidth.hpp"

namespace units {

struct UnitWeight {
    double n_per_m3;

    constexpr explicit UnitWeight(double w) : n_per_m3(w) {}
};

// Weight of a 2D section per unit width: unit weight times cross-section area.
constexpr ForcePerWidth operator*(UnitWeight w, Area a) {
    return ForcePerWidth(w.n_per_m3 * a.square_meters);
}
constexpr ForcePerWidth operator*(Area a, UnitWeight w) { return w * a; }

} // namespace units
