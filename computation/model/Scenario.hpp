/**
 * @file Scenario.hpp
 * @brief Everything needed for one analysis run. Plain data: validation happens in the
 *        Validator (FR-02 to FR-04), not in these types.
 */

#pragma once

#include <cstddef>
#include <optional>

#include "Criteria.hpp"
#include "DamSection.hpp"
#include "materials/Concrete.hpp"
#include "materials/Fluid.hpp"
#include "units/Angle.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

namespace model {

// Elevation of the upstream water surface above the base.
struct WaterLevel {
    bool operator==(const WaterLevel&) const = default;
    units::Length elevation;
};

// Cohesion is fixed at zero in v1 (CON-05).
struct Foundation {
    bool operator==(const Foundation&) const = default;
    units::Angle friction_angle;
    std::optional<units::Pressure> allowable_bearing = std::nullopt;
};

// Foundation drains under the base (v1.1, FR-25). The effectiveness E is the fraction of the
// head drop from heel to tailwater that the drains remove at the drain line, 0 < E <= 1.
// There is deliberately no default: it is an engineering judgement (docs/09, decision D-U2).
struct Drains {
    bool operator==(const Drains&) const = default;
    double effectiveness;
    units::Length distance_from_heel;
    units::Length gallery_elevation = units::Length(0.0); // drain-line elevation above the base
};

// Request to model uplift on the base (v1.1, FR-24). Without drains the head falls linearly
// from the heel to the toe.
struct Uplift {
    bool operator==(const Uplift&) const = default;
    std::optional<Drains> drains = std::nullopt;
};

struct Scenario {
    bool operator==(const Scenario&) const = default;
    DamSection section;
    WaterLevel water_level;
    Foundation foundation;
    LoadCondition condition = LoadCondition::Usual;
    materials::Fluid fluid{};
    materials::Concrete concrete{};
    CriteriaSet criteria = CriteriaSet::usace();
    // Number of evenly spaced pressure samples along the wetted face (FR-20); empty = the face vertices.
    std::optional<std::size_t> pressure_samples = std::nullopt;
    // v1.1: both absent means the v1 analysis, unchanged (NFR-12).
    std::optional<units::Length> tailwater_depth = std::nullopt; // downstream water above the base (FR-22)
    std::optional<Uplift> uplift = std::nullopt;
};

} // namespace model
