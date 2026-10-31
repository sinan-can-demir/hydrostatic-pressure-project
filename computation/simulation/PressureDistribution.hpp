/**
 * @file PressureDistribution.hpp
 * @brief Hydrostatic pressure sampled along the wetted upstream face (FR-05, FR-20).
 */

#pragma once

#include <vector>

#include "math/Point2.hpp"
#include "math/Vector2.hpp"
#include "units/Pressure.hpp"

namespace simulation {

struct PressureSample {
    bool operator==(const PressureSample&) const = default;
    math::Point2 position;
    units::Pressure pressure;
    // Unit vector normal to the face, pointing into the dam (the direction the water pushes).
    math::Vector2 normal;
};

// Ordered from the heel upward; empty when the face is dry.
struct PressureDistribution {
    bool operator==(const PressureDistribution&) const = default;
    std::vector<PressureSample> samples;
};

} // namespace simulation
