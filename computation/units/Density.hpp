/**
 * @file Density.hpp
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

struct Density {
    bool operator==(const Density&) const = default;
    double kg_per_m3;

    constexpr explicit Density(double rho) : kg_per_m3(rho) {}
};

} // namespace units
