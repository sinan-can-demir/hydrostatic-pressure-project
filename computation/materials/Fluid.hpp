/**
 * @file Fluid.hpp
 * @brief Retained fluid; defaults to fresh water under standard gravity.
 */

#pragma once

#include "units/Acceleration.hpp"
#include "units/Density.hpp"

namespace materials {

struct Fluid {
    units::Density density = units::Density(1000.0);
    units::Acceleration gravity = units::Acceleration(9.81);
};

} // namespace materials
