/**
 * @file Point2.hpp
 * @brief A position in the section plane (x downstream, y up), coordinates in metres.
 */

#pragma once

#include "Vector2.hpp"
#include "units/Length.hpp"

namespace math {

struct Point2 {
    bool operator==(const Point2&) const = default;
    units::Length x;
    units::Length y;
};

constexpr Vector2 operator-(Point2 a, Point2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Point2 operator+(Point2 p, Vector2 v) { return {p.x + v.x, p.y + v.y}; }

} // namespace math
