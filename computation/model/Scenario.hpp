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
};

} // namespace model
