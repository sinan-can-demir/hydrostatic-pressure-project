/**
 * @file Resultant.hpp
 * @brief Base resultant, contact pressures, sliding and the overturning report (FR-08 to FR-11).
 *
 * Conventions: x runs downstream, y up, moments are counter-clockwise positive about the toe.
 * A moment is therefore resisting when positive (weight upstream of the toe) and overturning
 * when negative (water pushing downstream above the base). The vertical resultant V is positive
 * downward, so it is the negative of the sum of fy. Distances x_R and the eccentricity e are
 * measured along the base from the toe; e = B/2 - x_R is positive when the resultant lies on the
 * toe side of the base centre.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ForceContribution.hpp"
#include "model/DamSection.hpp"
#include "units/Angle.hpp"
#include "units/ForcePerWidth.hpp"
#include "units/Length.hpp"
#include "units/MomentPerWidth.hpp"
#include "units/Pressure.hpp"

namespace simulation {

struct Resultant {
    bool operator==(const Resultant&) const = default;

    units::ForcePerWidth vertical;    // V, downward positive
    units::ForcePerWidth horizontal;  // H, downstream positive
    units::MomentPerWidth moment_about_toe;
    // Empty when the net vertical load is not downward (V <= 0): no location exists.
    std::optional<units::Length> x_from_toe;
    std::optional<units::Length> eccentricity;
    units::Length base_length;
};

Resultant make_resultant(const std::vector<ForceContribution>& contributions,
                         const model::DamSection& section);

enum class ContactState { Full, Partial, None };

// Base pressures. Partial contact is triangular over `length`, with zero pressure at the end of
// the base the resultant is far from; None means the resultant is on or beyond the base ends.
struct Contact {
    bool operator==(const Contact&) const = default;

    ContactState state;
    units::Length length;
    units::Pressure toe_pressure;
    units::Pressure heel_pressure;
};

Contact make_contact(const Resultant& resultant);

// FS = V tan(phi) / H with zero cohesion; empty when H <= 0 ("no driving shear").
struct SlidingResult {
    bool operator==(const SlidingResult&) const = default;

    std::optional<double> factor_of_safety;
};

SlidingResult make_sliding(const Resultant& resultant, units::Angle friction_angle);

enum class MomentRole { Resisting, Overturning, Neutral };
enum class ForceDirection { Horizontal, Vertical };

// One force component, its lever arm about the toe (a distance, >= 0 for a force on the
// section) and its signed moment.
struct OverturningTerm {
    bool operator==(const OverturningTerm&) const = default;

    std::string source;
    ForceDirection direction;
    units::ForcePerWidth force;
    units::Length arm;
    units::MomentPerWidth moment;
    MomentRole role;
};

struct OverturningReport {
    bool operator==(const OverturningReport&) const = default;

    std::vector<OverturningTerm> terms;
    units::MomentPerWidth resisting;    // sum of resisting moments (>= 0)
    units::MomentPerWidth overturning;  // magnitude of the sum of overturning moments (>= 0)
    std::optional<double> ratio;        // resisting / overturning; empty = unbounded
};

OverturningReport make_overturning(const std::vector<ForceContribution>& contributions,
                                   const model::DamSection& section);

} // namespace simulation
