#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "math/Point2.hpp"
#include "model/Scenario.hpp"
#include "simulation/ValidationError.hpp"
#include "simulation/Validator.hpp"
#include "units/Angle.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

using namespace units;
using math::Point2;
using simulation::ErrorCode;
using simulation::ValidationError;

namespace {

Point2 P(double x, double y) { return Point2{Length(x), Length(y)}; }

model::Scenario scenario(std::vector<Point2> vertices, double water = 10.0, double phi_deg = 35.0) {
    return model::Scenario{
        .section = model::DamSection{std::move(vertices)},
        .water_level = model::WaterLevel{Length(water)},
        .foundation = model::Foundation{Angle::from_degrees(phi_deg)},
    };
}

model::Scenario ac1(double water = 10.0, double phi_deg = 35.0) {
    return scenario({P(0, 0), P(5, 0), P(5, 10), P(0, 10)}, water, phi_deg);
}

bool has(const std::vector<ValidationError>& errors, ErrorCode code) {
    return std::any_of(errors.begin(), errors.end(),
                       [code](const ValidationError& e) { return e.code == code; });
}

const ValidationError* find(const std::vector<ValidationError>& errors, ErrorCode code) {
    const auto it = std::find_if(errors.begin(), errors.end(),
                                 [code](const ValidationError& e) { return e.code == code; });
    return it == errors.end() ? nullptr : &*it;
}

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

} // namespace

TEST_CASE("valid reference scenarios produce no errors", "[FR-01][FR-02][V-01]") {
    CHECK(simulation::validate(ac1()).empty());
    CHECK(simulation::validate(scenario({P(0, 0), P(7, 0), P(0, 10)})).empty());
    CHECK(simulation::validate(scenario({P(0, 0), P(5, 0), P(5, 10), P(2, 10)})).empty());
    CHECK(simulation::validate(scenario({P(0, 0), P(5, 0), P(5, 10), P(-2, 10)})).empty());
    CHECK(simulation::validate(ac1(6.0)).empty());
    CHECK(simulation::validate(ac1(0.0)).empty());
}

TEST_CASE("error codes have stable names", "[V-01]") {
    CHECK(simulation::to_string(ErrorCode::ClockwiseOrder) == "clockwise_order");
    CHECK(simulation::to_string(ErrorCode::TooFewVertices) == "too_few_vertices");
    CHECK(simulation::to_string(ErrorCode::UpstreamFaceDips) == "upstream_face_dips");
    CHECK(simulation::to_string(ErrorCode::WaterLevelAboveCrest) == "water_level_above_crest");
}

// ---- geometry -------------------------------------------------------------

TEST_CASE("fewer than three vertices is rejected", "[FR-01][FR-02][V-01]") {
    CHECK(has(simulation::validate(scenario({})), ErrorCode::TooFewVertices));
    CHECK(has(simulation::validate(scenario({P(0, 0)})), ErrorCode::TooFewVertices));
    CHECK(has(simulation::validate(scenario({P(0, 0), P(5, 0)})), ErrorCode::TooFewVertices));
}

TEST_CASE("clockwise vertex order is rejected with a clear message", "[FR-02][G-03][V-01]") {
    const auto errors = simulation::validate(scenario({P(0, 10), P(5, 10), P(5, 0), P(0, 0)}));
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].code == ErrorCode::ClockwiseOrder);
    CHECK(errors[0].path == "dam.vertices_m");
    CHECK(errors[0].message.find("clockwise") != std::string::npos);
    CHECK(errors[0].message.find("counter-clockwise") != std::string::npos);
}

TEST_CASE("self-intersecting section is rejected", "[FR-02][G-04][V-01]") {
    CHECK(has(simulation::validate(scenario({P(0, 0), P(5, 0), P(1, 6), P(4, 6)})),
              ErrorCode::SelfIntersecting));
}

TEST_CASE("zero area is rejected", "[FR-02][V-01]") {
    CHECK(has(simulation::validate(scenario({P(0, 0), P(5, 0), P(10, 0)})), ErrorCode::ZeroArea));
}

TEST_CASE("repeated consecutive vertices are rejected", "[FR-02][V-01]") {
    const auto errors = simulation::validate(scenario({P(0, 0), P(5, 0), P(5, 0), P(5, 10), P(0, 10)}));
    REQUIRE(has(errors, ErrorCode::DuplicateVertex));
    CHECK(find(errors, ErrorCode::DuplicateVertex)->path == "dam.vertices_m[2]");
}

TEST_CASE("a section without a base edge on y = 0 is rejected", "[FR-02][G-05][V-01]") {
    // Floating above the base.
    CHECK(has(simulation::validate(scenario({P(0, 5), P(5, 5), P(2, 10)})), ErrorCode::NoBaseEdge));
    // Sloped bottom: only one vertex on y = 0.
    CHECK(has(simulation::validate(scenario({P(0, 0), P(5, 1), P(5, 10), P(0, 10)})), ErrorCode::NoBaseEdge));
}

TEST_CASE("more than one base edge is rejected", "[FR-02][G-05][V-01]") {
    const auto errors = simulation::validate(
        scenario({P(0, 0), P(2, 0), P(2, 5), P(3, 5), P(3, 0), P(5, 0), P(5, 10), P(0, 10)}));
    CHECK(has(errors, ErrorCode::MultipleBaseEdges));
}

TEST_CASE("a vertex below the base is rejected", "[FR-02][G-05][V-01]") {
    const auto errors = simulation::validate(scenario({P(0, -1), P(5, -1), P(5, 10), P(0, 10)}));
    REQUIRE(has(errors, ErrorCode::VertexBelowBase));
    CHECK(find(errors, ErrorCode::VertexBelowBase)->path == "dam.vertices_m[0]");
}

TEST_CASE("an upstream face that dips is rejected", "[FR-02][G-06][V-01]") {
    // From the crest (2,10) the face goes down to (3,4) and then back up to (0,8).
    const auto errors =
        simulation::validate(scenario({P(0, 0), P(8, 0), P(8, 10), P(2, 10), P(3, 4), P(0, 8)}));
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].code == ErrorCode::UpstreamFaceDips);
}

TEST_CASE("a stepped but non-decreasing upstream face is accepted", "[FR-02][G-06]") {
    CHECK(simulation::validate(scenario({P(0, 0), P(8, 0), P(8, 10), P(2, 10), P(2, 5), P(0, 5)})).empty());
}

// ---- numeric sanity -------------------------------------------------------

TEST_CASE("NaN, infinite and huge coordinates are rejected", "[NFR-10][V-05]") {
    const auto nan_errors = simulation::validate(scenario({P(0, 0), P(5, 0), P(kNaN, 10), P(0, 10)}));
    REQUIRE(has(nan_errors, ErrorCode::NonFiniteValue));
    CHECK(find(nan_errors, ErrorCode::NonFiniteValue)->path == "dam.vertices_m[2]");

    CHECK(has(simulation::validate(scenario({P(0, 0), P(5, 0), P(5, kInf), P(0, 10)})),
              ErrorCode::NonFiniteValue));
    CHECK(has(simulation::validate(scenario({P(0, 0), P(2e6, 0), P(2e6, 10), P(0, 10)})),
              ErrorCode::ValueOutOfRange));
    CHECK(has(simulation::validate(ac1(kNaN)), ErrorCode::NonFiniteValue));
    CHECK(has(simulation::validate(ac1(kInf)), ErrorCode::NonFiniteValue));
}

// ---- water level ----------------------------------------------------------

TEST_CASE("water level must lie between the base and the crest", "[FR-04][V-03]") {
    CHECK(simulation::validate(ac1(0.0)).empty());
    CHECK(simulation::validate(ac1(10.0)).empty());
    CHECK(has(simulation::validate(ac1(10.5)), ErrorCode::WaterLevelAboveCrest));
    const auto below = simulation::validate(ac1(-0.1));
    REQUIRE(has(below, ErrorCode::WaterLevelNegative));
    CHECK(find(below, ErrorCode::WaterLevelNegative)->path == "water_level_m");
}

// ---- friction angle -------------------------------------------------------

TEST_CASE("friction angle must be strictly between 0 and 90 degrees", "[UC-02][V-04]") {
    CHECK(simulation::validate(ac1(10.0, 0.001)).empty());
    CHECK(simulation::validate(ac1(10.0, 89.999)).empty());
    CHECK(has(simulation::validate(ac1(10.0, 0.0)), ErrorCode::FrictionAngleOutOfRange));
    CHECK(has(simulation::validate(ac1(10.0, 90.0)), ErrorCode::FrictionAngleOutOfRange));
    CHECK(has(simulation::validate(ac1(10.0, -5.0)), ErrorCode::FrictionAngleOutOfRange));
    const auto nan = simulation::validate(ac1(10.0, kNaN));
    REQUIRE(has(nan, ErrorCode::NonFiniteValue));
    CHECK(find(nan, ErrorCode::NonFiniteValue)->path == "friction_angle_deg");
}

// ---- material and fluid parameters ----------------------------------------

TEST_CASE("fluid, concrete, bearing and criteria values are checked", "[FR-03][FR-14][UC-06][V-01]") {
    model::Scenario s = ac1();
    s.fluid.density = Density(0.0);
    CHECK(has(simulation::validate(s), ErrorCode::NonPositiveFluidDensity));

    s = ac1();
    s.fluid.gravity = Acceleration(-9.81);
    CHECK(has(simulation::validate(s), ErrorCode::NonPositiveGravity));

    s = ac1();
    s.concrete.unit_weight = UnitWeight(0.0);
    CHECK(has(simulation::validate(s), ErrorCode::NonPositiveUnitWeight));

    s = ac1();
    s.fluid.density = Density(kNaN);
    const auto nan = simulation::validate(s);
    REQUIRE(has(nan, ErrorCode::NonFiniteValue));
    CHECK(find(nan, ErrorCode::NonFiniteValue)->path == "fluid.density_kg_m3");

    s = ac1();
    s.foundation.allowable_bearing = Pressure(0.0);
    CHECK(has(simulation::validate(s), ErrorCode::NonPositiveAllowableBearing));

    s = ac1();
    s.foundation.allowable_bearing = Pressure(1.0e6);
    CHECK(simulation::validate(s).empty());

    s = ac1();
    s.criteria.unusual.min_sliding_fs = 0.0;
    const auto fs = simulation::validate(s);
    REQUIRE(has(fs, ErrorCode::NonPositiveSlidingFs));
    CHECK(find(fs, ErrorCode::NonPositiveSlidingFs)->path == "criteria.unusual.min_sliding_fs");
}

// ---- all errors at once (decision U1) -------------------------------------

TEST_CASE("every error is reported in one pass", "[UC-09][V-02]") {
    model::Scenario s = scenario({P(0, 10), P(5, 10), P(5, 0), P(0, 0)}, 12.0, 0.0);
    s.fluid.density = Density(-1.0);
    const auto errors = simulation::validate(s);
    CHECK(has(errors, ErrorCode::ClockwiseOrder));
    CHECK(has(errors, ErrorCode::WaterLevelAboveCrest));
    CHECK(has(errors, ErrorCode::FrictionAngleOutOfRange));
    CHECK(has(errors, ErrorCode::NonPositiveFluidDensity));
    CHECK(errors.size() == 4);
}

TEST_CASE("validation never throws, whatever the input", "[NFR-04][A-01]") {
    CHECK_NOTHROW(simulation::validate(scenario({})));
    CHECK_NOTHROW(simulation::validate(scenario({P(kNaN, kNaN), P(kInf, 0), P(0, kNaN)}, kNaN, kNaN)));
    CHECK_NOTHROW(simulation::validate(scenario({P(0, 0), P(0, 0), P(0, 0)})));
}

// ---- pressure sample count (FR-20) ----------------------------------------

TEST_CASE("pressure_samples must be between 2 and 100000 when given", "[FR-20][V-01]") {
    model::Scenario s = ac1();
    CHECK(simulation::validate(s).empty());
    s.pressure_samples = 2;
    CHECK(simulation::validate(s).empty());
    s.pressure_samples = 100000;
    CHECK(simulation::validate(s).empty());
    for (const std::size_t bad : {std::size_t{0}, std::size_t{1}, std::size_t{100001}}) {
        s.pressure_samples = bad;
        const auto errors = simulation::validate(s);
        REQUIRE(has(errors, ErrorCode::PressureSamplesOutOfRange));
        CHECK(find(errors, ErrorCode::PressureSamplesOutOfRange)->path == "pressure_samples");
    }
}

TEST_CASE("every error code has its documented stable name", "[V-01][FR-19]") {
    // These names appear in CLI output and in docs/07-json-schema.md; changing one is a breaking change.
    const std::pair<ErrorCode, const char*> names[] = {
        {ErrorCode::TooFewVertices, "too_few_vertices"},
        {ErrorCode::NonFiniteValue, "non_finite_value"},
        {ErrorCode::ValueOutOfRange, "value_out_of_range"},
        {ErrorCode::DuplicateVertex, "duplicate_vertex"},
        {ErrorCode::ClockwiseOrder, "clockwise_order"},
        {ErrorCode::ZeroArea, "zero_area"},
        {ErrorCode::SelfIntersecting, "self_intersecting"},
        {ErrorCode::NoBaseEdge, "no_base_edge"},
        {ErrorCode::MultipleBaseEdges, "multiple_base_edges"},
        {ErrorCode::VertexBelowBase, "vertex_below_base"},
        {ErrorCode::UpstreamFaceDips, "upstream_face_dips"},
        {ErrorCode::WaterLevelNegative, "water_level_negative"},
        {ErrorCode::WaterLevelAboveCrest, "water_level_above_crest"},
        {ErrorCode::FrictionAngleOutOfRange, "friction_angle_out_of_range"},
        {ErrorCode::NonPositiveFluidDensity, "non_positive_fluid_density"},
        {ErrorCode::NonPositiveGravity, "non_positive_gravity"},
        {ErrorCode::NonPositiveUnitWeight, "non_positive_unit_weight"},
        {ErrorCode::NonPositiveAllowableBearing, "non_positive_allowable_bearing"},
        {ErrorCode::NonPositiveSlidingFs, "non_positive_sliding_fs"},
        {ErrorCode::PressureSamplesOutOfRange, "pressure_samples_out_of_range"},
        {ErrorCode::DownstreamFaceRises, "downstream_face_rises"},
        {ErrorCode::TailwaterNegative, "tailwater_negative"},
        {ErrorCode::TailwaterAboveWaterLevel, "tailwater_above_water_level"},
        {ErrorCode::DrainEffectivenessOutOfRange, "drain_effectiveness_out_of_range"},
        {ErrorCode::DrainDistanceOutOfRange, "drain_distance_out_of_range"},
        {ErrorCode::GalleryElevationNegative, "gallery_elevation_negative"},
    };
    for (const auto& [code, name] : names) CHECK(simulation::to_string(code) == name);
}

TEST_CASE("a water level beyond the coordinate limit is rejected", "[NFR-10][V-05]") {
    model::Scenario s = ac1();
    s.water_level = model::WaterLevel{Length(2.0e6)};
    const auto errors = simulation::validate(s);
    REQUIRE(has(errors, ErrorCode::ValueOutOfRange));
    CHECK(find(errors, ErrorCode::ValueOutOfRange)->path == "water_level_m");
}

// ---- tailwater and uplift inputs (v1.1, docs/09, test plan section 11) ----------------------

namespace {

model::Scenario with_tailwater(double depth) {
    model::Scenario s = ac1();
    s.tailwater_depth = Length(depth);
    return s;
}

model::Scenario with_drains(double effectiveness, double distance, double gallery = 0.0) {
    model::Scenario s = ac1();
    s.uplift = model::Uplift{model::Drains{effectiveness, Length(distance), Length(gallery)}};
    return s;
}

} // namespace

TEST_CASE("tailwater and uplift are absent by default and a scenario without them is unchanged",
          "[FR-22][NFR-12][TW-01]") {
    const model::Scenario s = ac1();
    CHECK_FALSE(s.tailwater_depth.has_value());
    CHECK_FALSE(s.uplift.has_value());
    CHECK(simulation::validate(s).empty());
}

TEST_CASE("a tailwater depth from zero up to the water level is accepted", "[FR-22][TW-01]") {
    CHECK(simulation::validate(with_tailwater(0.0)).empty());
    CHECK(simulation::validate(with_tailwater(3.0)).empty());
    CHECK(simulation::validate(with_tailwater(10.0)).empty()); // equal to Hw: no head difference
}

TEST_CASE("a bad tailwater depth is rejected with its own code and path", "[FR-22][TW-01][V-01]") {
    SECTION("negative") {
        const auto errors = simulation::validate(with_tailwater(-0.5));
        REQUIRE(has(errors, ErrorCode::TailwaterNegative));
        CHECK(find(errors, ErrorCode::TailwaterNegative)->path == "tailwater_depth_m");
    }
    SECTION("above the water level") {
        const auto errors = simulation::validate(with_tailwater(10.5));
        REQUIRE(has(errors, ErrorCode::TailwaterAboveWaterLevel));
        CHECK(find(errors, ErrorCode::TailwaterAboveWaterLevel)->path == "tailwater_depth_m");
    }
    SECTION("above a lower water level") {
        model::Scenario s = ac1(6.0);
        s.tailwater_depth = Length(7.0);
        CHECK(has(simulation::validate(s), ErrorCode::TailwaterAboveWaterLevel));
    }
    SECTION("not a number or infinite") {
        for (const double bad : {kNaN, kInf, -kInf}) {
            const auto errors = simulation::validate(with_tailwater(bad));
            REQUIRE(has(errors, ErrorCode::NonFiniteValue));
            CHECK(find(errors, ErrorCode::NonFiniteValue)->path == "tailwater_depth_m");
        }
    }
    SECTION("beyond the coordinate limit") {
        const auto errors = simulation::validate(with_tailwater(2.0e6));
        CHECK(has(errors, ErrorCode::ValueOutOfRange));
    }
}

TEST_CASE("a tailwater error is reported together with other errors", "[FR-22][TW-01][V-02]") {
    model::Scenario s = ac1(10.0, 95.0); // friction angle out of range
    s.tailwater_depth = Length(-1.0);
    const auto errors = simulation::validate(s);
    CHECK(has(errors, ErrorCode::FrictionAngleOutOfRange));
    CHECK(has(errors, ErrorCode::TailwaterNegative));
}

TEST_CASE("the tailwater is not compared with an invalid water level", "[FR-22][TW-01]") {
    model::Scenario s = ac1(kNaN);
    s.tailwater_depth = Length(3.0);
    const auto errors = simulation::validate(s);
    CHECK(has(errors, ErrorCode::NonFiniteValue));
    CHECK_FALSE(has(errors, ErrorCode::TailwaterAboveWaterLevel));
}

TEST_CASE("a downstream face that rises toward the toe is rejected", "[FR-22][TW-02][V-01]") {
    // Walking down from the crest (5,10) the face drops to (6,4), then climbs back to (8,8), then drops to
    // the toe (8,0). Going down toward the toe it must never climb.
    const auto errors =
        simulation::validate(scenario({P(0, 0), P(8, 0), P(8, 8), P(6, 4), P(5, 10), P(0, 10)}));
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].code == ErrorCode::DownstreamFaceRises);
}

TEST_CASE("vertical, sloping and stepped downstream faces are accepted", "[FR-22][TW-02]") {
    CHECK(simulation::validate(ac1()).empty());                                  // vertical
    CHECK(simulation::validate(scenario({P(0, 0), P(7, 0), P(0, 10)})).empty()); // sloping
    CHECK(simulation::validate(scenario({P(0, 0), P(8, 0), P(8, 4), P(5, 4), P(5, 10), P(0, 10)}))
              .empty()); // stepped but never rising toward the toe
}

TEST_CASE("drains with a valid effectiveness and position are accepted", "[FR-25][UP-01]") {
    CHECK(simulation::validate(with_drains(0.5, 1.0)).empty());
    CHECK(simulation::validate(with_drains(1.0, 0.0)).empty());  // E = 1 at the heel
    CHECK(simulation::validate(with_drains(0.25, 5.0)).empty()); // X = B
    CHECK(simulation::validate(with_drains(0.5, 1.0, 2.0)).empty());
}

TEST_CASE("uplift without a drains block is a valid request for undrained uplift", "[FR-24][FR-25][UP-01]") {
    model::Scenario s = ac1();
    s.uplift = model::Uplift{};
    CHECK_FALSE(s.uplift->drains.has_value());
    CHECK(simulation::validate(s).empty());
}

TEST_CASE("bad drain data is rejected with distinct codes and paths", "[FR-25][UP-01][V-01]") {
    SECTION("effectiveness zero, negative or above one") {
        for (const double e : {0.0, -0.1, 1.0001}) {
            const auto errors = simulation::validate(with_drains(e, 1.0));
            REQUIRE(has(errors, ErrorCode::DrainEffectivenessOutOfRange));
            CHECK(find(errors, ErrorCode::DrainEffectivenessOutOfRange)->path ==
                  "uplift.drains.effectiveness");
        }
    }
    SECTION("effectiveness not finite") {
        const auto errors = simulation::validate(with_drains(kNaN, 1.0));
        REQUIRE(has(errors, ErrorCode::NonFiniteValue));
        CHECK(find(errors, ErrorCode::NonFiniteValue)->path == "uplift.drains.effectiveness");
    }
    SECTION("distance negative or beyond the base length") {
        for (const double x : {-0.1, 5.1}) {
            const auto errors = simulation::validate(with_drains(0.5, x));
            REQUIRE(has(errors, ErrorCode::DrainDistanceOutOfRange));
            CHECK(find(errors, ErrorCode::DrainDistanceOutOfRange)->path ==
                  "uplift.drains.distance_from_heel_m");
        }
    }
    SECTION("distance not finite") {
        const auto errors = simulation::validate(with_drains(0.5, kInf));
        REQUIRE(has(errors, ErrorCode::NonFiniteValue));
        CHECK(find(errors, ErrorCode::NonFiniteValue)->path == "uplift.drains.distance_from_heel_m");
    }
    SECTION("gallery elevation negative") {
        const auto errors = simulation::validate(with_drains(0.5, 1.0, -1.0));
        REQUIRE(has(errors, ErrorCode::GalleryElevationNegative));
        CHECK(find(errors, ErrorCode::GalleryElevationNegative)->path == "uplift.drains.gallery_elevation_m");
    }
    SECTION("gallery elevation not finite") {
        const auto errors = simulation::validate(with_drains(0.5, 1.0, kNaN));
        CHECK(has(errors, ErrorCode::NonFiniteValue));
    }
}

TEST_CASE("the drain position is not checked against an invalid section", "[FR-25][UP-01]") {
    model::Scenario s = scenario({P(0, 0)});
    s.uplift = model::Uplift{model::Drains{0.5, Length(99.0), Length(0.0)}};
    const auto errors = simulation::validate(s);
    CHECK(has(errors, ErrorCode::TooFewVertices));
    CHECK_FALSE(has(errors, ErrorCode::DrainDistanceOutOfRange));
}
