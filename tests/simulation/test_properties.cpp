// P-05 (random valid sections) and N-01 (bitwise determinism). The random sections come from a
// fixed seed, so a failure reproduces. Each check uses an oracle that is computed differently
// from the production code:
//   Fx          = 1/2 rho g Hw^2 for any face shape (pressure on the vertical projection)
//   Fy          = rho g * area between the wetted face and the vertical through the heel
//   weight      = gamma * shoelace area
//   contact     = Simpson integration of the pressure diagram for force and moment
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include "math/Point2.hpp"
#include "model/Scenario.hpp"
#include "simulation/Analyzer.hpp"
#include "simulation/Validator.hpp"
#include "units/Angle.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace units;
using math::Point2;
using model::Scenario;
using namespace simulation;

namespace {

Point2 P(double x, double y) { return Point2{Length(x), Length(y)}; }

// A uniform double in [0, 1) that does not depend on the standard library's distributions.
class Random {
public:
    explicit Random(std::uint64_t seed) : engine_(seed) {}
    double unit() { return static_cast<double>(engine_() >> 11) * 0x1.0p-53; }
    double in(double lo, double hi) { return lo + (hi - lo) * unit(); }

private:
    std::mt19937_64 engine_;
};

// A counter-clockwise gravity-dam section: sloping or vertical upstream face with an optional
// kink, flat crest, sloping downstream face. Not guaranteed valid; the caller validates.
Scenario random_scenario(Random& rng) {
    const double base = rng.in(2.0, 60.0);
    const double height = rng.in(2.0, 120.0);
    const double crest_upstream = rng.unit() < 0.4 ? 0.0 : rng.in(0.0, 0.4) * base;
    const double crest_downstream = crest_upstream + rng.in(0.05, 0.5) * base;

    std::vector<Point2> v{P(0, 0), P(base, 0), P(std::min(crest_downstream, base), height),
                          P(crest_upstream, height)};
    if (rng.unit() < 0.5) {
        const double yk = rng.in(0.2, 0.8) * height;
        v.push_back(P(rng.in(0.0, crest_upstream), yk));
    }

    const double pick = rng.unit();
    const double hw = pick < 0.1 ? 0.0 : pick < 0.2 ? height : rng.in(0.0, height);
    return Scenario{.section = model::DamSection{v},
                    .water_level = model::WaterLevel{Length(hw)},
                    .foundation = model::Foundation{Angle::from_degrees(rng.in(10.0, 60.0))},
                    .condition =
                        rng.unit() < 0.5 ? model::LoadCondition::Usual : model::LoadCondition::Extreme};
}

double shoelace_area(const std::vector<Point2>& v) {
    double twice = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        const Point2& a = v[i];
        const Point2& b = v[(i + 1) % v.size()];
        twice += a.x.meters * b.y.meters - b.x.meters * a.y.meters;
    }
    return twice / 2.0;
}

// Area between the wetted face and the vertical line through the heel, integrated over y with
// the trapezoid rule on each straight face segment (exact for straight segments).
double wedge_area(const Scenario& s) {
    const auto face = s.section.upstream_face();
    const double hw = s.water_level.elevation.meters;
    const double x0 = face.front().x.meters;
    double area = 0.0;
    for (std::size_t i = 0; i + 1 < face.size(); ++i) {
        double ya = face[i].y.meters, yb = face[i + 1].y.meters;
        double xa = face[i].x.meters - x0, xb = face[i + 1].x.meters - x0;
        if (ya >= hw) break;
        if (yb > hw) {
            xb = xa + (xb - xa) * (hw - ya) / (yb - ya);
            yb = hw;
        }
        area += 0.5 * (xa + xb) * (yb - ya);
    }
    return area;
}

// x of the wedge's centroid relative to the heel: integral of u^2/2 dy over the area, where u is
// the face's offset from the heel at height y (exact with Simpson, u^2 is quadratic per segment).
double wedge_centroid_offset(const Scenario& s) {
    const auto face = s.section.upstream_face();
    const double hw = s.water_level.elevation.meters;
    const double x0 = face.front().x.meters;
    double moment = 0.0;
    for (std::size_t i = 0; i + 1 < face.size(); ++i) {
        double ya = face[i].y.meters, yb = face[i + 1].y.meters;
        double ua = face[i].x.meters - x0, ub = face[i + 1].x.meters - x0;
        if (ya >= hw) break;
        if (yb > hw) {
            ub = ua + (ub - ua) * (hw - ya) / (yb - ya);
            yb = hw;
        }
        const double um = 0.5 * (ua + ub);
        moment += (yb - ya) / 6.0 * (0.5 * ua * ua + 4.0 * 0.5 * um * um + 0.5 * ub * ub);
    }
    return moment / wedge_area(s);
}

double base_of(const Scenario& s) { return s.section.base_length().meters; }

double simpson(double a, double b, auto f) { return (b - a) / 6.0 * (f(a) + 4.0 * f(0.5 * (a + b)) + f(b)); }

} // namespace

TEST_CASE("P-05 random valid sections satisfy the independent oracles", "[FR-08][FR-09][P-05]") {
    Random rng(20261031);
    int analysed = 0;
    for (int n = 0; n < 1500; ++n) {
        const Scenario s = random_scenario(rng);
        if (!validate(s).empty()) continue;
        ++analysed;
        INFO("case " << n);
        const auto r = analyze(s);
        REQUIRE(r.has_value());

        const double rho_g = s.fluid.density.kg_per_m3 * s.fluid.gravity.m_per_s2;
        const double hw = s.water_level.elevation.meters;

        double fx_water = 0.0, fy_water = 0.0, fy_weight = 0.0;
        for (const ForceContribution& c : r->contributions) {
            if (c.source == "self weight")
                fy_weight += c.fy.n_per_m;
            else {
                fx_water += c.fx.n_per_m;
                fy_water += c.fy.n_per_m;
            }
        }
        CHECK_THAT(fx_water, WithinRel(0.5 * rho_g * hw * hw, 1e-9));
        CHECK_THAT(fy_water, WithinAbs(-rho_g * wedge_area(s), 1e-9 * (1.0 + rho_g * wedge_area(s))));
        // The horizontal water force acts at Hw/3 above the base, whatever the face shape.
        for (const ForceContribution& c : r->contributions) {
            if (c.source == "water (horizontal)") CHECK_THAT(c.point.y.meters, WithinRel(hw / 3.0, 1e-9));
        }
        // The vertical water force acts through the centroid of the water above the face.
        for (const ForceContribution& c : r->contributions) {
            if (c.source != "water (vertical)") continue;
            const double x0 = s.section.heel().x.meters;
            CHECK_THAT(c.point.x.meters,
                       WithinAbs(x0 + wedge_centroid_offset(s), 1e-9 * (1.0 + std::abs(x0) + base_of(s))));
        }
        CHECK_THAT(fy_weight,
                   WithinRel(-s.concrete.unit_weight.n_per_m3 * shoelace_area(s.section.vertices), 1e-9));

        // Reported V, H, M against a fresh sum over the contributions.
        const double toe_x = s.section.toe().x.meters;
        double v = 0.0, h = 0.0, m = 0.0, scale = 0.0;
        for (const ForceContribution& c : r->contributions) {
            v -= c.fy.n_per_m;
            h += c.fx.n_per_m;
            const double mx = (c.point.x.meters - toe_x) * c.fy.n_per_m;
            const double my = -c.point.y.meters * c.fx.n_per_m;
            m += mx + my;
            scale += std::abs(mx) + std::abs(my);
        }
        CHECK_THAT(r->resultant.vertical.n_per_m, WithinRel(v, 1e-12));
        CHECK_THAT(r->resultant.horizontal.n_per_m, WithinAbs(h, 1e-9 * (1.0 + std::abs(h))));
        CHECK_THAT(r->resultant.moment_about_toe.n_m_per_m, WithinAbs(m, 1e-9 * (1.0 + scale)));
        CHECK_THAT(r->overturning.resisting.n_m_per_m - r->overturning.overturning.n_m_per_m,
                   WithinAbs(m, 1e-9 * (1.0 + scale)));

        // Contact pressures integrate to V, and their moment about the toe is V * x_R.
        const Contact& c = r->contact;
        if (c.state == ContactState::None) continue;
        const double base = r->resultant.base_length.meters;
        const double a = c.length.meters;
        double force = 0.0, moment = 0.0;
        if (c.state == ContactState::Full) {
            const double qt = c.toe_pressure.pascals, qh = c.heel_pressure.pascals;
            auto q = [&](double x) { return qt + (qh - qt) * x / base; };
            force = simpson(0.0, base, q);
            moment = simpson(0.0, base, [&](double x) { return q(x) * x; });
        } else if (c.toe_pressure.pascals > 0.0) {
            const double qt = c.toe_pressure.pascals;
            auto q = [&](double x) { return qt * (1.0 - x / a); };
            force = simpson(0.0, a, q);
            moment = simpson(0.0, a, [&](double x) { return q(x) * x; });
        } else {
            const double qh = c.heel_pressure.pascals;
            auto q = [&](double x) { return qh * (x - (base - a)) / a; };
            force = simpson(base - a, base, q);
            moment = simpson(base - a, base, [&](double x) { return q(x) * x; });
        }
        CHECK_THAT(force, WithinRel(v, 1e-9));
        CHECK_THAT(moment, WithinRel(v * r->resultant.x_from_toe->meters, 1e-9));
    }
    CHECK(analysed >= 1000); // the generator must produce enough valid sections to mean something
}

TEST_CASE("N-01 a thousand repeated runs are bitwise identical", "[NFR-03][N-01]") {
    Random rng(7);
    std::vector<Scenario> scenarios;
    while (scenarios.size() < 4) {
        const Scenario s = random_scenario(rng);
        if (validate(s).empty()) scenarios.push_back(s);
    }
    for (const Scenario& s : scenarios) {
        const auto first = analyze(s);
        REQUIRE(first.has_value());
        bool identical = true;
        for (int i = 0; i < 1000; ++i) identical = identical && (*analyze(s) == *first);
        CHECK(identical);
    }
}
