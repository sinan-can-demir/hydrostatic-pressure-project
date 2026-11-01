#include <catch2/catch_test_macros.hpp>

#include <type_traits>

#include "math/Point2.hpp"
#include "math/Vector2.hpp"
#include "units/Area.hpp"
#include "units/Length.hpp"

using namespace units;
using math::Point2;
using math::Vector2;

TEST_CASE("point and vector store their components", "[FR-01][M-01]") {
    const Point2 p{Length(1.5), Length(-2.0)};
    CHECK(p.x.meters == 1.5);
    CHECK(p.y.meters == -2.0);

    const Vector2 v{Length(3.0), Length(4.0)};
    CHECK(v.x.meters == 3.0);
    CHECK(v.y.meters == 4.0);
}

TEST_CASE("point minus point is a vector, point plus vector is a point", "[FR-01][M-01]") {
    static_assert(
        std::is_same_v<decltype(Point2{Length(0), Length(0)} - Point2{Length(0), Length(0)}), Vector2>);
    static_assert(
        std::is_same_v<decltype(Point2{Length(0), Length(0)} + Vector2{Length(0), Length(0)}), Point2>);

    const Point2 heel{Length(0.0), Length(0.0)};
    const Point2 toe{Length(5.0), Length(0.0)};
    const Vector2 d = toe - heel;
    CHECK(d.x.meters == 5.0);
    CHECK(d.y.meters == 0.0);

    const Point2 back = heel + d;
    CHECK(back.x.meters == 5.0);
    CHECK(back.y.meters == 0.0);
}

TEST_CASE("vector arithmetic", "[FR-01][M-01]") {
    const Vector2 a{Length(1.0), Length(2.0)};
    const Vector2 b{Length(10.0), Length(20.0)};

    const Vector2 sum = a + b;
    CHECK(sum.x.meters == 11.0);
    CHECK(sum.y.meters == 22.0);

    const Vector2 diff = a - b;
    CHECK(diff.x.meters == -9.0);
    CHECK(diff.y.meters == -18.0);

    const Vector2 scaled = a * 3.0;
    CHECK(scaled.x.meters == 3.0);
    CHECK(scaled.y.meters == 6.0);
    CHECK((3.0 * a).y.meters == 6.0);

    const Vector2 neg = -a;
    CHECK(neg.x.meters == -1.0);
    CHECK(neg.y.meters == -2.0);
}

TEST_CASE("cross product is an area whose sign gives the turn direction", "[FR-02][M-01]") {
    static_assert(
        std::is_same_v<decltype(cross(Vector2{Length(0), Length(0)}, Vector2{Length(0), Length(0)})), Area>);

    const Vector2 along_base{Length(5.0), Length(0.0)};
    const Vector2 up{Length(0.0), Length(10.0)};

    // A counter-clockwise turn (base direction, then up) is positive.
    CHECK(cross(along_base, up).square_meters == 50.0);
    // Reversing the order is a clockwise turn.
    CHECK(cross(up, along_base).square_meters == -50.0);
    // Parallel vectors give zero.
    CHECK(cross(along_base, along_base * 2.0).square_meters == 0.0);
}

TEST_CASE("dot product is an area-valued scalar product", "[M-01]") {
    const Vector2 a{Length(3.0), Length(4.0)};
    const Vector2 b{Length(2.0), Length(-1.0)};
    CHECK(dot(a, b).square_meters == 2.0);
    CHECK(dot(Vector2{Length(1.0), Length(0.0)}, Vector2{Length(0.0), Length(1.0)}).square_meters == 0.0);
}
