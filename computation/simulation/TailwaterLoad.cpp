#include "TailwaterLoad.hpp"

#include "FaceLoad.hpp"

namespace simulation {

std::vector<ForceContribution> TailwaterLoad::contributions(const model::Scenario& scenario) const {
    if (!scenario.tailwater_depth) return {};
    const double depth = scenario.tailwater_depth->meters;
    const double k = scenario.fluid.density.kg_per_m3 * scenario.fluid.gravity.m_per_s2;
    const std::vector<detail::Pt> chain = detail::wetted_chain(scenario.section.downstream_face(), depth);
    return detail::face_contributions(chain, depth, k, detail::Side::Downstream, "tailwater (horizontal)",
                                      "tailwater (vertical)");
}

} // namespace simulation
