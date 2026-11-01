#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "units/Acceleration.hpp"
#include "units/Angle.hpp"
#include "units/Area.hpp"
#include "units/Density.hpp"
#include "units/ForcePerWidth.hpp"
#include "units/Length.hpp"
#include "units/MomentPerWidth.hpp"
#include "units/Pressure.hpp"
#include "units/UnitWeight.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace units;

TEST_CASE("units expose their SI values", "[NFR-02][U-01]") {
    CHECK(Length(2.5).meters == 2.5);
    CHECK(Area(6.0).square_meters == 6.0);
    CHECK(UnitWeight(23500.0).n_per_m3 == 23500.0);
    CHECK(ForcePerWidth(10.0).n_per_m == 10.0);
    CHECK(MomentPerWidth(7.0).n_m_per_m == 7.0);
    CHECK(Angle(1.0).radians == 1.0);
}

TEST_CASE("hydrostatic pressure of 1 m of water is 9810 Pa", "[NFR-02][U-01]") {
    const Pressure p = hydrostatic(Density(1000.0), Acceleration(9.81), Length(1.0));
    CHECK_THAT(p.pascals, WithinRel(9810.0, 1e-12));
}

TEST_CASE("length arithmetic", "[NFR-02][U-01]") {
    CHECK((Length(2.0) + Length(3.0)).meters == 5.0);
    CHECK((Length(2.0) - Length(3.0)).meters == -1.0);
    CHECK((Length(2.0) * 4.0).meters == 8.0);
    CHECK((4.0 * Length(2.0)).meters == 8.0);
    CHECK((-Length(2.0)).meters == -2.0);
}

TEST_CASE("length times length is an area", "[NFR-02][U-01]") {
    static_assert(std::is_same_v<decltype(Length(1.0) * Length(1.0)), Area>);
    CHECK((Length(5.0) * Length(10.0)).square_meters == 50.0);
}

TEST_CASE("area arithmetic", "[NFR-02][U-01]") {
    CHECK((Area(2.0) + Area(3.0)).square_meters == 5.0);
    CHECK((Area(2.0) - Area(3.0)).square_meters == -1.0);
    CHECK((Area(2.0) * 4.0).square_meters == 8.0);
    CHECK((4.0 * Area(2.0)).square_meters == 8.0);
}

TEST_CASE("unit weight times area is a force per width", "[NFR-02][U-01]") {
    static_assert(std::is_same_v<decltype(UnitWeight(1.0) * Area(1.0)), ForcePerWidth>);
    CHECK((UnitWeight(23500.0) * Area(50.0)).n_per_m == 1175000.0);
    CHECK((Area(50.0) * UnitWeight(23500.0)).n_per_m == 1175000.0);
}

TEST_CASE("force per width arithmetic", "[NFR-02][U-01]") {
    CHECK((ForcePerWidth(2.0) + ForcePerWidth(3.0)).n_per_m == 5.0);
    CHECK((ForcePerWidth(2.0) - ForcePerWidth(3.0)).n_per_m == -1.0);
    CHECK((ForcePerWidth(2.0) * 4.0).n_per_m == 8.0);
    CHECK((4.0 * ForcePerWidth(2.0)).n_per_m == 8.0);
    CHECK((-ForcePerWidth(2.0)).n_per_m == -2.0);
}

TEST_CASE("force per width times length is a moment per width", "[NFR-02][U-01]") {
    static_assert(std::is_same_v<decltype(ForcePerWidth(1.0) * Length(1.0)), MomentPerWidth>);
    CHECK((ForcePerWidth(490500.0) * Length(2.0)).n_m_per_m == 981000.0);
    CHECK((Length(2.0) * ForcePerWidth(490500.0)).n_m_per_m == 981000.0);
}

TEST_CASE("moment per width arithmetic", "[NFR-02][U-01]") {
    CHECK((MomentPerWidth(2.0) + MomentPerWidth(3.0)).n_m_per_m == 5.0);
    CHECK((MomentPerWidth(2.0) - MomentPerWidth(3.0)).n_m_per_m == -1.0);
    CHECK((MomentPerWidth(2.0) * 4.0).n_m_per_m == 8.0);
    CHECK((4.0 * MomentPerWidth(2.0)).n_m_per_m == 8.0);
    CHECK((-MomentPerWidth(2.0)).n_m_per_m == -2.0);
}

TEST_CASE("moment per width divided by force per width is a length", "[NFR-02][U-01]") {
    static_assert(std::is_same_v<decltype(MomentPerWidth(1.0) / ForcePerWidth(1.0)), Length>);
    CHECK((MomentPerWidth(1302500.0) / ForcePerWidth(1175000.0)).meters == 1302500.0 / 1175000.0);
}

TEST_CASE("angle conversions", "[NFR-02][U-01]") {
    CHECK_THAT(Angle::from_degrees(180.0).radians, WithinRel(3.14159265358979323846, 1e-15));
    CHECK_THAT(Angle(3.14159265358979323846).degrees(), WithinRel(180.0, 1e-15));
    CHECK_THAT(Angle::from_degrees(35.0).degrees(), WithinRel(35.0, 1e-14));
}

TEST_CASE("tangent of an angle", "[NFR-02][U-01]") {
    CHECK_THAT(tan(Angle::from_degrees(35.0)), WithinRel(0.7002075382097097, 1e-14));
    CHECK_THAT(tan(Angle::from_degrees(45.0)), WithinRel(1.0, 1e-14));
    CHECK_THAT(tan(Angle::from_degrees(0.0)), WithinAbs(0.0, 1e-15));
}
