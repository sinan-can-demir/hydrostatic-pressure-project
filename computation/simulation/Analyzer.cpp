#include "Analyzer.hpp"

#include <utility>

#include "SelfWeight.hpp"
#include "TailwaterLoad.hpp"
#include "Validator.hpp"
#include "WaterLoad.hpp"

namespace simulation {

namespace {

// FR-15: attached to every result, including failed ones.
constexpr const char* kUpliftWarning = "uplift not modelled; results are unconservative for real dams";

} // namespace

Analyzer::Analyzer(std::vector<std::unique_ptr<Load>> loads) : loads_(std::move(loads)) {}

Analyzer Analyzer::with_default_loads() {
    std::vector<std::unique_ptr<Load>> loads;
    loads.push_back(std::make_unique<WaterLoad>());
    loads.push_back(std::make_unique<SelfWeight>());
    loads.push_back(std::make_unique<TailwaterLoad>()); // emits nothing without a tailwater depth
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
                  contributions,
                  resultant,
                  contact,
                  sliding,
                  make_overturning(contributions, scenario.section),
                  checks,
                  {kUpliftWarning},
                  make_verdict(checks)};
}

std::expected<Result, std::vector<ValidationError>> analyze(const model::Scenario& scenario) {
    return Analyzer::with_default_loads().run(scenario);
}

} // namespace simulation
