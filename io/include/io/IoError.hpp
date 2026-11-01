/**
 * @file IoError.hpp
 * @brief Failures while reading a scenario document. Separate from simulation::ValidationError:
 *        these describe the document's shape, validation describes its meaning.
 */

#pragma once

#include <string>
#include <string_view>

namespace io {

enum class IoErrorCode {
    MalformedJson,
    UnsupportedSchemaVersion,
    MissingField,
    UnknownField,
    WrongType,
    InvalidValue,
};

constexpr std::string_view to_string(IoErrorCode code) {
    switch (code) {
        case IoErrorCode::MalformedJson: return "malformed_json";
        case IoErrorCode::UnsupportedSchemaVersion: return "unsupported_schema_version";
        case IoErrorCode::MissingField: return "missing_field";
        case IoErrorCode::UnknownField: return "unknown_field";
        case IoErrorCode::WrongType: return "wrong_type";
        case IoErrorCode::InvalidValue: return "invalid_value";
    }
    return "unknown";
}

struct IoError {
    IoErrorCode code;
    std::string path;     // dotted path in the document, e.g. "fluid.density_kg_m3"; "" = whole document
    std::string message;
};

} // namespace io
