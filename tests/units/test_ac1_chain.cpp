// Reference case AC-1 (docs/02-requirements.md section 6, docs/06-test-plan.md section 3)
// computed purely through the unit types. Rectangular dam B = 5 m, H = 10 m, water at the crest,
// rho = 1000 kg/m3, g = 9.81 m/s2, gamma = 23 500 N/m3. Hand derivation:
//   p_base = rho g H = 98 100 Pa;  Fx = 1/2 * p_base * H = 490 500 N/m, acting H/3 above the base
//   W = gamma * B * H = 1 175 000 N/m, acting B/2 = 2.5 m from the toe
//   M_res = 2 937 500;  M_over = 490 500 * 10/3 = 1 635 000;  x_R = (M_res - M_over) / W
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "units/Acceleration.hpp"
#include "units/Area.hpp"
#include "units/Density.hpp"
#include "units/ForcePerWidth.hpp"
#include "units/Length.hpp"
#include "units/MomentPerWidth.hpp"
#include "units/Pressure.hpp"
#include "units/UnitWeight.hpp"

using Catch::Matchers::WithinRel;
using namespace units;

TEST_CASE("AC-1 forces and moments through unit types", "[NFR-01][NFR-02][FR-06][FR-07][FR-08][AC-1]") {
    const Length base(5.0);
    const Length height(10.0);

    const Pressure p_base = hydrostatic(Density(1000.0), Acceleration(9.81), height);
    const ForcePerWidth water = 0.5 * (p_base * height);
    CHECK_THAT(water.n_per_m, WithinRel(490500.0, 1e-9));

    const ForcePerWidth weight = UnitWeight(23500.0) * (base * height);
    CHECK_THAT(weight.n_per_m, WithinRel(1175000.0, 1e-9));

    const MomentPerWidth resisting = weight * (0.5 * base);
    const MomentPerWidth overturning = water * (height * (1.0 / 3.0));
    CHECK_THAT(resisting.n_m_per_m, WithinRel(2937500.0, 1e-9));
    CHECK_THAT(overturning.n_m_per_m, WithinRel(1635000.0, 1e-9));

    const Length x_r = (resisting - overturning) / weight;
    CHECK_THAT(x_r.meters, WithinRel(1.1085106382978724, 1e-9));
}
