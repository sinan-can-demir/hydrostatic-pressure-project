/**
 * @file ForceContribution.hpp
 * @brief One force acting on the dam, per unit width: components, application point, source.
 *
 * Components follow the section axes (x downstream, y up), so water pushes with a positive fx
 * and weight is a negative fy. The point only fixes the line of action of each component.
 */

#pragma once

#include <string>

#include "math/Point2.hpp"
#include "units/ForcePerWidth.hpp"

namespace simulation {

struct ForceContribution {
    bool operator==(const ForceContribution&) const = default;
    units::ForcePerWidth fx;
    units::ForcePerWidth fy;
    math::Point2 point;
    std::string source;
};

} // namespace simulation
