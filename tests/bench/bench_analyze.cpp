// N-03 (NFR-08): advisory benchmark. Analyses a 1000-vertex section and reports the best of
// several runs against the 10 ms budget. It never fails the build: timings depend on the
// machine and on optimisation flags, so the number is information, not a gate.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

#include "math/Point2.hpp"
#include "model/Scenario.hpp"
#include "simulation/Analyzer.hpp"
#include "simulation/Validator.hpp"
#include "units/Angle.hpp"

int main() {
    using math::Point2;
    using units::Length;

    constexpr int kDownstreamPoints = 997;  // + heel, toe, upstream crest = 1000 vertices
    const double base = 60.0;
    const double height = 100.0;

    std::vector<Point2> v{{Length(0), Length(0)}, {Length(base), Length(0)}};
    for (int i = 1; i <= kDownstreamPoints; ++i) {
        const double t = static_cast<double>(i) / kDownstreamPoints;
        v.push_back({Length(base * (1.0 - 0.6 * std::pow(t, 1.5))), Length(height * t)});
    }
    v.push_back({Length(0), Length(height)});

    const model::Scenario scenario{.section = model::DamSection{v},
                                   .water_level = model::WaterLevel{Length(0.9 * height)},
                                   .foundation = model::Foundation{units::Angle::from_degrees(40.0)}};

    auto best_of_5 = [](auto&& work) {
        double best = 1e300;
        for (int run = 0; run < 5; ++run) {
            const auto start = std::chrono::steady_clock::now();
            work();
            const auto stop = std::chrono::steady_clock::now();
            best = std::min(best, std::chrono::duration<double, std::milli>(stop - start).count());
        }
        return best;
    };
    const double validate_ms = best_of_5([&] { (void)simulation::validate(scenario); });

    double best_ms = 1e300;
    bool ok = true;
    for (int run = 0; run < 5; ++run) {
        const auto start = std::chrono::steady_clock::now();
        const auto result = simulation::analyze(scenario);
        const auto stop = std::chrono::steady_clock::now();
        ok = ok && result.has_value();
        best_ms = std::min(best_ms, std::chrono::duration<double, std::milli>(stop - start).count());
    }

    std::printf("N-03: of that, validate() alone: %.3f ms\n", validate_ms);
    std::printf("N-03: analyze() of %zu vertices: best of 5 = %.3f ms (budget 10 ms) [%s]%s\n", v.size(), best_ms,
                best_ms <= 10.0 ? "within budget" : "OVER BUDGET",
                ok ? "" : " -- the benchmark section was rejected as invalid");
    return ok ? 0 : 1;
}
