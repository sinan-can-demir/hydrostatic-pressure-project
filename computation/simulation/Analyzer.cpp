#include "Analyzer.hpp"

#include <utility>

#include "SelfWeight.hpp"
#include "TailwaterLoad.hpp"
#include "UpliftLoad.hpp"
#include "Validator.hpp"
#include "WaterLoad.hpp"

namespace simulation {

namespace {

// FR-15: attached to every result without an uplift request, including failed ones.
constexpr const char* kUpliftWarning = "uplift not modelled; results are unconservative for real dams";

// FR-15 (v1.1): replaces the warning above when uplift is modelled.
constexpr const char* kUpliftModelled =
    "uplift modelled per EM 1110-2-2200 para. 3-3.d assuming the whole base is in compression";

// FR-28: the uplift diagram is only valid for a base fully in compression.
constexpr const char* kPartialContactWarning =
    "base not fully in compression: uplift assumes full contact and is unconservative; the standard "
    "requires full reservoir pressure on the uncompressed zone";

// FR-25: drain effectiveness limits of EM 1110-2-2200 para. 3-3.d(1)(c).
constexpr const char* kEffectivenessRange =
    "drain effectiveness is outside the 25 to 50 percent range assumed by EM 1110-2-2200; "
    "it is a judgement the user must justify";
constexpr const char* kEffectivenessHigh =
    "drain effectiveness above 67 percent exceeds the limit EM 1110-2-2200 allows, and only with "
    "approval and supporting data";

std::vector<std::string> make_warnings(const model::Scenario& scenario, const Contact& contact) {
    if (!scenario.uplift) return {kUpliftWarning};

    std::vector<std::string> warnings{kUpliftModelled};
    if (scenario.uplift->drains) {
        const double e = scenario.uplift->drains->effectiveness;
        if (e > 0.67) {
            warnings.emplace_back(kEffectivenessHigh);
        } else if (e < 0.25 || e > 0.50) {
            warnings.emplace_back(kEffectivenessRange);
        }
    }
    if (contact.state != ContactState::Full) warnings.emplace_back(kPartialContactWarning);
    return warnings;
}

} // namespace

Analyzer::Analyzer(std::vector<std::unique_ptr<Load>> loads) : loads_(std::move(loads)) {}

Analyzer Analyzer::with_default_loads() {
    std::vector<std::unique_ptr<Load>> loads;
    loads.push_back(std::make_unique<WaterLoad>());
    loads.push_back(std::make_unique<SelfWeight>());
    loads.push_back(std::make_unique<TailwaterLoad>()); // emits nothing without a tailwater depth
    loads.push_back(std::make_unique<UpliftLoad>());    // emits nothing without an uplift request
    return Analyzer(std::move(loads));
}

std::expected<Result, std::vector<ValidationError>> Analyzer::run(const model::Scenario& scenario) const {
    if (std::vector<ValidationError> errors = validate(scenario); !errors.empty()) {
        return std::unexpected(std::move(errors));
    }

    std::vector<ForceContribution> contributions;
    for (const std::unique_ptr<Load>& load : loads_) {
        std::vector<ForceContribution> part = load->contributions(scenario);
        contributions.insert(contributions.end(), std::make_move_iterator(part.begin()),
                             std::make_move_iterator(part.end()));
    }

    const Resultant resultant = make_resultant(contributions, scenario.section);
    const Contact contact = make_contact(resultant);
    const SlidingResult sliding = make_sliding(resultant, scenario.foundation.friction_angle);
    const model::CriteriaRow criteria = scenario.criteria.row_for(scenario.condition);
    const Checks checks{check_location(resultant, contact, criteria.zone),
                        check_sliding(sliding, criteria.min_sliding_fs),
                        check_bearing(contact, scenario.foundation.allowable_bearing)};

    return Result{scenario,
                  WaterLoad{}.pressure(scenario, scenario.pressure_samples),
                  uplift_diagram(scenario),
                  contributions,
                  resultant,
                  contact,
                  sliding,
                  make_overturning(contributions, scenario.section),
                  checks,
                  make_warnings(scenario, contact),
                  make_verdict(checks)};
}

std::expected<Result, std::vector<ValidationError>> analyze(const model::Scenario& scenario) {
    return Analyzer::with_default_loads().run(scenario);
}

} // namespace simulation
