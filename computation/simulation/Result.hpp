/**
 * @file Result.hpp
 * @brief Everything one analysis produces: the echoed inputs, every intermediate value, the
 *        checks and the verdict (NFR-09).
 */

#pragma once

#include <string>
#include <vector>

#include "Checks.hpp"
#include "ForceContribution.hpp"
#include "PressureDistribution.hpp"
#include "Resultant.hpp"
#include "model/Scenario.hpp"

namespace simulation {

struct Result {
    bool operator==(const Result&) const = default;

    model::Scenario scenario;
    PressureDistribution pressure;
    std::vector<ForceContribution> contributions;
    Resultant resultant;
    Contact contact;
    SlidingResult sliding;
    OverturningReport overturning;
    Checks checks;
    std::vector<std::string> warnings;
    Verdict verdict;
};

} // namespace simulation
