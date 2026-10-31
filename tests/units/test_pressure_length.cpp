#include <catch2/catch_test_macros.hpp>

#include <type_traits>

#include "units/Density.hpp"
#include "units/ForcePerWidth.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

using namespace units;

// Domain decision M1: pressure times length is the force per unit width of a 2D section.
TEST_CASE("pressure times length is a force per width", "[NFR-02][FR-06][U-02]") {
    static_assert(std::is_same_v<decltype(Pressure(1.0) * Length(1.0)), ForcePerWidth>);
    static_assert(std::is_same_v<decltype(Length(1.0) * Pressure(1.0)), ForcePerWidth>);

    const Pressure p(98100.0);
    const Length h(10.0);
    CHECK((p * h).n_per_m == 981000.0);
    CHECK((h * p).n_per_m == 981000.0);
}

TEST_CASE("pressure arithmetic is unchanged", "[NFR-02][U-01]") {
    CHECK((Pressure(2.0) + Pressure(3.0)).pascals == 5.0);
    CHECK((Pressure(2.0) - Pressure(3.0)).pascals == -1.0);
    CHECK((Pressure(2.0) * 4.0).pascals == 8.0);
    CHECK((4.0 * Pressure(2.0)).pascals == 8.0);
}
