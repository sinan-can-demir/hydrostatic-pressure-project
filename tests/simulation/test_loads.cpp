#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include "math/Point2.hpp"
#include "model/Scenario.hpp"
#include "simulation/ForceContribution.hpp"
#include "simulation/Load.hpp"
#include "simulation/PressureDistribution.hpp"
#include "simulation/SelfWeight.hpp"
#include "simulation/TailwaterLoad.hpp"
#include "simulation/UpliftLoad.hpp"
#include "simulation/WaterLoad.hpp"
#include "units/Length.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace units;
using math::Point2;
using model::DamSection;
using model::Scenario;
using simulation::ForceContribution;

namespace {

Point2 P(double x, double y) { return Point2{Length(x), Length(y)}; }

Scenario scenario(std::vector<Point2> vertices, double hw) {
    return Scenario{.section = DamSection{std::move(vertices)},
                    .water_level = model::WaterLevel{Length(hw)},
                    .foundation = model::Foundation{Angle::from_degrees(35.0)}};
}

Scenario ac1(double hw = 10.0) { return scenario({P(0, 0), P(5, 0), P(5, 10), P(0, 10)}, hw); }
Scenario ac3(double hw = 10.0) { return scenario({P(0, 0), P(5, 0), P(5, 10), P(2, 10)}, hw); }

const ForceContribution* find(const std::vector<ForceContribution>& all, const char* source) {
    for (const auto& c : all) {
        if (c.source == source) return &c;
    }
    return nullptr;
}

} // namespace

TEST_CASE("L-01 WaterLoad on a vertical face, Hw at the crest (AC-1)", "[FR-06][L-01]") {
    const auto all = simulation::WaterLoad{}.contributions(ac1());
    const auto* h = find(all, "water (horizontal)");
    REQUIRE(h != nullptr);
    CHECK_THAT(h->fx.n_per_m, WithinRel(490500.0, 1e-12));
    CHECK(h->fy.n_per_m == 0.0);
    CHECK_THAT(h->point.y.meters, WithinRel(10.0 / 3.0, 1e-12));
    CHECK_THAT(h->point.x.meters, WithinAbs(0.0, 1e-12));
    // A vertical face carries no vertical water force, so none is reported.
    CHECK(find(all, "water (vertical)") == nullptr);
}

TEST_CASE("L-01 WaterLoad with a partial level (AC-4)", "[FR-06][L-01]") {
    const auto all = simulation::WaterLoad{}.contributions(ac1(6.0));
    const auto* h = find(all, "water (horizontal)");
    REQUIRE(h != nullptr);
    CHECK_THAT(h->fx.n_per_m, WithinRel(176580.0, 1e-12));
    CHECK_THAT(h->point.y.meters, WithinRel(2.0, 1e-12));
}

TEST_CASE("L-01 empty reservoir yields no water force (AC-6)", "[FR-04][L-01]") {
    CHECK(simulation::WaterLoad{}.contributions(ac1(0.0)).empty());
}

TEST_CASE("L-02 WaterLoad on an inclined face (AC-3)", "[FR-06][L-02]") {
    const auto all = simulation::WaterLoad{}.contributions(ac3());
    const auto* h = find(all, "water (horizontal)");
    const auto* v = find(all, "water (vertical)");
    REQUIRE(h != nullptr);
    REQUIRE(v != nullptr);
    CHECK_THAT(h->fx.n_per_m, WithinRel(490500.0, 1e-12));
    CHECK_THAT(h->point.y.meters, WithinRel(10.0 / 3.0, 1e-12));
    // Weight of the water wedge above the face: 10 m2 * 9810 N/m3, acting downward.
    CHECK_THAT(v->fy.n_per_m, WithinRel(-98100.0, 1e-12));
    CHECK(v->fx.n_per_m == 0.0);
    CHECK_THAT(v->point.x.meters, WithinRel(2.0 / 3.0, 1e-12));
}

TEST_CASE("L-02 water above the face is clipped at the water level", "[FR-06][L-02]") {
    // Inclined face (x = 0.2 y), Hw = 5: the pressure triangle gives Fx = 0.5 * 9810 * 5^2 and
    // the wedge of water above the wetted face gives Fy = 9810 * 0.2 * 5^2 / 2 downward.
    const auto all = simulation::WaterLoad{}.contributions(ac3(5.0));
    CHECK_THAT(find(all, "water (horizontal)")->fx.n_per_m, WithinRel(122625.0, 1e-12));
    CHECK_THAT(find(all, "water (vertical)")->fy.n_per_m, WithinRel(-24525.0, 1e-12));
}

TEST_CASE("WaterLoad honours the fluid density and gravity", "[FR-03][L-01]") {
    Scenario s = ac1();
    s.fluid.density = Density(1025.0);
    const auto all = simulation::WaterLoad{}.contributions(s);
    CHECK_THAT(find(all, "water (horizontal)")->fx.n_per_m, WithinRel(0.5 * 1025.0 * 9.81 * 100.0, 1e-12));
}

TEST_CASE("L-03 SelfWeight: W and centroid", "[FR-07][L-03]") {
    SECTION("AC-1 rectangle") {
        const auto all = simulation::SelfWeight{}.contributions(ac1());
        REQUIRE(all.size() == 1);
        CHECK(all[0].source == "self weight");
        CHECK_THAT(all[0].fy.n_per_m, WithinRel(-1175000.0, 1e-12));
        CHECK(all[0].fx.n_per_m == 0.0);
        CHECK_THAT(all[0].point.x.meters, WithinRel(2.5, 1e-12));
        CHECK_THAT(all[0].point.y.meters, WithinRel(5.0, 1e-12));
    }
    SECTION("AC-2 triangle: W at B/3 from the heel") {
        const auto all = simulation::SelfWeight{}.contributions(scenario({P(0, 0), P(7, 0), P(0, 10)}, 10.0));
        CHECK_THAT(all[0].fy.n_per_m, WithinRel(-822500.0, 1e-12));
        CHECK_THAT(all[0].point.x.meters, WithinRel(7.0 / 3.0, 1e-12));
    }
    SECTION("AC-3 trapezoid") {
        const auto all = simulation::SelfWeight{}.contributions(ac3());
        CHECK_THAT(all[0].fy.n_per_m, WithinRel(-940000.0, 1e-12));
    }
}

TEST_CASE("FR-05 pressure samples at the face vertices plus the water surface", "[FR-05][L-01]") {
    const auto d = simulation::WaterLoad{}.pressure(ac1(6.0));
    REQUIRE(d.samples.size() == 2);
    CHECK_THAT(d.samples[0].pressure.pascals, WithinRel(9810.0 * 6.0, 1e-12));
    CHECK_THAT(d.samples[0].position.y.meters, WithinAbs(0.0, 1e-12));
    CHECK_THAT(d.samples[1].position.y.meters, WithinRel(6.0, 1e-12));
    CHECK_THAT(d.samples[1].pressure.pascals, WithinAbs(0.0, 1e-9));
    // Unit normal pointing into the dam (downstream, +x, for a vertical upstream face).
    CHECK_THAT(d.samples[0].normal.x.meters, WithinRel(1.0, 1e-12));
    CHECK_THAT(d.samples[0].normal.y.meters, WithinAbs(0.0, 1e-12));
}

TEST_CASE("FR-05 base pressure equals rho g Hw for a vertical face", "[FR-05]") {
    const auto d = simulation::WaterLoad{}.pressure(ac1());
    CHECK_THAT(d.samples.front().pressure.pascals, WithinRel(98100.0, 1e-12));
}

TEST_CASE("FR-05 face vertices below the level are all sampled", "[FR-05]") {
    // Face with a kink at y = 4.
    const auto s = scenario({P(0, 0), P(8, 0), P(8, 10), P(1, 10), P(0, 4)}, 7.0);
    const auto d = simulation::WaterLoad{}.pressure(s);
    REQUIRE(d.samples.size() == 3);
    CHECK_THAT(d.samples[1].position.y.meters, WithinRel(4.0, 1e-12));
    CHECK_THAT(d.samples[2].position.y.meters, WithinRel(7.0, 1e-12));
    CHECK_THAT(d.samples[2].position.x.meters, WithinRel(1.0 * 3.0 / 6.0 + 0.0, 1e-12));
}

TEST_CASE("FR-20 configurable sample count gives evenly spaced samples", "[FR-20]") {
    const auto d = simulation::WaterLoad{}.pressure(ac1(), 5);
    REQUIRE(d.samples.size() == 5);
    for (std::size_t i = 0; i < 5; ++i) {
        const double y = 2.5 * static_cast<double>(i);
        CHECK_THAT(d.samples[i].position.y.meters, WithinAbs(y, 1e-12));
        CHECK_THAT(d.samples[i].pressure.pascals, WithinAbs(9810.0 * (10.0 - y), 1e-9));
    }
}

TEST_CASE("empty reservoir has no pressure samples", "[FR-04][FR-05]") {
    CHECK(simulation::WaterLoad{}.pressure(ac1(0.0)).samples.empty());
}

namespace {

// A load that is not part of the library: a fixed horizontal push of 1000 N/m at y = 2 m.
class StubLoad final : public simulation::Load {
public:
    std::vector<ForceContribution> contributions(const Scenario&) const override {
        return {ForceContribution{ForcePerWidth(1000.0), ForcePerWidth(0.0), P(0, 2), "stub"}};
    }
};

} // namespace

TEST_CASE("L-04 a stub Load plugs into the same interface", "[ADR-0004][L-04]") {
    std::vector<std::unique_ptr<simulation::Load>> loads;
    loads.push_back(std::make_unique<simulation::WaterLoad>());
    loads.push_back(std::make_unique<simulation::SelfWeight>());
    loads.push_back(std::make_unique<StubLoad>());

    std::vector<ForceContribution> all;
    for (const auto& load : loads) {
        const auto part = load->contributions(ac1());
        all.insert(all.end(), part.begin(), part.end());
    }
    REQUIRE(all.size() == 3);
    CHECK(all[0].source == "water (horizontal)");
    CHECK(all[1].source == "self weight");
    CHECK(all[2].source == "stub");
}

TEST_CASE("loads are deterministic", "[NFR-03]") {
    const auto a = simulation::WaterLoad{}.contributions(ac3());
    const auto b = simulation::WaterLoad{}.contributions(ac3());
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].fx.n_per_m == b[i].fx.n_per_m);
        CHECK(a[i].fy.n_per_m == b[i].fy.n_per_m);
    }
}

// ---- TailwaterLoad (v1.1, FR-22, FR-23; reference case AU-2) ----------------------------------
//
// Independent derivations. Vertical downstream face x = 5, tailwater H2 = 3 m:
//   Fx = -1/2 * 1000 * 9.81 * 3^2 = -44 145 N/m (towards the upstream side), at 3/3 = 1 m above the base.
// Sloping downstream face (0,0) (7,0) (0,10), H2 = 3 m: the face rises from the toe (7,0) with slope
//   dx/dy = -0.7, so the wetted part ends at (4.9, 3). Water above that face fills the triangle
//   (4.9,3) (7,3) (7,0): area 1/2 * 2.1 * 3 = 3.15 m2, centroid x = (4.9 + 7 + 7)/3 = 6.3 m.
//   Fy = -9810 * 3.15 = -30 901.5 N/m (downward) acting at x = 6.3; Fx as above, at y = 1 m.

namespace {

Scenario with_tailwater(Scenario s, double depth) {
    s.tailwater_depth = Length(depth);
    return s;
}

} // namespace

TEST_CASE("TW-03 TailwaterLoad on a vertical downstream face (AU-2)", "[FR-23][TW-03]") {
    const auto all = simulation::TailwaterLoad{}.contributions(with_tailwater(ac1(), 3.0));
    REQUIRE(all.size() == 1);
    const auto* h = find(all, "tailwater (horizontal)");
    REQUIRE(h != nullptr);
    CHECK_THAT(h->fx.n_per_m, WithinRel(-44145.0, 1e-12));
    CHECK(h->fy.n_per_m == 0.0);
    CHECK_THAT(h->point.y.meters, WithinRel(1.0, 1e-12));
    CHECK_THAT(h->point.x.meters, WithinRel(5.0, 1e-12));
    // A vertical face carries no vertical water force, so none is reported.
    CHECK(find(all, "tailwater (vertical)") == nullptr);
}

TEST_CASE("TW-03 no tailwater, or tailwater of zero depth, adds nothing", "[FR-22][FR-23][NFR-12][TW-03]") {
    CHECK(simulation::TailwaterLoad{}.contributions(ac1()).empty());
    CHECK(simulation::TailwaterLoad{}.contributions(with_tailwater(ac1(), 0.0)).empty());
}

TEST_CASE("TW-03 tailwater at the full water level equals the upstream force with the sign reversed",
          "[FR-23][TW-03]") {
    const auto up = simulation::WaterLoad{}.contributions(ac1(10.0));
    const auto down = simulation::TailwaterLoad{}.contributions(with_tailwater(ac1(10.0), 10.0));
    REQUIRE(up.size() == 1);
    REQUIRE(down.size() == 1);
    CHECK_THAT(down[0].fx.n_per_m, WithinRel(-up[0].fx.n_per_m, 1e-12));
    CHECK_THAT(down[0].point.y.meters, WithinRel(up[0].point.y.meters, 1e-12));
}

TEST_CASE("TW-04 TailwaterLoad on a sloping downstream face", "[FR-23][TW-04]") {
    const Scenario triangle = with_tailwater(scenario({P(0, 0), P(7, 0), P(0, 10)}, 10.0), 3.0);
    const auto all = simulation::TailwaterLoad{}.contributions(triangle);
    REQUIRE(all.size() == 2);
    const auto* h = find(all, "tailwater (horizontal)");
    const auto* v = find(all, "tailwater (vertical)");
    REQUIRE(h != nullptr);
    REQUIRE(v != nullptr);
    CHECK_THAT(h->fx.n_per_m, WithinRel(-44145.0, 1e-12));
    CHECK_THAT(h->point.y.meters, WithinRel(1.0, 1e-12));
    CHECK_THAT(v->fy.n_per_m, WithinRel(-30901.5, 1e-12));
    CHECK(v->fx.n_per_m == 0.0);
    CHECK_THAT(v->point.x.meters, WithinRel(6.3, 1e-12));
}

TEST_CASE("TW-04 tailwater honours the fluid density and gravity", "[FR-03][TW-04]") {
    Scenario s = with_tailwater(ac1(), 3.0);
    s.fluid.density = Density(1025.0);
    s.fluid.gravity = Acceleration(9.80665);
    const auto all = simulation::TailwaterLoad{}.contributions(s);
    REQUIRE(all.size() == 1);
    CHECK_THAT(all[0].fx.n_per_m, WithinRel(-0.5 * 1025.0 * 9.80665 * 9.0, 1e-12));
}

TEST_CASE("TW-03 the TailwaterLoad is deterministic", "[NFR-03][TW-03]") {
    const Scenario s = with_tailwater(scenario({P(0, 0), P(7, 0), P(0, 10)}, 10.0), 3.0);
    CHECK(simulation::TailwaterLoad{}.contributions(s) == simulation::TailwaterLoad{}.contributions(s));
}

// ---- UpliftLoad (v1.1, FR-24; AC-1 dam, B = 5, H1 = 10, rho g = 9810) -----------------------------
//
// Hand values (docs/09 section 6, checked by an independent integration script). The uplift is a
// single upward force at the centroid of the pressure diagram; its moment about the toe (x = 5) is
// fy * (x - 5). Heads are in metres of water; areas below are head * length.

namespace {

Scenario with_uplift(Scenario s, std::optional<model::Drains> drains = std::nullopt) {
    s.uplift = model::Uplift{drains};
    return s;
}

model::Drains drains(double e, double x, double h4 = 0.0) { return model::Drains{e, Length(x), Length(h4)}; }

double toe_moment(const ForceContribution& c) { return c.fy.n_per_m * (c.point.x.meters - 5.0); }

const ForceContribution& uplift_of(const std::vector<ForceContribution>& all) {
    const ForceContribution* u = find(all, "uplift");
    REQUIRE(u != nullptr);
    return *u;
}

} // namespace

TEST_CASE("UP-02 UpliftLoad emits nothing unless uplift is requested", "[FR-24][NFR-12][UP-02]") {
    CHECK(simulation::UpliftLoad{}.contributions(ac1()).empty());
    Scenario tail_only = ac1();
    tail_only.tailwater_depth = Length(3.0);
    CHECK(simulation::UpliftLoad{}.contributions(tail_only).empty());
}

TEST_CASE("UP-02 AU-1 no drains, no tailwater", "[FR-24][UP-02]") {
    const auto all = simulation::UpliftLoad{}.contributions(with_uplift(ac1()));
    REQUIRE(all.size() == 1);
    const ForceContribution& u = uplift_of(all);
    CHECK_THAT(u.fy.n_per_m, WithinRel(245250.0, 1e-12)); // 9810 * 5 * (10 + 0) / 2, upward
    CHECK(u.fx.n_per_m == 0.0);
    CHECK_THAT(toe_moment(u), WithinRel(-817500.0, 1e-12));
    CHECK_THAT(u.point.y.meters, WithinAbs(0.0, 1e-12));
}

TEST_CASE("UP-02 AU-2 tailwater changes only the toe ordinate", "[FR-24][UP-02]") {
    Scenario s = with_uplift(ac1());
    s.tailwater_depth = Length(3.0);
    const ForceContribution u = uplift_of(simulation::UpliftLoad{}.contributions(s));
    CHECK_THAT(u.fy.n_per_m, WithinRel(318825.0, 1e-12)); // 9810 * 5 * (10 + 3) / 2
    CHECK_THAT(toe_moment(u), WithinRel(-940125.0, 1e-12));
}

TEST_CASE("UP-03 AU-3 drains E = 0.5 at X = 1 m", "[FR-24][FR-25][UP-03]") {
    // H3 = 0.5 * 10 * 4 / 5 = 4: heads 10 -> 4 over 1 m (area 7) and 4 -> 0 over 4 m (area 8).
    const ForceContribution u =
        uplift_of(simulation::UpliftLoad{}.contributions(with_uplift(ac1(), drains(0.5, 1.0))));
    CHECK_THAT(u.fy.n_per_m, WithinRel(147150.0, 1e-12));
    CHECK_THAT(toe_moment(u), WithinRel(-523200.0, 1e-12));
}

TEST_CASE("UP-03 gallery above the tailwater raises the drain-line head", "[FR-24][UP-03]") {
    // H4 = 6 > H2 = 0, so H' = 6 and H3 = 0.5 * (10 - 6) * 4 / 5 + 6 = 7.6:
    // 10 -> 7.6 over 1 m (8.8) and 7.6 -> 0 over 4 m (15.2); moment about heel 119/3 head-m^2.
    const ForceContribution u =
        uplift_of(simulation::UpliftLoad{}.contributions(with_uplift(ac1(), drains(0.5, 1.0, 6.0))));
    CHECK_THAT(u.fy.n_per_m, WithinRel(9810.0 * 24.0, 1e-12));
    CHECK_THAT(toe_moment(u), WithinRel(9810.0 * (119.0 / 3.0 - 120.0), 1e-12));
}

TEST_CASE("UP-03 gallery below the tailwater leaves the tailwater as the floor", "[FR-24][UP-03]") {
    // H2 = 3, H4 = 1 < H2, so H' = 3 and H3 = 0.5 * 7 * 4 / 5 + 3 = 5.8:
    // 10 -> 5.8 over 1 m (7.9) and 5.8 -> 3 over 4 m (17.6): 25.5 head-m.
    Scenario s = with_uplift(ac1(), drains(0.5, 1.0, 1.0));
    s.tailwater_depth = Length(3.0);
    const ForceContribution u = uplift_of(simulation::UpliftLoad{}.contributions(s));
    CHECK_THAT(u.fy.n_per_m, WithinRel(9810.0 * 25.5, 1e-12));
}

TEST_CASE("UP-04 AU-5 drains within 0.05 H1 of the heel use one straight line", "[FR-24][UP-04]") {
    // X = 0.2 <= 0.05 * 10: H3 = 0.5 * 10 = 5 at the heel, straight to 0 at the toe (12.5 head-m).
    const ForceContribution u =
        uplift_of(simulation::UpliftLoad{}.contributions(with_uplift(ac1(), drains(0.5, 0.2))));
    CHECK_THAT(u.fy.n_per_m, WithinRel(122625.0, 1e-12));
    CHECK_THAT(toe_moment(u), WithinRel(-408750.0, 1e-12));
}

TEST_CASE("UP-04 the threshold X = 0.05 H1 selects the two rules", "[FR-24][UP-04]") {
    // X = 0.5 is exactly 0.05 * 10: single line (same as AU-5). X = 0.51 is bilinear:
    // H3 = 4.49; areas 3.69495 + 10.08005 = 13.775 head-m.
    const ForceContribution at =
        uplift_of(simulation::UpliftLoad{}.contributions(with_uplift(ac1(), drains(0.5, 0.5))));
    CHECK_THAT(at.fy.n_per_m, WithinRel(122625.0, 1e-12));
    const ForceContribution above =
        uplift_of(simulation::UpliftLoad{}.contributions(with_uplift(ac1(), drains(0.5, 0.51))));
    CHECK_THAT(above.fy.n_per_m, WithinRel(9810.0 * 13.775, 1e-12));
}

TEST_CASE("UP-03 a toe drain (X = B) and an effectiveness of 1 are handled", "[FR-24][UP-03]") {
    // X = B: H3 = H' = 0 at the toe, so the diagram is the undrained triangle in two parts.
    const ForceContribution toe =
        uplift_of(simulation::UpliftLoad{}.contributions(with_uplift(ac1(), drains(0.5, 5.0))));
    CHECK_THAT(toe.fy.n_per_m, WithinRel(9810.0 * (10.0 + 0.0) / 2.0 * 5.0, 1e-12));
    // E = 1: the drain line is at H', so the heel-to-drain part is 10 -> 0 over 1 m (5), then 0.
    const ForceContribution full =
        uplift_of(simulation::UpliftLoad{}.contributions(with_uplift(ac1(), drains(1.0, 1.0))));
    CHECK_THAT(full.fy.n_per_m, WithinRel(9810.0 * 5.0, 1e-12));
}

TEST_CASE("UP-03 the UpliftLoad is deterministic and uses the fluid properties", "[NFR-03][UP-03]") {
    Scenario s = with_uplift(ac1(), drains(0.5, 1.0));
    CHECK(simulation::UpliftLoad{}.contributions(s) == simulation::UpliftLoad{}.contributions(s));
    s.fluid.density = Density(1025.0);
    s.fluid.gravity = Acceleration(9.80665);
    const ForceContribution u = uplift_of(simulation::UpliftLoad{}.contributions(s));
    CHECK_THAT(u.fy.n_per_m, WithinRel(1025.0 * 9.80665 * 15.0, 1e-12));
}
