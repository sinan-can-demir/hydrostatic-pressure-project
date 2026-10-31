/**
 * @file SelfWeight.hpp
 * @brief Weight of the dam body: W = unit weight * area, acting down through the centroid (FR-07).
 */

#pragma once

#include <vector>

#include "Load.hpp"

namespace simulation {

class SelfWeight final : public Load {
public:
    std::vector<ForceContribution> contributions(const model::Scenario& scenario) const override;
};

} // namespace simulation
