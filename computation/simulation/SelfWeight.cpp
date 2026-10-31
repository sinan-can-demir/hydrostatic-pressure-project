#include "SelfWeight.hpp"

namespace simulation {

std::vector<ForceContribution> SelfWeight::contributions(const model::Scenario& scenario) const {
    const units::ForcePerWidth weight = scenario.concrete.unit_weight * scenario.section.area();
    return {ForceContribution{units::ForcePerWidth(0.0), -weight, scenario.section.centroid(),
                              "self weight"}};
}

} // namespace simulation
