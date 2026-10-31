#pragma once

/**
 * @file Length.hpp
 * @author Sinan Demir
 * @brief
 * @version 0.1
 * @date 2026-01-06
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Area.hpp"

namespace units {

struct Length {
    bool operator==(const Length&) const = default;
    double meters;

    constexpr explicit Length(double m) : meters(m) {}
};

constexpr Length operator+(Length a, Length b) { return Length(a.meters + b.meters); }
constexpr Length operator-(Length a, Length b) { return Length(a.meters - b.meters); }
constexpr Length operator-(Length a) { return Length(-a.meters); }
constexpr Length operator*(Length a, double s) { return Length(a.meters * s); }
constexpr Length operator*(double s, Length a) { return Length(s * a.meters); }

constexpr Area operator*(Length a, Length b) { return Area(a.meters * b.meters); }

} // namespace units
