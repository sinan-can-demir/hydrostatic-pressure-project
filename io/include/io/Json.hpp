/**
 * @file Json.hpp
 * @brief JSON scenario reading and result writing, schema versions 1 and 2 (FR-17, FR-19, FR-31).
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

// Version 2 adds tailwater and uplift (docs/09). Both versions are read; a result is written as
// version 2 only when the scenario uses the new inputs, so version 1 scenarios keep their exact
// output (NFR-12).
inline constexpr int kSchemaVersion = 2;

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
