/**
 * @file Validator.hpp
 * @brief Checks a Scenario against requirements FR-02 to FR-04, NFR-10 and decisions U1 to U3.
 */

#pragma once

#include <vector>

#include "ValidationError.hpp"
#include "model/Scenario.hpp"

namespace simulation {

// Returns every problem found (decision U1); an empty list means the scenario is valid.
// Never throws. Checks that depend on a sound polygon are skipped when the polygon is
// unusable (fewer than 3 vertices, non-finite or out-of-range coordinates, repeated vertices).
std::vector<ValidationError> validate(const model::Scenario& scenario);

} // namespace simulation
