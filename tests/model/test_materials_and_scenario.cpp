#include <catch2/catch_test_macros.hpp>

#include <optional>

#include "materials/Concrete.hpp"
#include "materials/Fluid.hpp"
#include "math/Point2.hpp"
#include "model/Criteria.hpp"
#include "model/Scenario.hpp"
#include "units/Angle.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

using namespace units;
using math::Point2;

TEST_CASE("fluid defaults to water", "[FR-03]") {
    const materials::Fluid fluid;
    CHECK(fluid.density.kg_per_m3 == 1000.0);
    CHECK(fluid.gravity.m_per_s2 == 9.81);
}

TEST_CASE("fluid values can be overridden", "[FR-03]") {
    const materials::Fluid seawater{.density = Density(1025.0)};
    CHECK(seawater.density.kg_per_m3 == 1025.0);
    CHECK(seawater.gravity.m_per_s2 == 9.81);
}

TEST_CASE("concrete defaults to 23.5 kN/m3", "[FR-03]") {
    const materials::Concrete concrete;
    CHECK(concrete.unit_weight.n_per_m3 == 23500.0);
    CHECK(materials::Concrete{.unit_weight = UnitWeight(24000.0)}.unit_weight.n_per_m3 == 24000.0);
}

TEST_CASE("criteria preset equals EM 1110-2-2200 Table 4-1", "[FR-13]") {
    const model::CriteriaSet usace = model::CriteriaSet::usace();

    const model::CriteriaRow usual = usace.row_for(model::LoadCondition::Usual);
    CHECK(usual.zone == model::ResultantZone::MiddleThird);
    CHECK(usual.min_sliding_fs == 2.0);

    const model::CriteriaRow unusual = usace.row_for(model::LoadCondition::Unusual);
    CHECK(unusual.zone == model::ResultantZone::MiddleHalf);
    CHECK(unusual.min_sliding_fs == 1.7);

    const model::CriteriaRow extreme = usace.row_for(model::LoadCondition::Extreme);
    CHECK(extreme.zone == model::ResultantZone::WithinBase);
    CHECK(extreme.min_sliding_fs == 1.3);
}

namespace {

// AC-1 scenario: rectangular dam B = 5 m, H = 10 m, water at the crest, phi = 35 degrees.
model::Scenario ac1() {
    return model::Scenario{
        .section = model::DamSection{{Point2{Length(0), Length(0)}, Point2{Length(5), Length(0)},
                                      Point2{Length(5), Length(10)}, Point2{Length(0), Length(10)}}},
        .water_level = model::WaterLevel{Length(10.0)},
        .foundation = model::Foundation{Angle::from_degrees(35.0)},
    };
}

} // namespace

TEST_CASE("scenario holds the AC-1 inputs", "[FR-01][FR-03][FR-04]") {
    const model::Scenario s = ac1();
    REQUIRE(s.section.vertices.size() == 4);
    CHECK(s.section.vertices[2].x.meters == 5.0);
    CHECK(s.section.vertices[2].y.meters == 10.0);
    CHECK(s.water_level.elevation.meters == 10.0);
    CHECK(s.foundation.friction_angle.degrees() > 34.999999);
    CHECK(s.foundation.friction_angle.degrees() < 35.000001);
}

TEST_CASE("scenario optional inputs take documented defaults", "[FR-03][FR-13][FR-14]") {
    const model::Scenario s = ac1();
    CHECK(s.condition == model::LoadCondition::Usual);
    CHECK(s.fluid.density.kg_per_m3 == 1000.0);
    CHECK(s.fluid.gravity.m_per_s2 == 9.81);
    CHECK(s.concrete.unit_weight.n_per_m3 == 23500.0);
    CHECK_FALSE(s.foundation.allowable_bearing.has_value());
    CHECK(s.criteria.row_for(model::LoadCondition::Usual).min_sliding_fs == 2.0);
}

TEST_CASE("scenario accepts an allowable bearing pressure and a load condition", "[FR-14]") {
    model::Scenario s = ac1();
    s.foundation.allowable_bearing = Pressure(1.5e6);
    s.condition = model::LoadCondition::Extreme;
    REQUIRE(s.foundation.allowable_bearing.has_value());
    CHECK(s.foundation.allowable_bearing->pascals == 1.5e6);
    CHECK(s.condition == model::LoadCondition::Extreme);
}
