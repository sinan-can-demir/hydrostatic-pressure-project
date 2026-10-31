/**
 * @file Analyzer.hpp
 * @brief The whole analysis as a pure function from Scenario to Result (FR-16).
 *
 * Invalid scenarios come back as the full list of validation errors; nothing is thrown and
 * nothing is computed. The Analyzer holds the Load set, so callers can add their own loads
 * (ADR-0004); analyze() uses the default set: WaterLoad then SelfWeight.
 */

#pragma once

#include <expected>
#include <memory>
#include <vector>

#include "Load.hpp"
#include "Result.hpp"
#include "ValidationError.hpp"

namespace simulation {

class Analyzer {
public:
    explicit Analyzer(std::vector<std::unique_ptr<Load>> loads);
    static Analyzer with_default_loads();

    std::expected<Result, std::vector<ValidationError>> run(const model::Scenario& scenario) const;

private:
    std::vector<std::unique_ptr<Load>> loads_;
};

std::expected<Result, std::vector<ValidationError>> analyze(const model::Scenario& scenario);

} // namespace simulation
