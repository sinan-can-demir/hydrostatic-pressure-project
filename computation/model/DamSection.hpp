/**
 * @file DamSection.hpp
 * @brief Gravity-dam cross-section: a counter-clockwise polygon with its base on y = 0.
 *
 * The geometry queries assume a section that passed simulation::validate(); on an invalid
 * section they return unspecified (but non-crashing) values.
 */

#pragma once

#include <cstddef>
#include <vector>

#include "math/Point2.hpp"
#include "units/Area.hpp"
#include "units/Length.hpp"

namespace model {

struct DamSection {
    bool operator==(const DamSection&) const = default;
    std::vector<math::Point2> vertices;

    // Indices of the vertices lying on the base line y = 0 (within the geometric tolerance).
    std::vector<std::size_t> base_vertex_indices() const;

    math::Point2 heel() const; // upstream base corner
    math::Point2 toe() const;  // downstream base corner
    units::Length base_length() const;
    units::Length crest_height() const;
    units::Area area() const;
    math::Point2 centroid() const;

    // Boundary chain from the heel up to the crest, heel first. The crest vertex is the
    // upstream-most of the highest vertices.
    std::vector<math::Point2> upstream_face() const;
};

} // namespace model
