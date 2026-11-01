#include "Validator.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>

#include "math/Polygon.hpp"

namespace simulation {

namespace {

constexpr double kMaxCoordinateMeters = 1e6; // NFR-10

using Errors = std::vector<ValidationError>;

void add(Errors& errors, ErrorCode code, std::string path, std::string message) {
    errors.push_back({code, std::move(path), std::move(message)});
}

std::string vertex_path(std::size_t i) { return std::format("dam.vertices_m[{}]", i); }

// Reports non-finite values; returns true when the value can be tested further.
bool finite(Errors& errors, double value, const std::string& path) {
    if (std::isfinite(value)) return true;
    add(errors, ErrorCode::NonFiniteValue, path, std::format("{} must be a finite number", path));
    return false;
}

void validate_section(const model::DamSection& section, Errors& errors) {
    const auto& v = section.vertices;
    const std::size_t n = v.size();
    const std::string path = "dam.vertices_m";

    if (n < 3) {
        add(errors, ErrorCode::TooFewVertices, path,
            std::format("the dam section needs at least 3 vertices, got {}", n));
        return;
    }

    bool numbers_ok = true;
    for (std::size_t i = 0; i < n; ++i) {
        for (const double c : {v[i].x.meters, v[i].y.meters}) {
            if (!std::isfinite(c)) {
                add(errors, ErrorCode::NonFiniteValue, vertex_path(i),
                    std::format("vertex {} has a non-finite coordinate", i));
                numbers_ok = false;
                break;
            }
            if (std::fabs(c) > kMaxCoordinateMeters) {
                add(errors, ErrorCode::ValueOutOfRange, vertex_path(i),
                    std::format("vertex {} has a coordinate beyond +/-{:g} m", i, kMaxCoordinateMeters));
                numbers_ok = false;
                break;
            }
        }
    }
    if (!numbers_ok) return;

    const double tol = math::geometric_tolerance(v).meters;
    const std::size_t errors_before = errors.size();

    // Independent structural checks.
    bool duplicates = false;
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t j = (i + 1) % n;
        const double dx = v[j].x.meters - v[i].x.meters;
        const double dy = v[j].y.meters - v[i].y.meters;
        if (std::hypot(dx, dy) <= tol) {
            add(errors, ErrorCode::DuplicateVertex, vertex_path(j),
                std::format("vertex {} repeats vertex {}", j, i));
            duplicates = true;
        }
    }

    for (std::size_t i = 0; i < n; ++i) {
        if (v[i].y.meters < -tol) {
            add(errors, ErrorCode::VertexBelowBase, vertex_path(i),
                std::format("vertex {} lies below the base line y = 0", i));
        }
    }

    const std::vector<std::size_t> base = section.base_vertex_indices();
    if (base.size() > 2) {
        add(errors, ErrorCode::MultipleBaseEdges, path,
            std::format("{} vertices lie on y = 0; the section must have exactly one base edge",
                        base.size()));
    } else if (base.size() < 2 || (base[1] - base[0] != 1 && !(base[0] == 0 && base[1] == n - 1))) {
        add(errors, ErrorCode::NoBaseEdge, path,
            "the section needs one horizontal base edge on y = 0 joining two consecutive vertices");
    }

    // Shape checks need non-degenerate edges.
    if (!duplicates) {
        const bool simple = math::is_simple(v, units::Length(tol));
        if (!simple) {
            add(errors, ErrorCode::SelfIntersecting, path, "the section outline crosses or touches itself");
        }

        const double area = math::signed_area(v).square_meters;
        const double size = tol / 1e-12;
        if (std::fabs(area) <= tol * size) {
            add(errors, ErrorCode::ZeroArea, path, "the section has zero area");
        } else if (area < 0.0 && simple) {
            add(errors, ErrorCode::ClockwiseOrder, path,
                "vertices are clockwise; list them counter-clockwise (base left to right, then up the "
                "downstream face, back across the crest and down the upstream face)");
        }
    }

    // The upstream face needs a sound, counter-clockwise polygon with a single base edge.
    if (errors.size() == errors_before) {
        const std::vector<math::Point2> face = section.upstream_face();
        for (std::size_t k = 0; k + 1 < face.size(); ++k) {
            if (face[k + 1].y.meters < face[k].y.meters - tol) {
                add(errors, ErrorCode::UpstreamFaceDips, path,
                    std::format("the upstream face drops from y = {:g} m to y = {:g} m going up from the "
                                "heel; its elevation must not decrease between the heel and the crest",
                                face[k].y.meters, face[k + 1].y.meters));
                break;
            }
        }
    }
}

void validate_water_level(const model::Scenario& s, Errors& errors) {
    const std::string path = "water_level_m";
    const double level = s.water_level.elevation.meters;
    if (!finite(errors, level, path)) return;
    if (std::fabs(level) > kMaxCoordinateMeters) {
        add(errors, ErrorCode::ValueOutOfRange, path,
            std::format("water level beyond +/-{:g} m", kMaxCoordinateMeters));
        return;
    }
    if (level < 0.0) {
        add(errors, ErrorCode::WaterLevelNegative, path,
            "the water level must not be below the base (y = 0)");
        return;
    }

    const auto& v = s.section.vertices;
    const bool usable = !v.empty() && std::all_of(v.begin(), v.end(), [](const math::Point2& p) {
        return std::isfinite(p.x.meters) && std::isfinite(p.y.meters);
    });
    if (!usable) return;

    double crest = v[0].y.meters;
    for (const math::Point2& p : v) crest = std::max(crest, p.y.meters);
    if (level > crest + math::geometric_tolerance(v).meters) {
        add(errors, ErrorCode::WaterLevelAboveCrest, path,
            std::format("the water level {:g} m is above the crest at {:g} m (overtopping is not modelled)",
                        level, crest));
    }
}

void validate_positive(Errors& errors, double value, const std::string& path, ErrorCode code,
                       const char* what) {
    if (!finite(errors, value, path)) return;
    if (value <= 0.0) add(errors, code, path, std::format("{} must be greater than zero", what));
}

void validate_parameters(const model::Scenario& s, Errors& errors) {
    // Friction angle: strictly between 0 and 90 degrees (decision U3).
    const double phi = s.foundation.friction_angle.degrees();
    if (finite(errors, phi, "friction_angle_deg") && (phi <= 0.0 || phi >= 90.0)) {
        add(errors, ErrorCode::FrictionAngleOutOfRange, "friction_angle_deg",
            std::format("the friction angle must be strictly between 0 and 90 degrees, got {:g}", phi));
    }

    if (s.foundation.allowable_bearing.has_value()) {
        validate_positive(errors, s.foundation.allowable_bearing->pascals, "allowable_bearing_pa",
                          ErrorCode::NonPositiveAllowableBearing, "the allowable bearing pressure");
    }

    validate_positive(errors, s.fluid.density.kg_per_m3, "fluid.density_kg_m3",
                      ErrorCode::NonPositiveFluidDensity, "the fluid density");
    validate_positive(errors, s.fluid.gravity.m_per_s2, "fluid.gravity_m_s2", ErrorCode::NonPositiveGravity,
                      "the gravitational acceleration");
    validate_positive(errors, s.concrete.unit_weight.n_per_m3, "concrete.unit_weight_n_m3",
                      ErrorCode::NonPositiveUnitWeight, "the concrete unit weight");

    validate_positive(errors, s.criteria.usual.min_sliding_fs, "criteria.usual.min_sliding_fs",
                      ErrorCode::NonPositiveSlidingFs, "the minimum sliding factor of safety");
    validate_positive(errors, s.criteria.unusual.min_sliding_fs, "criteria.unusual.min_sliding_fs",
                      ErrorCode::NonPositiveSlidingFs, "the minimum sliding factor of safety");
    validate_positive(errors, s.criteria.extreme.min_sliding_fs, "criteria.extreme.min_sliding_fs",
                      ErrorCode::NonPositiveSlidingFs, "the minimum sliding factor of safety");

    if (s.pressure_samples && (*s.pressure_samples < 2 || *s.pressure_samples > 100000)) {
        add(errors, ErrorCode::PressureSamplesOutOfRange, "pressure_samples",
            std::format("pressure_samples must be between 2 and 100000, got {}", *s.pressure_samples));
    }
}

} // namespace

std::vector<ValidationError> validate(const model::Scenario& scenario) {
    Errors errors;
    validate_section(scenario.section, errors);
    validate_water_level(scenario, errors);
    validate_parameters(scenario, errors);
    return errors;
}

} // namespace simulation
