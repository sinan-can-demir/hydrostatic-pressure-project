// Includes every unit header twice to verify include guards and self-containment.
#include "units/Acceleration.hpp"
#include "units/Density.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

#include "units/Acceleration.hpp"
#include "units/Density.hpp"
#include "units/Length.hpp"
#include "units/Pressure.hpp"

static_assert(units::hydrostatic(units::Density(1000.0), units::Acceleration(9.81), units::Length(1.0)).pascals == 9810.0);
