/**
 * @file TailwaterLoad.hpp
 * @brief Hydrostatic load of the downstream water (tailwater) on the downstream face (FR-23).
 *
 * Nonoverflow sections only: the full tailwater depth is used, with no retrogression reduction
 * (EM 1110-2-2200 para. 3-3.c(3)(a), decision D-U3). The load points into the dam, so its
 * horizontal component is negative (towards the upstream side). A scenario without a tailwater
 * depth, or with depth zero, contributes nothing (NFR-12).
 */

#pragma once

#include <vector>

#include "Load.hpp"

namespace simulation {

class TailwaterLoad final : public Load {
public:
    std::vector<ForceContribution> contributions(const model::Scenario& scenario) const override;
};

} // namespace simulation
