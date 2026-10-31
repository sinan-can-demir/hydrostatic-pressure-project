// Compile-fail harness (CF-01, NFR-02). Built with no macro it must compile cleanly;
// built with exactly one CF_<NAME> macro it must fail to compile.
#include "units/Acceleration.hpp"
#include "units/Angle.hpp"
#include "units/Area.hpp"
#include "units/Density.hpp"
#include "units/ForcePerWidth.hpp"
#include "units/Length.hpp"
#include "units/MomentPerWidth.hpp"
#include "units/Pressure.hpp"
#include "units/UnitWeight.hpp"

using namespace units;

namespace {

[[maybe_unused]] Length takes_length(Length l) { return l; }

[[maybe_unused]] void harness() {
    const Length l(2.0);
    const Pressure p(3.0);
    const Density rho(1000.0);
    const ForcePerWidth f(4.0);
    const MomentPerWidth m(5.0);

    // Allowed counterparts: these must keep compiling.
    [[maybe_unused]] ForcePerWidth ok1 = p * l;
    [[maybe_unused]] Pressure ok2 = p + p;
    [[maybe_unused]] MomentPerWidth ok3 = f * l;
    [[maybe_unused]] Length ok4 = takes_length(l);
    [[maybe_unused]] Length ok5 = Length(1.0);
    [[maybe_unused]] ForcePerWidth ok6 = f + f;
    [[maybe_unused]] MomentPerWidth ok7 = m + m;
    [[maybe_unused]] Density ok8 = rho;

#ifdef CF_PRESSURE_DIV_LENGTH
    [[maybe_unused]] auto bad = p / l;
#endif
#ifdef CF_PRESSURE_PLUS_LENGTH
    [[maybe_unused]] auto bad = p + l;
#endif
#ifdef CF_LENGTH_PLUS_DENSITY
    [[maybe_unused]] auto bad = l + rho;
#endif
#ifdef CF_PRESSURE_TIMES_PRESSURE
    [[maybe_unused]] auto bad = p * p;
#endif
#ifdef CF_DENSITY_TIMES_LENGTH
    [[maybe_unused]] auto bad = rho * l;
#endif
#ifdef CF_IMPLICIT_LENGTH_FROM_DOUBLE
    [[maybe_unused]] Length bad = 3.0;
#endif
#ifdef CF_PRESSURE_FOR_LENGTH_PARAMETER
    [[maybe_unused]] auto bad = takes_length(p);
#endif
#ifdef CF_FORCE_PLUS_MOMENT
    [[maybe_unused]] auto bad = f + m;
#endif
}

} // namespace
