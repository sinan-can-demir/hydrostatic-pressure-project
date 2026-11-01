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
    math/test_polygon.cpp
    model/test_materials_and_scenario.cpp
    model/test_dam_section.cpp
    simulation/test_validator.cpp
    simulation/test_loads.cpp
    simulation/test_stability.cpp
    simulation/test_properties.cpp)
target_link_libraries(hp_tests_core PRIVATE hp_core Catch2::Catch2WithMain)
hp_set_warnings(hp_tests_core)
catch_discover_tests(hp_tests_core PROPERTIES LABELS "fast")

# --- JSON layer and command line (L4) ----------------------------------------
# The tests inspect output structurally, so they link the JSON library too; only hp_io
# (and these tests) ever include it.
if(HP_BUILD_IO)
    add_executable(hp_tests_io io/test_json.cpp)
    target_link_libraries(hp_tests_io PRIVATE hp_io nlohmann_json::nlohmann_json Catch2::Catch2WithMain)
    target_compile_definitions(hp_tests_io PRIVATE HP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
    hp_set_warnings(hp_tests_io)
    catch_discover_tests(hp_tests_io PROPERTIES LABELS "fast")

    add_executable(hp_tests_cli cli/test_cli.cpp)
    target_link_libraries(hp_tests_cli PRIVATE hp_cli_lib nlohmann_json::nlohmann_json Catch2::Catch2WithMain)
    target_compile_definitions(hp_tests_cli PRIVATE HP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
    hp_set_warnings(hp_tests_cli)
    catch_discover_tests(hp_tests_cli PROPERTIES LABELS "fast")

    # End to end: the real executable on the AC-1 document must exit 0 and report a verdict.
    add_test(NAME cli_smoke COMMAND hp_cli ${CMAKE_CURRENT_SOURCE_DIR}/data/ac1_scenario.json)
    set_tests_properties(cli_smoke PROPERTIES PASS_REGULAR_EXPRESSION "\"verdict\": \"fail\"" LABELS "fast")
endif()

# --- advisory benchmark (N-03, NFR-08) ---------------------------------------
# Prints a timing; fails only if the benchmark section itself is invalid. Label "advisory" keeps it
# out of `ctest -L fast`. Timings are only meaningful in an optimised build (-DCMAKE_BUILD_TYPE=Release).
add_executable(hp_bench_analyze bench/bench_analyze.cpp)
target_link_libraries(hp_bench_analyze PRIVATE hp_core)
hp_set_warnings(hp_bench_analyze)
add_test(NAME bench_N-03_NFR-08_analyze_1000_vertices COMMAND hp_bench_analyze)
set_tests_properties(bench_N-03_NFR-08_analyze_1000_vertices PROPERTIES LABELS "advisory")

# --- build-graph tests (N-04, NFR-06) ---------------------------------------
# hp_core must not be able to include an hp_io header or the JSON library. The positive probe
# compiles; each negative probe adds one forbidden include and must fail to compile.
if(HP_BUILD_IO)
    add_library(hp_bg_positive OBJECT build_graph/core_probe.cpp)
    target_link_libraries(hp_bg_positive PRIVATE hp_core)
    hp_set_warnings(hp_bg_positive)

    foreach(probe IO_HEADER JSON_LIBRARY)
        add_library(hp_bg_${probe} OBJECT EXCLUDE_FROM_ALL build_graph/core_probe.cpp)
        target_link_libraries(hp_bg_${probe} PRIVATE hp_core)
        target_compile_definitions(hp_bg_${probe} PRIVATE CF_${probe})
        hp_set_warnings(hp_bg_${probe})
        add_test(NAME build_graph_NFR-06_core_cannot_include_${probe}
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target hp_bg_${probe})
        set_tests_properties(build_graph_NFR-06_core_cannot_include_${probe} PROPERTIES
            WILL_FAIL TRUE RESOURCE_LOCK hp_cf_build LABELS "fast")
    endforeach()
endif()

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
