#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <memory>
#include <vector>

#include "math/Point2.hpp"
#include "model/Scenario.hpp"
#include "simulation/Analyzer.hpp"
#include "simulation/Checks.hpp"
#include "simulation/Resultant.hpp"
#include "simulation/SelfWeight.hpp"
#include "simulation/WaterLoad.hpp"
#include "units/Angle.hpp"
#include "units/Length.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace units;
using math::Point2;
using model::DamSection;
using model::LoadCondition;
using model::Scenario;
using namespace simulation;

namespace {

Point2 P(double x, double y) { return Point2{Length(x), Length(y)}; }

Scenario scenario(std::vector<Point2> vertices, double hw, LoadCondition c = LoadCondition::Usual) {
    return Scenario{.section = DamSection{std::move(vertices)},
                    .water_level = model::WaterLevel{Length(hw)},
                    .foundation = model::Foundation{Angle::from_degrees(35.0)},
                    .condition = c};
}

Scenario ac1(double hw = 10.0, LoadCondition c = LoadCondition::Usual) {
    return scenario({P(0, 0), P(5, 0), P(5, 10), P(0, 10)}, hw, c);
}
Scenario ac2() { return scenario({P(0, 0), P(7, 0), P(0, 10)}, 10.0); }
Scenario ac5() { return scenario({P(0, 0), P(1, 0), P(1, 10), P(0, 10)}, 10.0); }

Result run(const Scenario& s) {
    auto r = analyze(s);
    REQUIRE(r.has_value());
    return *r;
}

const double kTan35 = std::tan(35.0 * std::numbers::pi / 180.0);

} // namespace

// ---- R-01 resultant ------------------------------------------------------------------------

TEST_CASE("R-01 AC-1 resultant: V, H, M, x_R, e", "[FR-08][R-01]") {
    const Result r = run(ac1());
    const double m_res = 1175000.0 * 2.5;
    const double m_over = 490500.0 * 10.0 / 3.0;
    CHECK_THAT(r.resultant.vertical.n_per_m, WithinRel(1175000.0, 1e-9));
    CHECK_THAT(r.resultant.horizontal.n_per_m, WithinRel(490500.0, 1e-9));
    CHECK_THAT(r.resultant.moment_about_toe.n_m_per_m, WithinRel(m_res - m_over, 1e-9));
    REQUIRE(r.resultant.x_from_toe.has_value());
    CHECK_THAT(r.resultant.x_from_toe->meters, WithinRel(1.10851, 1e-5));
    CHECK_THAT(r.resultant.eccentricity->meters, WithinRel(1.39149, 1e-5));
}

TEST_CASE("R-01 AC-2 resultant", "[FR-08][R-01]") {
    const Result r = run(ac2());
    CHECK_THAT(r.resultant.vertical.n_per_m, WithinRel(822500.0, 1e-9));
    CHECK_THAT(r.resultant.x_from_toe->meters, WithinRel(2.67882, 1e-5));
    CHECK_THAT(r.resultant.eccentricity->meters, WithinRel(0.82118, 1e-5));
}

TEST_CASE("R-01 AC-4 partial level", "[FR-08][R-01]") {
    const Result r = run(ac1(6.0));
    CHECK_THAT(r.resultant.x_from_toe->meters, WithinRel(2.19944, 1e-5));
}

TEST_CASE("R-01 AC-5 resultant lies beyond the heel", "[FR-08][R-01]") {
    const Result r = run(ac5());
    CHECK_THAT(r.resultant.vertical.n_per_m, WithinRel(235000.0, 1e-9));
    CHECK_THAT(r.resultant.x_from_toe->meters, WithinRel(-6.45745, 1e-5));
}

TEST_CASE("R-01 AC-6 empty reservoir", "[FR-08][R-01]") {
    const Result r = run(ac1(0.0));
    CHECK(r.resultant.horizontal.n_per_m == 0.0);
    CHECK_THAT(r.resultant.x_from_toe->meters, WithinRel(2.5, 1e-12));
}

// ---- R-02 contact --------------------------------------------------------------------------

TEST_CASE("R-02 full contact (AC-2)", "[FR-09][R-02]") {
    const Contact& c = run(ac2()).contact;
    CHECK(c.state == ContactState::Full);
    CHECK_THAT(c.length.meters, WithinRel(7.0, 1e-12));
    CHECK_THAT(c.toe_pressure.pascals, WithinRel(200204.0, 1e-5));
    CHECK_THAT(c.heel_pressure.pascals, WithinRel(34796.0, 1e-4));
}

TEST_CASE("R-02 partial contact near the toe (AC-1)", "[FR-09][R-02]") {
    const Contact& c = run(ac1()).contact;
    CHECK(c.state == ContactState::Partial);
    CHECK_THAT(c.length.meters, WithinRel(3.32553, 1e-5));
    CHECK_THAT(c.toe_pressure.pascals, WithinRel(706650.0, 1e-4));
    CHECK(c.heel_pressure.pascals == 0.0);
}

TEST_CASE("R-02 partial contact near the heel (mirrored)", "[FR-09][R-02]") {
    // Weight only, acting 0.5 m from the heel of a 5 m base: x_R = 4.5 m from the toe.
    const DamSection section{{P(0, 0), P(5, 0), P(5, 10), P(0, 10)}};
    const std::vector<ForceContribution> contributions{
        ForceContribution{ForcePerWidth(0.0), ForcePerWidth(-1000.0), P(0.5, 5), "w"}};
    const Resultant res = make_resultant(contributions, section);
    const Contact c = make_contact(res);
    CHECK(c.state == ContactState::Partial);
    CHECK_THAT(c.length.meters, WithinRel(1.5, 1e-12));
    CHECK_THAT(c.heel_pressure.pascals, WithinRel(2.0 * 1000.0 / 1.5, 1e-12));
    CHECK(c.toe_pressure.pascals == 0.0);
}

TEST_CASE("R-02 no valid contact when the resultant is outside the base (AC-5)", "[FR-09][R-02]") {
    const Contact& c = run(ac5()).contact;
    CHECK(c.state == ContactState::None);
    CHECK(c.length.meters == 0.0);
}

TEST_CASE("R-02 AC-6 uniform contact V/B", "[FR-09][R-02]") {
    const Contact& c = run(ac1(0.0)).contact;
    CHECK(c.state == ContactState::Full);
    CHECK_THAT(c.toe_pressure.pascals, WithinRel(235000.0, 1e-12));
    CHECK_THAT(c.heel_pressure.pascals, WithinRel(235000.0, 1e-12));
}

TEST_CASE("R-02 contact pressures integrate to V", "[FR-09][R-02][P-05]") {
    for (const Scenario& s : {ac1(), ac2(), ac1(6.0), ac1(0.0)}) {
        const Result r = run(s);
        const double force = 0.5 * (r.contact.toe_pressure.pascals + r.contact.heel_pressure.pascals) *
                             r.contact.length.meters;
        CHECK_THAT(force, WithinRel(r.resultant.vertical.n_per_m, 1e-9));
    }
}

TEST_CASE("R-02 a net upward load leaves no contact", "[FR-09][R-02]") {
    const DamSection section{{P(0, 0), P(5, 0), P(5, 10), P(0, 10)}};
    const std::vector<ForceContribution> contributions{
        ForceContribution{ForcePerWidth(0.0), ForcePerWidth(500.0), P(2.5, 5), "lift"}};
    const Resultant res = make_resultant(contributions, section);
    CHECK_FALSE(res.x_from_toe.has_value());
    CHECK(make_contact(res).state == ContactState::None);
}

// ---- R-03 overturning report ---------------------------------------------------------------

TEST_CASE("R-03 AC-1 overturning report lists every term", "[FR-11][R-03]") {
    const OverturningReport& o = run(ac1()).overturning;
    REQUIRE(o.terms.size() == 2);
    CHECK(o.terms[0].source == "water (horizontal)");
    CHECK(o.terms[0].role == MomentRole::Overturning);
    CHECK_THAT(o.terms[0].force.n_per_m, WithinRel(490500.0, 1e-12));
    CHECK_THAT(o.terms[0].arm.meters, WithinRel(10.0 / 3.0, 1e-12));
    CHECK_THAT(o.terms[0].moment.n_m_per_m, WithinRel(-1635000.0, 1e-12));
    CHECK(o.terms[1].source == "self weight");
    CHECK(o.terms[1].role == MomentRole::Resisting);
    CHECK_THAT(o.terms[1].arm.meters, WithinRel(2.5, 1e-12));
    CHECK_THAT(o.terms[1].moment.n_m_per_m, WithinRel(2937500.0, 1e-12));
    CHECK_THAT(o.resisting.n_m_per_m, WithinRel(2937500.0, 1e-12));
    CHECK_THAT(o.overturning.n_m_per_m, WithinRel(1635000.0, 1e-12));
    REQUIRE(o.ratio.has_value());
    CHECK_THAT(*o.ratio, WithinRel(1.7966, 1e-4));
}

TEST_CASE("R-03 AC-2 ratio", "[FR-11][R-03]") {
    const OverturningReport& o = run(ac2()).overturning;
    CHECK_THAT(o.resisting.n_m_per_m, WithinRel(3838333.0, 1e-6));
    CHECK_THAT(*o.ratio, WithinRel(2.3476, 1e-4));
}

TEST_CASE("R-03 the vertical water component on an inclined face is its own resisting term",
          "[FR-11][R-03]") {
    const OverturningReport& o =
        run(scenario({P(0, 0), P(5, 0), P(5, 10), P(2, 10)}, 10.0)).overturning;
    REQUIRE(o.terms.size() == 3);
    CHECK(o.terms[1].source == "water (vertical)");
    CHECK(o.terms[1].direction == ForceDirection::Vertical);
    CHECK(o.terms[1].role == MomentRole::Resisting);
    CHECK_THAT(o.terms[1].arm.meters, WithinRel(5.0 - 2.0 / 3.0, 1e-12));
}

TEST_CASE("R-03 totals match the resultant", "[FR-11][R-03]") {
    for (const Scenario& s : {ac1(), ac2(), ac1(6.0), ac5()}) {
        const Result r = run(s);
        CHECK_THAT(r.overturning.resisting.n_m_per_m - r.overturning.overturning.n_m_per_m,
                   WithinRel(r.resultant.moment_about_toe.n_m_per_m, 1e-9));
    }
}

TEST_CASE("R-03 no overturning moment gives an unbounded ratio (AC-6)", "[FR-11][R-03]") {
    const OverturningReport& o = run(ac1(0.0)).overturning;
    CHECK(o.overturning.n_m_per_m == 0.0);
    CHECK_FALSE(o.ratio.has_value());
}

// ---- C-01 sliding --------------------------------------------------------------------------

TEST_CASE("C-01 sliding factor of safety", "[FR-10][C-01]") {
    CHECK_THAT(*run(ac1()).sliding.factor_of_safety, WithinRel(1.6774, 1e-4));
    CHECK_THAT(*run(ac2()).sliding.factor_of_safety, WithinRel(1.17415, 1e-5));
    CHECK_THAT(*run(ac1(6.0)).sliding.factor_of_safety, WithinRel(4.6593, 1e-4));
    CHECK_THAT(*run(ac1()).sliding.factor_of_safety, WithinRel(1175000.0 * kTan35 / 490500.0, 1e-12));
}

TEST_CASE("C-01 H = 0 reports no driving shear instead of dividing by zero", "[FR-10][C-01]") {
    CHECK_FALSE(run(ac1(0.0)).sliding.factor_of_safety.has_value());
}

// ---- C-02 .. C-05 checks -------------------------------------------------------------------

TEST_CASE("C-02 AC-1 usual: location and sliding fail, overall FAIL", "[FR-12][C-02]") {
    const Result r = run(ac1(10.0, LoadCondition::Usual));
    CHECK(r.checks.location.status == CheckStatus::Fail);
    CHECK(r.checks.sliding.status == CheckStatus::Fail);
    CHECK(r.verdict == Verdict::Fail);
}

TEST_CASE("C-02 AC-1 extreme: location and sliding pass", "[FR-12][C-02]") {
    const Result r = run(ac1(10.0, LoadCondition::Extreme));
    CHECK(r.checks.location.status == CheckStatus::Pass);
    CHECK(r.checks.sliding.status == CheckStatus::Pass);
    CHECK(r.verdict == Verdict::Pass);
}

TEST_CASE("C-02 AC-2 usual: location passes, sliding fails", "[FR-12][C-02]") {
    const Result r = run(ac2());
    CHECK(r.checks.location.status == CheckStatus::Pass);
    CHECK(r.checks.sliding.status == CheckStatus::Fail);
    CHECK(r.verdict == Verdict::Fail);
}

TEST_CASE("C-02 AC-4 usual: all checks pass", "[FR-12][C-02]") {
    const Result r = run(ac1(6.0));
    CHECK(r.checks.location.status == CheckStatus::Pass);
    CHECK(r.checks.sliding.status == CheckStatus::Pass);
    CHECK(r.verdict == Verdict::Pass);
}

TEST_CASE("C-02 AC-5 fails: resultant outside the base", "[FR-12][C-02]") {
    const Result r = run(ac5());
    CHECK(r.checks.location.status == CheckStatus::Fail);
    CHECK(r.verdict == Verdict::Fail);
}

TEST_CASE("C-02 AC-6 passes with no driving shear", "[FR-12][C-02]") {
    const Result r = run(ac1(0.0));
    CHECK(r.checks.sliding.status == CheckStatus::Pass);
    CHECK(r.verdict == Verdict::Pass);
}

TEST_CASE("C-02 location boundaries flip exactly at the threshold", "[FR-12][C-02]") {
    const Length base(5.0);
    const Contact full{ContactState::Full, base, Pressure(1.0), Pressure(1.0)};
    auto resultant_at = [&](double x) {
        return Resultant{ForcePerWidth(1.0), ForcePerWidth(0.0), MomentPerWidth(x), Length(x),
                         Length(2.5 - x), base};
    };
    const double third = 5.0 / 3.0;
    const double two_thirds = 2.0 * 5.0 / 3.0;
    using model::ResultantZone;
    CHECK(check_location(resultant_at(third), full, ResultantZone::MiddleThird).status == CheckStatus::Pass);
    CHECK(check_location(resultant_at(two_thirds), full, ResultantZone::MiddleThird).status == CheckStatus::Pass);
    CHECK(check_location(resultant_at(std::nextafter(third, 0.0)), full, ResultantZone::MiddleThird).status ==
          CheckStatus::Fail);
    CHECK(check_location(resultant_at(std::nextafter(two_thirds, 5.0)), full, ResultantZone::MiddleThird).status ==
          CheckStatus::Fail);
    CHECK(check_location(resultant_at(1.25), full, ResultantZone::MiddleHalf).status == CheckStatus::Pass);
    CHECK(check_location(resultant_at(std::nextafter(1.25, 0.0)), full, ResultantZone::MiddleHalf).status ==
          CheckStatus::Fail);
    CHECK(check_location(resultant_at(3.75), full, ResultantZone::MiddleHalf).status == CheckStatus::Pass);
}

TEST_CASE("C-02 sliding boundary flips exactly at the minimum", "[FR-12][C-02]") {
    CHECK(check_sliding(SlidingResult{2.0}, 2.0).status == CheckStatus::Pass);
    CHECK(check_sliding(SlidingResult{std::nextafter(2.0, 0.0)}, 2.0).status == CheckStatus::Fail);
    CHECK(check_sliding(SlidingResult{std::nullopt}, 2.0).status == CheckStatus::Pass);
}

TEST_CASE("C-03 a user override of the criteria replaces the preset", "[FR-13][C-03]") {
    Scenario s = ac1();
    s.criteria.usual.min_sliding_fs = 1.5;  // AC-1 FS is 1.6774
    CHECK(run(s).checks.sliding.status == CheckStatus::Pass);
    s.criteria.usual.min_sliding_fs = 1.7;
    CHECK(run(s).checks.sliding.status == CheckStatus::Fail);
    CHECK(run(s).checks.sliding.minimum == 1.7);
}

TEST_CASE("C-04 bearing check: pass, fail and skipped", "[FR-14][C-04]") {
    const Contact partial{ContactState::Partial, Length(3.0), Pressure(706650.0), Pressure(0.0)};
    CHECK(check_bearing(partial, Pressure(1.0e6)).status == CheckStatus::Pass);
    CHECK(check_bearing(partial, Pressure(706650.0)).status == CheckStatus::Pass);
    CHECK(check_bearing(partial, Pressure(5.0e5)).status == CheckStatus::Fail);
    CHECK(check_bearing(partial, std::nullopt).status == CheckStatus::Skipped);
    const Contact none{ContactState::None, Length(0.0), Pressure(0.0), Pressure(0.0)};
    CHECK(check_bearing(none, Pressure(1.0e6)).status == CheckStatus::Fail);
}

TEST_CASE("C-04 bearing through analyze", "[FR-14][C-04]") {
    Scenario s = ac1(6.0);
    CHECK(run(s).checks.bearing.status == CheckStatus::Skipped);
    s.foundation.allowable_bearing = Pressure(1.0e5);
    CHECK(run(s).checks.bearing.status == CheckStatus::Fail);
    s.foundation.allowable_bearing = Pressure(1.0e7);
    CHECK(run(s).checks.bearing.status == CheckStatus::Pass);
}

TEST_CASE("C-05 verdict: any failed check fails, skipped checks do not", "[FR-12][C-05]") {
    const LocationCheck pass_loc{CheckStatus::Pass, model::ResultantZone::MiddleThird, Length(0), Length(1)};
    const SlidingCheck pass_slide{CheckStatus::Pass, 3.0, 2.0};
    const BearingCheck skipped{CheckStatus::Skipped, Pressure(0.0), std::nullopt};
    CHECK(make_verdict(Checks{pass_loc, pass_slide, skipped}) == Verdict::Pass);
    CHECK(make_verdict(Checks{pass_loc, SlidingCheck{CheckStatus::Fail, 1.0, 2.0}, skipped}) == Verdict::Fail);
    CHECK(make_verdict(Checks{pass_loc, pass_slide,
                              BearingCheck{CheckStatus::Fail, Pressure(2.0), Pressure(1.0)}}) == Verdict::Fail);
}

// ---- W-01, A-01, A-02, L-04 ----------------------------------------------------------------

TEST_CASE("W-01 the uplift warning is in every result, failed ones included", "[FR-15][W-01]") {
    for (const Scenario& s : {ac1(), ac1(0.0), ac5()}) {
        const Result r = run(s);
        REQUIRE(r.warnings.size() == 1);
        CHECK(r.warnings[0] == "uplift not modelled; results are unconservative for real dams");
    }
}

TEST_CASE("A-01 an invalid scenario returns errors and does not throw", "[FR-16][NFR-04][A-01]") {
    Scenario s = ac1(11.0);  // water above the crest
    CHECK_NOTHROW(analyze(s));
    const auto r = analyze(s);
    REQUIRE_FALSE(r.has_value());
    CHECK_FALSE(r.error().empty());
}

TEST_CASE("A-02 the same scenario gives equal results", "[FR-16][NFR-03][A-02]") {
    CHECK(run(ac1()) == run(ac1()));
    CHECK_FALSE(run(ac1()) == run(ac1(6.0)));
}

TEST_CASE("the result echoes the scenario and carries the pressure and contributions", "[NFR-09]") {
    const Result r = run(ac1());
    CHECK(r.scenario == ac1());
    CHECK(r.pressure.samples.size() == 2);
    CHECK(r.contributions.size() == 2);
}

namespace {
class StubLoad final : public Load {
public:
    std::vector<ForceContribution> contributions(const Scenario&) const override {
        return {ForceContribution{ForcePerWidth(1000.0), ForcePerWidth(0.0), P(0, 2), "stub"}};
    }
};
} // namespace

TEST_CASE("L-04 a stub Load added to the Analyzer shifts the resultant", "[ADR-0004][L-04]") {
    std::vector<std::unique_ptr<Load>> loads;
    loads.push_back(std::make_unique<WaterLoad>());
    loads.push_back(std::make_unique<SelfWeight>());
    loads.push_back(std::make_unique<StubLoad>());
    const Result with_stub = *Analyzer(std::move(loads)).run(ac1());
    const Result base = run(ac1());

    REQUIRE(with_stub.contributions.size() == 3);
    CHECK(with_stub.contributions[2].source == "stub");
    CHECK_THAT(with_stub.resultant.horizontal.n_per_m, WithinRel(490500.0 + 1000.0, 1e-12));
    CHECK_THAT(with_stub.resultant.moment_about_toe.n_m_per_m,
               WithinRel(base.resultant.moment_about_toe.n_m_per_m - 2000.0, 1e-12));
    CHECK(with_stub.overturning.terms.size() == 3);
}

// ---- properties ----------------------------------------------------------------------------

TEST_CASE("P-01 scaling all lengths by k scales forces by k^2 and leaves ratios unchanged", "[NFR-01][P-01]") {
    const double k = 3.0;
    const Result a = run(ac2());
    const Result b = run(scenario({P(0, 0), P(7 * k, 0), P(0, 10 * k)}, 10.0 * k));
    CHECK_THAT(b.resultant.horizontal.n_per_m, WithinRel(a.resultant.horizontal.n_per_m * k * k, 1e-9));
    CHECK_THAT(b.resultant.vertical.n_per_m, WithinRel(a.resultant.vertical.n_per_m * k * k, 1e-9));
    CHECK_THAT(b.resultant.x_from_toe->meters, WithinRel(a.resultant.x_from_toe->meters * k, 1e-9));
    CHECK_THAT(*b.sliding.factor_of_safety, WithinRel(*a.sliding.factor_of_safety, 1e-9));
    CHECK_THAT(*b.overturning.ratio, WithinRel(*a.overturning.ratio, 1e-9));
}

TEST_CASE("P-02 a cyclic rotation of the vertex list gives the same result", "[FR-01][P-02]") {
    const Result a = run(scenario({P(0, 0), P(5, 0), P(5, 10), P(2, 10)}, 8.0));
    const Result b = run(scenario({P(5, 10), P(2, 10), P(0, 0), P(5, 0)}, 8.0));
    CHECK_THAT(b.resultant.moment_about_toe.n_m_per_m, WithinRel(a.resultant.moment_about_toe.n_m_per_m, 1e-9));
    CHECK_THAT(b.resultant.vertical.n_per_m, WithinRel(a.resultant.vertical.n_per_m, 1e-9));
    CHECK_THAT(*b.sliding.factor_of_safety, WithinRel(*a.sliding.factor_of_safety, 1e-9));
    CHECK(b.verdict == a.verdict);
}

TEST_CASE("P-03 translating the section along x leaves the results unchanged", "[FR-08][P-03]") {
    const Result a = run(ac2());
    const Result b = run(scenario({P(100, 0), P(107, 0), P(100, 10)}, 10.0));
    CHECK_THAT(b.resultant.x_from_toe->meters, WithinRel(a.resultant.x_from_toe->meters, 1e-9));
    CHECK_THAT(b.contact.toe_pressure.pascals, WithinRel(a.contact.toe_pressure.pascals, 1e-9));
    CHECK_THAT(*b.sliding.factor_of_safety, WithinRel(*a.sliding.factor_of_safety, 1e-9));
}

TEST_CASE("P-04 a higher water level never decreases the horizontal force or overturning moment",
          "[FR-06][P-04]") {
    double last_h = -1.0;
    double last_m = -1.0;
    for (double hw = 0.0; hw <= 10.0; hw += 0.5) {
        const Result r = run(scenario({P(0, 0), P(5, 0), P(5, 10), P(2, 10)}, hw));
        CHECK(r.resultant.horizontal.n_per_m >= last_h);
        CHECK(r.overturning.overturning.n_m_per_m >= last_m);
        last_h = r.resultant.horizontal.n_per_m;
        last_m = r.overturning.overturning.n_m_per_m;
    }
}

TEST_CASE("I-05 pressure_samples gives that many evenly spaced samples in the result", "[FR-20][I-05]") {
    Scenario s = ac1();
    CHECK(run(s).pressure.samples.size() == 2);
    s.pressure_samples = 11;
    const Result r = run(s);
    REQUIRE(r.pressure.samples.size() == 11);
    CHECK_THAT(r.pressure.samples[3].position.y.meters, WithinRel(3.0, 1e-12));
    CHECK(r.scenario.pressure_samples == 11);
}
