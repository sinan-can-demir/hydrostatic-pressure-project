/**
 * @file Vector2.hpp
 * @brief 2D displacement in the section plane (x downstream, y up), components in metres.
 */

#pragma once

#include "units/Area.hpp"
#include "units/Length.hpp"

namespace math {

struct Vector2 {
    units::Length x;
    units::Length y;
};

constexpr Vector2 operator+(Vector2 a, Vector2 b) { return {a.x + b.x, a.y + b.y}; }
constexpr Vector2 operator-(Vector2 a, Vector2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Vector2 operator-(Vector2 a) { return {-a.x, -a.y}; }
constexpr Vector2 operator*(Vector2 a, double s) { return {a.x * s, a.y * s}; }
constexpr Vector2 operator*(double s, Vector2 a) { return a * s; }

// z component of the 3D cross product; positive when b is counter-clockwise from a.
constexpr units::Area cross(Vector2 a, Vector2 b) { return a.x * b.y - a.y * b.x; }

constexpr units::Area dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }

} // namespace math
