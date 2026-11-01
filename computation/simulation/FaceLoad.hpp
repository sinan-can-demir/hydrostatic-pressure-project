/**
 * @file FaceLoad.hpp
 * @brief Shared closed-form integration of a hydrostatic load on a polyline face.
 *
 * Used by WaterLoad (upstream face) and TailwaterLoad (downstream face). The face is a chain
 * whose elevation never decreases along it (validated, FR-02 and FR-22); the wetted part is a
 * prefix plus one clipped point. Each segment is integrated exactly, with no quadrature.
 */

#pragma once

#include <vector>

#include "ForceContribution.hpp"
#include "math/Point2.hpp"

namespace simulation::detail {

struct Pt {
    double x;
    double y;
};

// The part of `face` (ordered upward) that lies below `level`, clipped at the level.
std::vector<Pt> wetted_chain(const std::vector<math::Point2>& face, double level);

// Which side of the dam the water is on. The load on the dam points into the dam: for a chain
// ordered upward, the dam is on the right of the upstream face and on the left of the
// downstream face.
enum class Side { Upstream, Downstream };

// Horizontal and vertical contributions of water of depth `level` (pressure k * (level - y),
// k = rho g) on the wetted chain. A component that is exactly zero is omitted. The names are the
// `source` labels of the two contributions.
std::vector<ForceContribution> face_contributions(const std::vector<Pt>& chain, double level, double k,
                                                  Side side, const char* horizontal_name,
                                                  const char* vertical_name);

} // namespace simulation::detail
