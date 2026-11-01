/**
 * @file Acceleration.hpp
 * @author Sinan Demir
 * @brief
 * @version 0.1
 * @date 2026-01-06
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

namespace units {

struct Acceleration {
    bool operator==(const Acceleration&) const = default;
    double m_per_s2;

    constexpr explicit Acceleration(double g) : m_per_s2(g) {}
};

} // namespace units
