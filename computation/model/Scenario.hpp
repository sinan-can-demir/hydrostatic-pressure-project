/**
 * @file Scenario.hpp
 * @brief Everything needed for one analysis run. Plain data: validation happens in the
 *        Validator (FR-02 to FR-04), not in these types.
 */

#pragma once

#include <optional>
#include <vector>

#include "Criteria.hpp"
#include "materials/Concrete.hpp"
#include "materials/Fluid.hpp"
#include "math/Point2.hpp"
#include "units/Angle.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

namespace model {

// Counter-clockwise polygon with its base on y = 0.
struct DamSection {
    std::vector<math::Point2> vertices;
};

// Elevation of the upstream water surface above the base.
struct WaterLevel {
    units::Length elevation;
};

// Cohesion is fixed at zero in v1 (CON-05).
struct Foundation {
    units::Angle friction_angle;
    std::optional<units::Pressure> allowable_bearing = std::nullopt;
};

struct Scenario {
    DamSection section;
    WaterLevel water_level;
    Foundation foundation;
    LoadCondition condition = LoadCondition::Usual;
    materials::Fluid fluid{};
    materials::Concrete concrete{};
    CriteriaSet criteria = CriteriaSet::usace();
};

} // namespace model
