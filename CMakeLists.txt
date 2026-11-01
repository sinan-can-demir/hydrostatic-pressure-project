cmake_minimum_required(VERSION 3.21)
project(hydrostatic_pressure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HP_BUILD_TESTS "Build the test suite" ON)
option(HP_BUILD_IO "Build the JSON layer (hp_io) and the command-line tool" ON)

# Strict warnings for our own targets only (never for fetched dependencies).
# -ffp-contract=off keeps floating-point results independent of fused multiply-add (ADR-0007).
function(hp_set_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall -Wextra -Wpedantic -Werror -ffp-contract=off
        # Catch2's TEST_CASE uses __COUNTER__, which Clang's pedantic mode flags as a C2y extension.
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c2y-extensions>)
endfunction()

# Header-only layers (dependencies point downward only, NFR-06).
add_library(hp_units INTERFACE)
target_include_directories(hp_units INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/computation)
target_compile_features(hp_units INTERFACE cxx_std_23)

add_library(hp_math STATIC computation/math/Polygon.cpp)
target_link_libraries(hp_math PUBLIC hp_units)
hp_set_warnings(hp_math)

# Materials, scenario model, validation (and later the analyzer).
add_library(hp_core STATIC
    computation/model/DamSection.cpp
    computation/simulation/Validator.cpp
    computation/simulation/WaterLoad.cpp
    computation/simulation/SelfWeight.cpp
    computation/simulation/Resultant.cpp
    computation/simulation/Checks.cpp
    computation/simulation/Analyzer.cpp)
target_link_libraries(hp_core PUBLIC hp_math hp_units)
hp_set_warnings(hp_core)

# Compile every header as the first include of its own translation unit, twice,
# to prove each header is self-contained and has an include guard.
file(GLOB HP_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/computation
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/units/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/math/Point2.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/math/Vector2.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/math/Polygon.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/Validator.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/ValidationError.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/ForceContribution.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/Load.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/PressureDistribution.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/SelfWeight.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/WaterLoad.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/Resultant.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/Checks.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/Result.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/simulation/Analyzer.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/materials/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/model/*.hpp)
set(HP_HEADER_CHECK_SOURCES "")
foreach(header ${HP_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)
    get_filename_component(name ${header} NAME_WE)
    set(tu ${CMAKE_BINARY_DIR}/header_check/${dir}_${name}.cpp)
    file(GENERATE OUTPUT ${tu} CONTENT "#include \"${header}\"\n#include \"${header}\"\n")
    list(APPEND HP_HEADER_CHECK_SOURCES ${tu})
endforeach()
add_library(hp_header_check OBJECT ${HP_HEADER_CHECK_SOURCES})
target_link_libraries(hp_header_check PRIVATE hp_core)
hp_set_warnings(hp_header_check)

# JSON reading/writing and the command line. nlohmann/json is confined to hp_io (ADR-0005);
# hp_core never sees it, so the core builds and tests with -DHP_BUILD_IO=OFF.
if(HP_BUILD_IO)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.12.0
        GIT_SHALLOW    TRUE
        SYSTEM)
    FetchContent_MakeAvailable(nlohmann_json)

    # io/ sits outside computation/ on purpose: hp_core's include path then cannot reach hp_io
    # headers (NFR-06; checked by the build_graph tests).
    add_library(hp_io STATIC io/src/Json.cpp)
    target_include_directories(hp_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/io/include)
    target_link_libraries(hp_io PUBLIC hp_core PRIVATE nlohmann_json::nlohmann_json)
    hp_set_warnings(hp_io)

    add_library(hp_cli_lib STATIC apps/cli/Cli.cpp)
    target_include_directories(hp_cli_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/apps/cli)
    target_link_libraries(hp_cli_lib PUBLIC hp_io)
    hp_set_warnings(hp_cli_lib)

    add_executable(hp_cli apps/cli/main.cpp)
    target_link_libraries(hp_cli PRIVATE hp_cli_lib)
    hp_set_warnings(hp_cli)
endif()

if(HP_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.9.1)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    add_subdirectory(tests)
endif()
