include(Catch)

# --- runtime tests ---------------------------------------------------------
add_executable(hp_tests_units
    units/test_units.cpp
    units/test_pressure_length.cpp
    units/test_ac1_chain.cpp)
target_link_libraries(hp_tests_units PRIVATE hp_units Catch2::Catch2WithMain)
hp_set_warnings(hp_tests_units)
catch_discover_tests(hp_tests_units PROPERTIES LABELS "fast")

add_executable(hp_tests_core
    math/test_point_vector.cpp
    model/test_materials_and_scenario.cpp)
target_link_libraries(hp_tests_core PRIVATE hp_core Catch2::Catch2WithMain)
hp_set_warnings(hp_tests_core)
catch_discover_tests(hp_tests_core PROPERTIES LABELS "fast")

# --- compile-fail tests (CF-01, NFR-02) ------------------------------------
# Every forbidden operation lives in forbidden_ops.cpp behind its own CF_<NAME> macro.
# The positive target compiles the file with no macro (proves includes and harness are valid);
# each negative target defines one macro and must FAIL to compile.
set(HP_FORBIDDEN_OPS
    PRESSURE_DIV_LENGTH
    PRESSURE_PLUS_LENGTH
    LENGTH_PLUS_DENSITY
    PRESSURE_TIMES_PRESSURE
    DENSITY_TIMES_LENGTH
    IMPLICIT_LENGTH_FROM_DOUBLE
    PRESSURE_FOR_LENGTH_PARAMETER
    FORCE_PLUS_MOMENT)

add_library(hp_cf_positive OBJECT compile_fail/forbidden_ops.cpp)
target_link_libraries(hp_cf_positive PRIVATE hp_units)
hp_set_warnings(hp_cf_positive)

foreach(op ${HP_FORBIDDEN_OPS})
    add_library(hp_cf_${op} OBJECT EXCLUDE_FROM_ALL compile_fail/forbidden_ops.cpp)
    target_link_libraries(hp_cf_${op} PRIVATE hp_units)
    target_compile_definitions(hp_cf_${op} PRIVATE CF_${op})
    hp_set_warnings(hp_cf_${op})
    add_test(NAME compile_fail_${op}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target hp_cf_${op})
    # The nested builds share one build directory, so they must not run concurrently.
    set_tests_properties(compile_fail_${op} PROPERTIES
        WILL_FAIL TRUE RESOURCE_LOCK hp_cf_build LABELS "fast")
endforeach()
