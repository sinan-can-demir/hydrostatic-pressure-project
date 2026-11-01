#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <vector>

#include "math/Point2.hpp"
#include "math/Polygon.hpp"
#include "units/Length.hpp"

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace units;
using math::Point2;

namespace {

Point2 P(double x, double y) { return Point2{Length(x), Length(y)}; }

const Length kTol(1e-9);

} // namespace

TEST_CASE("signed area is positive counter-clockwise and negative clockwise", "[FR-02][G-01][G-03]") {
    const std::vector<Point2> rect{P(0, 0), P(5, 0), P(5, 10), P(0, 10)};
    CHECK_THAT(math::signed_area(rect).square_meters, WithinRel(50.0, 1e-12));

    const std::vector<Point2> reversed{P(0, 10), P(5, 10), P(5, 0), P(0, 0)};
    CHECK_THAT(math::signed_area(reversed).square_meters, WithinRel(-50.0, 1e-12));

    const std::vector<Point2> triangle{P(0, 0), P(7, 0), P(0, 10)};
    CHECK_THAT(math::signed_area(triangle).square_meters, WithinRel(35.0, 1e-12));

    const std::vector<Point2> trapezoid{P(0, 0), P(5, 0), P(5, 10), P(2, 10)};
    CHECK_THAT(math::signed_area(trapezoid).square_meters, WithinRel(40.0, 1e-12));
}

TEST_CASE("signed area ignores which vertex the list starts at", "[FR-01][P-02]") {
    const std::vector<Point2> a{P(0, 0), P(5, 0), P(5, 10), P(0, 10)};
    const std::vector<Point2> b{P(5, 10), P(0, 10), P(0, 0), P(5, 0)};
    CHECK_THAT(math::signed_area(a).square_meters, WithinRel(math::signed_area(b).square_meters, 1e-12));
}

TEST_CASE("centroid of rectangle, triangle and trapezoid", "[FR-07][G-01]") {
    const Point2 c1 = math::centroid(std::vector<Point2>{P(0, 0), P(5, 0), P(5, 10), P(0, 10)});
    CHECK_THAT(c1.x.meters, WithinRel(2.5, 1e-12));
    CHECK_THAT(c1.y.meters, WithinRel(5.0, 1e-12));

    const Point2 c2 = math::centroid(std::vector<Point2>{P(0, 0), P(7, 0), P(0, 10)});
    CHECK_THAT(c2.x.meters, WithinRel(7.0 / 3.0, 1e-12));
    CHECK_THAT(c2.y.meters, WithinRel(10.0 / 3.0, 1e-12));

    // Hand decomposition: rectangle x in [2,5] (area 30, centroid 3.5, 5) plus triangle
    // (0,0) (2,0) (2,10) (area 10, centroid 4/3, 10/3): cx = 71/24, cy = 55/12.
    const Point2 c3 = math::centroid(std::vector<Point2>{P(0, 0), P(5, 0), P(5, 10), P(2, 10)});
    CHECK_THAT(c3.x.meters, WithinRel(71.0 / 24.0, 1e-12));
    CHECK_THAT(c3.y.meters, WithinRel(55.0 / 12.0, 1e-12));
}

TEST_CASE("centroid is the same for either orientation", "[FR-07][G-01]") {
    const Point2 c = math::centroid(std::vector<Point2>{P(0, 10), P(5, 10), P(5, 0), P(0, 0)});
    CHECK_THAT(c.x.meters, WithinRel(2.5, 1e-12));
    CHECK_THAT(c.y.meters, WithinRel(5.0, 1e-12));
}

TEST_CASE("simple polygons are accepted", "[FR-02][G-04]") {
    CHECK(math::is_simple(std::vector<Point2>{P(0, 0), P(5, 0), P(5, 10), P(0, 10)}, kTol));
    CHECK(math::is_simple(std::vector<Point2>{P(0, 0), P(7, 0), P(0, 10)}, kTol));
    // Concave L shape.
    CHECK(math::is_simple(
        std::vector<Point2>{P(0, 0), P(5, 0), P(5, 10), P(3, 10), P(3, 4), P(0, 4)}, kTol));
    // A straight-angle vertex (collinear but not folded back) is fine.
    CHECK(math::is_simple(
        std::vector<Point2>{P(0, 0), P(5, 0), P(5, 5), P(5, 10), P(0, 10)}, kTol));
}

TEST_CASE("self-intersecting polygons are rejected", "[FR-02][G-04]") {
    // Bow tie.
    CHECK_FALSE(math::is_simple(std::vector<Point2>{P(0, 0), P(5, 10), P(5, 0), P(0, 10)}, kTol));
    // Two non-adjacent edges cross.
    CHECK_FALSE(math::is_simple(std::vector<Point2>{P(0, 0), P(5, 0), P(1, 6), P(4, 6)}, kTol));
}

TEST_CASE("a vertex touching a non-adjacent edge is rejected", "[FR-02][G-04]") {
    CHECK_FALSE(math::is_simple(
        std::vector<Point2>{P(0, 0), P(6, 0), P(6, 6), P(3, 0), P(0, 6)}, kTol));
}

TEST_CASE("adjacent edges folding back on each other are rejected", "[FR-02][G-04]") {
    CHECK_FALSE(math::is_simple(std::vector<Point2>{P(0, 0), P(5, 0), P(2, 0), P(0, 10)}, kTol));
    // A triangle has no non-adjacent edges, so only the fold-back test can reject this one.
    CHECK_FALSE(math::is_simple(std::vector<Point2>{P(0, 0), P(5, 0), P(2, 0)}, kTol));
}

TEST_CASE("a zero-length edge is rejected", "[FR-02][G-04]") {
    CHECK_FALSE(math::is_simple(
        std::vector<Point2>{P(0, 0), P(5, 0), P(5, 0), P(5, 10), P(0, 10)}, kTol));
}

TEST_CASE("geometric tolerance is relative to the polygon size", "[NFR-01][ADR-0007]") {
    const std::vector<Point2> small{P(0, 0), P(1, 0), P(1, 1), P(0, 1)};
    const std::vector<Point2> large{P(0, 0), P(500, 0), P(500, 1000), P(0, 1000)};
    CHECK_THAT(math::geometric_tolerance(small).meters, WithinRel(1e-12, 1e-12));
    CHECK_THAT(math::geometric_tolerance(large).meters, WithinRel(1e-9, 1e-12));
    CHECK_THAT(math::geometric_tolerance(std::vector<Point2>{}).meters, WithinAbs(0.0, 0.0));
}

TEST_CASE("a long outline is simple, and a far-apart crossing is still found", "[FR-02][G-04][NFR-08]") {
    // Many vertices on a curved downstream face, so most edge pairs are far apart.
    auto curve = [](Point2 crest_upstream) {
        std::vector<Point2> v{P(0, 0), P(60, 0)};
        for (int i = 1; i <= 400; ++i) {
            const double t = i / 400.0;
            v.push_back(P(60.0 * (1.0 - 0.6 * std::pow(t, 1.5)), 100.0 * t));
        }
        v.push_back(crest_upstream);
        return v;
    };
    CHECK(math::is_simple(curve(P(0, 100)), kTol));
    // Pulling the last vertex across the dam makes its two edges cut the downstream face
    // hundreds of edges away from them.
    CHECK_FALSE(math::is_simple(curve(P(54, 50)), kTol));
}
