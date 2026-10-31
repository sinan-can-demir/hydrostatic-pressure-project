#include "Polygon.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace math {

namespace {

constexpr double kRelativeTolerance = 1e-12;

// Plain-double view of a point for the intersection tests.
struct D {
    double x;
    double y;
};

D to_d(const Point2& p) { return {p.x.meters, p.y.meters}; }

double length_of(D a, D b) { return std::hypot(b.x - a.x, b.y - a.y); }

// Signed distance of c from the line through a and b (positive on the left of a->b).
double side_distance(D a, D b, D c) {
    const double cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    return cross / length_of(a, b);
}

int side(D a, D b, D c, double tol) {
    const double d = side_distance(a, b, c);
    return d > tol ? 1 : (d < -tol ? -1 : 0);
}

// c is assumed collinear with a-b; true when it lies within the segment (ends included).
bool on_segment(D a, D b, D c, double tol) {
    const double len = length_of(a, b);
    const double along = ((c.x - a.x) * (b.x - a.x) + (c.y - a.y) * (b.y - a.y)) / len;
    return along >= -tol && along <= len + tol;
}

bool segments_meet(D a, D b, D c, D d, double tol) {
    const int o1 = side(c, d, a, tol);
    const int o2 = side(c, d, b, tol);
    const int o3 = side(a, b, c, tol);
    const int o4 = side(a, b, d, tol);
    if (o1 * o2 < 0 && o3 * o4 < 0) return true;
    if (o1 == 0 && on_segment(c, d, a, tol)) return true;
    if (o2 == 0 && on_segment(c, d, b, tol)) return true;
    if (o3 == 0 && on_segment(a, b, c, tol)) return true;
    if (o4 == 0 && on_segment(a, b, d, tol)) return true;
    return false;
}

// Edges prev-mid and mid-next share the vertex mid; they overlap when one folds back on the other.
bool folds_back(D prev, D mid, D next, double tol) {
    if (side(prev, mid, next, tol) != 0) return false;
    return on_segment(prev, mid, next, tol) || on_segment(mid, next, prev, tol);
}

} // namespace

units::Area signed_area(std::span<const Point2> v) {
    // Fan from the first vertex; measuring from it keeps the sum accurate far from the origin.
    units::Area twice(0.0);
    for (std::size_t i = 1; i + 1 < v.size(); ++i) {
        twice = twice + cross(v[i] - v[0], v[i + 1] - v[0]);
    }
    return 0.5 * twice;
}

Point2 centroid(std::span<const Point2> v) {
    if (v.empty()) return Point2{units::Length(0.0), units::Length(0.0)};
    if (v.size() < 3) return v[0];

    // Area-weighted centroids of the fan triangles (v0, vi, vi+1), as offsets from v0.
    double area_sum = 0.0;
    double mx = 0.0;
    double my = 0.0;
    for (std::size_t i = 1; i + 1 < v.size(); ++i) {
        const Vector2 a = v[i] - v[0];
        const Vector2 b = v[i + 1] - v[0];
        const double tri_area = 0.5 * cross(a, b).square_meters;
        area_sum += tri_area;
        mx += tri_area * (a.x.meters + b.x.meters) / 3.0;
        my += tri_area * (a.y.meters + b.y.meters) / 3.0;
    }
    if (area_sum == 0.0) return v[0];
    return v[0] + Vector2{units::Length(mx / area_sum), units::Length(my / area_sum)};
}

bool is_simple(std::span<const Point2> v, units::Length tolerance) {
    const std::size_t n = v.size();
    if (n < 3) return false;
    const double tol = tolerance.meters;

    for (std::size_t i = 0; i < n; ++i) {
        if (length_of(to_d(v[i]), to_d(v[(i + 1) % n])) <= tol) return false;
    }

    // Adjacent edge pairs: only fold-backs can make them meet beyond their shared vertex.
    for (std::size_t i = 0; i < n; ++i) {
        if (folds_back(to_d(v[i]), to_d(v[(i + 1) % n]), to_d(v[(i + 2) % n]), tol)) return false;
    }

    // Non-adjacent edge pairs must not meet at all.
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 2; j < n; ++j) {
            if (i == 0 && j == n - 1) continue; // adjacent through the closing edge
            if (segments_meet(to_d(v[i]), to_d(v[(i + 1) % n]), to_d(v[j]), to_d(v[(j + 1) % n]), tol)) {
                return false;
            }
        }
    }
    return true;
}

units::Length geometric_tolerance(std::span<const Point2> v) {
    if (v.empty()) return units::Length(0.0);
    double min_x = v[0].x.meters, max_x = min_x;
    double min_y = v[0].y.meters, max_y = min_y;
    for (const Point2& p : v) {
        min_x = std::min(min_x, p.x.meters);
        max_x = std::max(max_x, p.x.meters);
        min_y = std::min(min_y, p.y.meters);
        max_y = std::max(max_y, p.y.meters);
    }
    return units::Length(kRelativeTolerance * std::max(max_x - min_x, max_y - min_y));
}

} // namespace math
