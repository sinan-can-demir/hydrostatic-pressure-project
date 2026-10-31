/**
 * @file WaterLoad.hpp
 * @brief Hydrostatic load of the upstream reservoir, p = rho g (Hw - y) normal to the face.
 *
 * The wetted face is integrated segment by segment in closed form (no quadrature). The result is
 * split into a horizontal and a vertical contribution; each point is the centroid of its
 * pressure diagram, which lies on the line of action of that component. A component that is
 * exactly zero (dry face, vertical face) is not reported.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "Load.hpp"
#include "PressureDistribution.hpp"

namespace simulation {

class WaterLoad final : public Load {
public:
    std::vector<ForceContribution> contributions(const model::Scenario& scenario) const override;

    // Samples at the wetted face vertices plus the water-surface point; with a sample count,
    // that many points evenly spaced along the wetted face (at least 2).
    PressureDistribution pressure(const model::Scenario& scenario,
                                  std::optional<std::size_t> sample_count = std::nullopt) const;
};

} // namespace simulation
