/**
 * @file ValidationError.hpp
 * @brief Typed validation failures. Codes are stable and shared by the library and the CLI.
 */

#pragma once

#include <string>
#include <string_view>

namespace simulation {

enum class ErrorCode {
    TooFewVertices,
    NonFiniteValue,
    ValueOutOfRange,
    DuplicateVertex,
    ClockwiseOrder,
    ZeroArea,
    SelfIntersecting,
    NoBaseEdge,
    MultipleBaseEdges,
    VertexBelowBase,
    UpstreamFaceDips,
    WaterLevelNegative,
    WaterLevelAboveCrest,
    FrictionAngleOutOfRange,
    NonPositiveFluidDensity,
    NonPositiveGravity,
    NonPositiveUnitWeight,
    NonPositiveAllowableBearing,
    NonPositiveSlidingFs,
};

constexpr std::string_view to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::TooFewVertices: return "too_few_vertices";
        case ErrorCode::NonFiniteValue: return "non_finite_value";
        case ErrorCode::ValueOutOfRange: return "value_out_of_range";
        case ErrorCode::DuplicateVertex: return "duplicate_vertex";
        case ErrorCode::ClockwiseOrder: return "clockwise_order";
        case ErrorCode::ZeroArea: return "zero_area";
        case ErrorCode::SelfIntersecting: return "self_intersecting";
        case ErrorCode::NoBaseEdge: return "no_base_edge";
        case ErrorCode::MultipleBaseEdges: return "multiple_base_edges";
        case ErrorCode::VertexBelowBase: return "vertex_below_base";
        case ErrorCode::UpstreamFaceDips: return "upstream_face_dips";
        case ErrorCode::WaterLevelNegative: return "water_level_negative";
        case ErrorCode::WaterLevelAboveCrest: return "water_level_above_crest";
        case ErrorCode::FrictionAngleOutOfRange: return "friction_angle_out_of_range";
        case ErrorCode::NonPositiveFluidDensity: return "non_positive_fluid_density";
        case ErrorCode::NonPositiveGravity: return "non_positive_gravity";
        case ErrorCode::NonPositiveUnitWeight: return "non_positive_unit_weight";
        case ErrorCode::NonPositiveAllowableBearing: return "non_positive_allowable_bearing";
        case ErrorCode::NonPositiveSlidingFs: return "non_positive_sliding_fs";
    }
    return "unknown";
}

struct ValidationError {
    ErrorCode code;
    std::string path;    // scenario field, named as in the JSON schema (unit-suffixed keys)
    std::string message;
};

} // namespace simulation
