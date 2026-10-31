/**
 * @file Load.hpp
 * @brief Extension interface (ADR-0004): something that produces force contributions.
 *
 * Implementations must be pure and deterministic (NFR-03): the same Scenario always yields the
 * same contributions, in the same order.
 */

#pragma once

#include <vector>

#include "ForceContribution.hpp"
#include "model/Scenario.hpp"

namespace simulation {

class Load {
public:
    virtual ~Load() = default;
    virtual std::vector<ForceContribution> contributions(const model::Scenario& scenario) const = 0;
};

} // namespace simulation
