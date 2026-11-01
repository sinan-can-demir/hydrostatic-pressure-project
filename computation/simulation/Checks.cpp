#include "Checks.hpp"

#include <algorithm>

namespace simulation {

LocationCheck check_location(const Resultant& resultant, const Contact& contact, model::ResultantZone zone) {
    const double base = resultant.base_length.meters;
    double lower = 0.0;
    double upper = base;
    if (zone == model::ResultantZone::MiddleThird) {
        lower = base / 3.0;
        upper = 2.0 * base / 3.0;
    } else if (zone == model::ResultantZone::MiddleHalf) {
        lower = base / 4.0;
        upper = 3.0 * base / 4.0;
    }

    const bool inside = resultant.x_from_toe && contact.state != ContactState::None &&
                        resultant.x_from_toe->meters >= lower && resultant.x_from_toe->meters <= upper;
    return {inside ? CheckStatus::Pass : CheckStatus::Fail, zone, units::Length(lower), units::Length(upper)};
}

SlidingCheck check_sliding(const SlidingResult& sliding, double minimum_fs) {
    const bool pass = !sliding.factor_of_safety || *sliding.factor_of_safety >= minimum_fs;
    return {pass ? CheckStatus::Pass : CheckStatus::Fail, sliding.factor_of_safety, minimum_fs};
}

BearingCheck check_bearing(const Contact& contact, std::optional<units::Pressure> allowable) {
    const units::Pressure max_pressure(std::max(contact.toe_pressure.pascals, contact.heel_pressure.pascals));
    if (!allowable) return {CheckStatus::Skipped, max_pressure, std::nullopt};
    const bool pass = contact.state != ContactState::None && max_pressure.pascals <= allowable->pascals;
    return {pass ? CheckStatus::Pass : CheckStatus::Fail, max_pressure, allowable};
}

Verdict make_verdict(const Checks& checks) {
    const bool failed = checks.location.status == CheckStatus::Fail ||
                        checks.sliding.status == CheckStatus::Fail ||
                        checks.bearing.status == CheckStatus::Fail;
    return failed ? Verdict::Fail : Verdict::Pass;
}

} // namespace simulation
