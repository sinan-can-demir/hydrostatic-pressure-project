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


namespace units {

struct Length {
    double meters;

    constexpr explicit Length(double m) : meters(m) {}
};

} // namespace units
