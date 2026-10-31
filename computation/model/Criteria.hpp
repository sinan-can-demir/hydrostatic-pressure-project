/**
 * @file Criteria.hpp
 * @brief Stability criteria per load condition; the default preset is
 *        USACE EM 1110-2-2200 (30 Jun 1995) Table 4-1.
 */

#pragma once

namespace model {

enum class LoadCondition { Usual, Unusual, Extreme };

// Allowed zone of the base resultant, measured from the toe as a fraction of the base length.
enum class ResultantZone { MiddleThird, MiddleHalf, WithinBase };

struct CriteriaRow {
    bool operator==(const CriteriaRow&) const = default;
    ResultantZone zone;
    double min_sliding_fs;
};

struct CriteriaSet {
    bool operator==(const CriteriaSet&) const = default;
    CriteriaRow usual{ResultantZone::MiddleThird, 2.0};
    CriteriaRow unusual{ResultantZone::MiddleHalf, 1.7};
    CriteriaRow extreme{ResultantZone::WithinBase, 1.3};

    static constexpr CriteriaSet usace() { return CriteriaSet{}; }

    constexpr CriteriaRow row_for(LoadCondition condition) const {
        if (condition == LoadCondition::Usual) return usual;
        if (condition == LoadCondition::Unusual) return unusual;
        return extreme;
    }
};

} // namespace model
