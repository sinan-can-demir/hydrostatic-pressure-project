/**
 * @file Json.hpp
 * @brief JSON scenario reading and result writing, schema version 1 (FR-17, FR-19).
 *
 * Keys carry their unit as a suffix. Unknown keys are rejected so typos are not silently
 * ignored. The JSON library stays inside the implementation: no exception and no JSON type
 * crosses this interface (ADR-0005, NFR-04).
 */

#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "IoError.hpp"
#include "model/Scenario.hpp"
#include "simulation/Result.hpp"

namespace io {

inline constexpr int kSchemaVersion = 1;

// Every structural problem found is returned, not just the first. Meaning (ranges, geometry)
// is checked later by simulation::validate().
std::expected<model::Scenario, std::vector<IoError>> parse_scenario(std::string_view json_text);

// The scenario in the input schema, explicit about every value, so parsing it back gives an
// equal Scenario.
std::string scenario_to_json(const model::Scenario& scenario);

// The full result: the echoed input, then every intermediate value, checks and verdict.
// Key order and number format are fixed, so equal results give identical text.
std::string result_to_json(const simulation::Result& result);

} // namespace io
