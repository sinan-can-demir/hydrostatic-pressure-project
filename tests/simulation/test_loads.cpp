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
