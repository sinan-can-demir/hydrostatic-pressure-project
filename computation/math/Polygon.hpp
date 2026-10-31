/**
 * @file Polygon.hpp
 * @brief Polygon measures and validity tests for 2D sections (vertex lists, implicitly closed).
 *
 * Tolerance policy (ADR-0007): geometric tests use geometric_tolerance(), a relative
 * tolerance of 1e-12 times the polygon's bounding-box size.
 */

#pragma once

#include <span>

#include "Point2.hpp"
#include "units/Area.hpp"
#include "units/Length.hpp"

namespace math {

// Positive for counter-clockwise vertex order, negative for clockwise; zero below 3 vertices.
units::Area signed_area(std::span<const Point2> vertices);

// Area centroid. Requires a non-zero area; otherwise returns the first vertex (or the origin
// for an empty list).
Point2 centroid(std::span<const Point2> vertices);

// True when the boundary does not cross or touch itself: no two non-adjacent edges meet,
// adjacent edges do not fold back on each other and no edge has zero length.
bool is_simple(std::span<const Point2> vertices, units::Length tolerance);

// 1e-12 times the larger bounding-box dimension; zero for an empty list.
units::Length geometric_tolerance(std::span<const Point2> vertices);

} // namespace math
