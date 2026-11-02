/**
 * @file UpliftLoad.hpp
 * @brief Water pressure pushing up on the base of the dam (uplift, FR-24).
 *
 * Pure, closed-form load for the case where the whole base is in compression (decision D-U1,
 * EM 1110-2-2200 para. 3-3.d, Figs. 3-1 to 3-3). The head falls from H1 (reservoir) at the heel to
 * H2 (tailwater, 0 if none) at the toe:
 *   - without drains, linearly;
 *   - with drains, through the drain-line head H3 = K (H1 - H') (L - X) / L + H' at distance X from
 *     the heel, where K = 1 - E and H' is the gallery elevation if that is above the tailwater,
 *     else the tailwater depth;
 *   - with drains within 0.05 H1 of the heel (Fig. 3-3), as one straight line from
 *     H3 = K (H1 - H') + H' at the heel to H2 at the toe.
 * The result is a single upward force at the centroid of the diagram, acting on the base (y = 0).
 * A scenario without an `uplift` request contributes nothing (NFR-12).
 */

#pragma once

#include <vector>

#include "Load.hpp"
#include "math/Point2.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

namespace simulation {

// One break point of the uplift pressure diagram: section x and the water pressure there.
struct UpliftSample {
    bool operator==(const UpliftSample&) const = default;
    units::Length x;
    units::Pressure pressure;
};

// The diagram's break points from heel to toe (heel, the drain line if the bilinear rule applies,
// toe), so a straight line between neighbours reproduces the diagram exactly (FR-30). Empty when the
// scenario has no `uplift` request.
std::vector<UpliftSample> uplift_diagram(const model::Scenario& scenario);

class UpliftLoad final : public Load {
public:
    std::vector<ForceContribution> contributions(const model::Scenario& scenario) const override;
};

} // namespace simulation
