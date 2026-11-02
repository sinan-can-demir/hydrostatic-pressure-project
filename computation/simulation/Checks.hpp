/**
 * @file Checks.hpp
 * @brief Pass/fail checks against the criteria and the overall verdict (FR-12 to FR-14).
 *
 * Comparisons carry no tolerance (ADR-0007): a value exactly on a threshold passes.
 */

#pragma once

#include <optional>

#include "Resultant.hpp"
#include "model/Criteria.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

namespace simulation {

enum class CheckStatus { Pass, Fail, Skipped };
enum class Verdict { Pass, Fail };

// The resultant must lie inside the zone (measured from the toe) and the base must be in contact.
struct LocationCheck {
    bool operator==(const LocationCheck&) const = default;

    CheckStatus status;
    model::ResultantZone zone;
    units::Length lower;
    units::Length upper;
};

struct SlidingCheck {
    bool operator==(const SlidingCheck&) const = default;

    CheckStatus status;
    std::optional<double> factor_of_safety; // empty = no driving shear, which passes
    double minimum;
};

// Skipped when no allowable pressure was supplied; a skipped check never fails the verdict.
// `allowable` is the limit applied: the user's allowable times the condition's bearing factor.
struct BearingCheck {
    bool operator==(const BearingCheck&) const = default;

    CheckStatus status;
    units::Pressure max_pressure;
    std::optional<units::Pressure> allowable;
};

struct Checks {
    bool operator==(const Checks&) const = default;

    LocationCheck location;
    SlidingCheck sliding;
    BearingCheck bearing;
};

LocationCheck check_location(const Resultant& resultant, const Contact& contact, model::ResultantZone zone);
SlidingCheck check_sliding(const SlidingResult& sliding, double minimum_fs);
BearingCheck check_bearing(const Contact& contact, std::optional<units::Pressure> allowable,
                           double bearing_factor);
Verdict make_verdict(const Checks& checks);

} // namespace simulation
