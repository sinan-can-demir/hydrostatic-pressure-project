#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

#include "math/Point2.hpp"
#include "model/Scenario.hpp"
#include "units/Length.hpp"

using Catch::Matchers::WithinRel;
using namespace units;
using math::Point2;
using model::DamSection;

namespace {

Point2 P(double x, double y) { return Point2{Length(x), Length(y)}; }

void check_point(const Point2& p, double x, double y) {
    CHECK_THAT(p.x.meters, WithinRel(x, 1e-12));
    if (y == 0.0) {
        CHECK(p.y.meters == 0.0);
    } else {
        CHECK_THAT(p.y.meters, WithinRel(y, 1e-12));
    }
}

} // namespace

TEST_CASE("AC-1 rectangle: heel, toe, crest, base, area, centroid", "[FR-01][FR-07][G-01][G-02]") {
    const DamSection s{{P(0, 0), P(5, 0), P(5, 10), P(0, 10)}};
    check_point(s.heel(), 0, 0);
    check_point(s.toe(), 5, 0);
    CHECK_THAT(s.crest_height().meters, WithinRel(10.0, 1e-12));
    CHECK_THAT(s.base_length().meters, WithinRel(5.0, 1e-12));
    CHECK_THAT(s.area().square_meters, WithinRel(50.0, 1e-12));
    check_point(s.centroid(), 2.5, 5.0);
}

TEST_CASE("AC-2 triangle with vertical upstream face", "[FR-01][FR-07][G-01][G-02]") {
    const DamSection s{{P(0, 0), P(7, 0), P(0, 10)}};
    check_point(s.heel(), 0, 0);
    check_point(s.toe(), 7, 0);
    CHECK_THAT(s.crest_height().meters, WithinRel(10.0, 1e-12));
    CHECK_THAT(s.base_length().meters, WithinRel(7.0, 1e-12));
    CHECK_THAT(s.area().square_meters, WithinRel(35.0, 1e-12));
    check_point(s.centroid(), 7.0 / 3.0, 10.0 / 3.0);
}

TEST_CASE("AC-3 trapezoid with inclined upstream face", "[FR-01][FR-07][G-01][G-02]") {
    const DamSection s{{P(0, 0), P(5, 0), P(5, 10), P(2, 10)}};
    check_point(s.heel(), 0, 0);
    check_point(s.toe(), 5, 0);
    CHECK_THAT(s.area().square_meters, WithinRel(40.0, 1e-12));
    check_point(s.centroid(), 71.0 / 24.0, 55.0 / 12.0);
}

TEST_CASE("upstream face runs from heel to crest", "[FR-05][G-02]") {
    const DamSection vertical{{P(0, 0), P(5, 0), P(5, 10), P(0, 10)}};
    const auto f1 = vertical.upstream_face();
    REQUIRE(f1.size() == 2);
    check_point(f1[0], 0, 0);
    check_point(f1[1], 0, 10);

    // Apex above the toe: the face is the sloping edge.
    const DamSection apex_at_toe{{P(0, 0), P(7, 0), P(7, 10)}};
    const auto f2 = apex_at_toe.upstream_face();
    REQUIRE(f2.size() == 2);
    check_point(f2[0], 0, 0);
    check_point(f2[1], 7, 10);

    // Inclined face: the crest vertex is the upstream-most of the two highest vertices.
    const DamSection inclined{{P(0, 0), P(5, 0), P(5, 10), P(2, 10)}};
    const auto f3 = inclined.upstream_face();
    REQUIRE(f3.size() == 2);
    check_point(f3[1], 2, 10);

    // Overhanging face leans over the water.
    const DamSection overhang{{P(0, 0), P(5, 0), P(5, 10), P(-2, 10)}};
    const auto f4 = overhang.upstream_face();
    REQUIRE(f4.size() == 2);
    check_point(f4[1], -2, 10);

    // A multi-vertex face is returned heel first.
    const DamSection broken{{P(0, 0), P(6, 0), P(6, 10), P(2, 10), P(1, 5), P(0, 3)}};
    const auto f5 = broken.upstream_face();
    REQUIRE(f5.size() == 4);
    check_point(f5[0], 0, 0);
    check_point(f5[1], 0, 3);
    check_point(f5[2], 1, 5);
    check_point(f5[3], 2, 10);
}

TEST_CASE("downstream face runs from toe to crest", "[FR-22][TW-02][G-02]") {
    const DamSection vertical{{P(0, 0), P(5, 0), P(5, 10), P(0, 10)}};
    const auto f1 = vertical.downstream_face();
    REQUIRE(f1.size() == 2);
    check_point(f1[0], 5, 0);
    check_point(f1[1], 5, 10);

    // The crest corner on the downstream face is the downstream-most of the top vertices.
    const DamSection inclined{{P(0, 0), P(5, 0), P(5, 10), P(2, 10)}};
    const auto f2 = inclined.downstream_face();
    REQUIRE(f2.size() == 2);
    check_point(f2[1], 5, 10);

    // A triangle with its apex above the heel: the whole sloping edge is the downstream face.
    const DamSection triangle{{P(0, 0), P(7, 0), P(0, 10)}};
    const auto f3 = triangle.downstream_face();
    REQUIRE(f3.size() == 2);
    check_point(f3[0], 7, 0);
    check_point(f3[1], 0, 10);

    // A stepped face keeps its corners, toe first.
    const DamSection stepped{{P(0, 0), P(8, 0), P(8, 4), P(5, 4), P(5, 10), P(0, 10)}};
    const auto f4 = stepped.downstream_face();
    REQUIRE(f4.size() == 4);
    check_point(f4[1], 8, 4);
    check_point(f4[2], 5, 4);
    check_point(f4[3], 5, 10);

    // Starting the vertex list elsewhere gives the same face.
    const DamSection shifted{{P(5, 10), P(0, 10), P(0, 0), P(5, 0)}};
    const auto f5 = shifted.downstream_face();
    REQUIRE(f5.size() == 2);
    check_point(f5[0], 5, 0);
    check_point(f5[1], 5, 10);
}

TEST_CASE("starting the vertex list elsewhere gives the same section", "[FR-01][P-02]") {
    const DamSection a{{P(0, 0), P(5, 0), P(5, 10), P(2, 10)}};
    const DamSection b{{P(5, 10), P(2, 10), P(0, 0), P(5, 0)}};
    check_point(b.heel(), 0, 0);
    check_point(b.toe(), 5, 0);
    CHECK_THAT(b.area().square_meters, WithinRel(a.area().square_meters, 1e-12));
    check_point(b.centroid(), 71.0 / 24.0, 55.0 / 12.0);
    const auto face = b.upstream_face();
    REQUIRE(face.size() == 2);
    check_point(face[0], 0, 0);
    check_point(face[1], 2, 10);
}

TEST_CASE("translating the section along x leaves lengths and areas unchanged", "[FR-08][P-03]") {
    const DamSection a{{P(0, 0), P(5, 0), P(5, 10), P(2, 10)}};
    const DamSection b{{P(100, 0), P(105, 0), P(105, 10), P(102, 10)}};
    CHECK_THAT(b.base_length().meters, WithinRel(a.base_length().meters, 1e-12));
    CHECK_THAT(b.area().square_meters, WithinRel(a.area().square_meters, 1e-12));
    CHECK_THAT(b.centroid().x.meters - 100.0, WithinRel(a.centroid().x.meters, 1e-9));
}
