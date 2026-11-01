#include "DamSection.hpp"

#include <algorithm>

#include "math/Polygon.hpp"

namespace model {

namespace {

const math::Point2 kOrigin{units::Length(0.0), units::Length(0.0)};

} // namespace

std::vector<std::size_t> DamSection::base_vertex_indices() const {
    const double tol = math::geometric_tolerance(vertices).meters;
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        if (vertices[i].y.meters >= -tol && vertices[i].y.meters <= tol) indices.push_back(i);
    }
    return indices;
}

namespace {

// Index of the heel: of the two base vertices, the one with the smaller x.
std::size_t heel_index(const DamSection& s) {
    const std::vector<std::size_t> base = s.base_vertex_indices();
    if (base.size() < 2) return 0;
    return s.vertices[base[0]].x.meters <= s.vertices[base[1]].x.meters ? base[0] : base[1];
}

std::size_t crest_index(const DamSection& s) {
    const double tol = math::geometric_tolerance(s.vertices).meters;
    double max_y = s.vertices[0].y.meters;
    for (const math::Point2& p : s.vertices) max_y = std::max(max_y, p.y.meters);

    std::size_t best = 0;
    bool found = false;
    for (std::size_t i = 0; i < s.vertices.size(); ++i) {
        if (s.vertices[i].y.meters < max_y - tol) continue;
        if (!found || s.vertices[i].x.meters < s.vertices[best].x.meters) {
            best = i;
            found = true;
        }
    }
    return best;
}

} // namespace

math::Point2 DamSection::heel() const { return vertices.empty() ? kOrigin : vertices[heel_index(*this)]; }

math::Point2 DamSection::toe() const {
    const std::vector<std::size_t> base = base_vertex_indices();
    if (base.size() < 2) return kOrigin;
    const math::Point2& a = vertices[base[0]];
    const math::Point2& b = vertices[base[1]];
    return a.x.meters >= b.x.meters ? a : b;
}

units::Length DamSection::base_length() const { return toe().x - heel().x; }

units::Length DamSection::crest_height() const {
    if (vertices.empty()) return units::Length(0.0);
    return vertices[crest_index(*this)].y;
}

units::Area DamSection::area() const { return math::signed_area(vertices); }

math::Point2 DamSection::centroid() const { return math::centroid(vertices); }

std::vector<math::Point2> DamSection::upstream_face() const {
    std::vector<math::Point2> face;
    if (vertices.size() < 3) return face;

    // Counter-clockwise order runs heel, toe, up the downstream side, across the crest and
    // down the upstream side back to the heel; walk that last stretch and reverse it.
    const std::size_t heel = heel_index(*this);
    std::size_t i = crest_index(*this);
    face.push_back(vertices[i]);
    while (i != heel) {
        i = (i + 1) % vertices.size();
        face.push_back(vertices[i]);
    }
    std::ranges::reverse(face);
    return face;
}

} // namespace model
