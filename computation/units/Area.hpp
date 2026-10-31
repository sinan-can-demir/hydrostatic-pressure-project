/**
 * @file Area.hpp
 * @brief Area in square metres.
 */

#pragma once

namespace units {

struct Area {
    double square_meters;

    constexpr explicit Area(double a) : square_meters(a) {}
};

constexpr Area operator+(Area a, Area b) { return Area(a.square_meters + b.square_meters); }
constexpr Area operator-(Area a, Area b) { return Area(a.square_meters - b.square_meters); }
constexpr Area operator*(Area a, double s) { return Area(a.square_meters * s); }
constexpr Area operator*(double s, Area a) { return Area(s * a.square_meters); }

} // namespace units
