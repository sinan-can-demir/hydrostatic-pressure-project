/**
 * @file Concrete.hpp
 * @brief Dam concrete; a single uniform unit weight (default 23.5 kN/m3).
 */

#pragma once

#include "units/UnitWeight.hpp"

namespace materials {

struct Concrete {
    bool operator==(const Concrete&) const = default;
    units::UnitWeight unit_weight = units::UnitWeight(23500.0);
};

} // namespace materials
