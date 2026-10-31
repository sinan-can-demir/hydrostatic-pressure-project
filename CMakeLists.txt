cmake_minimum_required(VERSION 3.21)
project(hydrostatic_pressure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HP_BUILD_TESTS "Build the test suite" ON)

# Strict warnings for our own targets only (never for fetched dependencies).
# -ffp-contract=off keeps floating-point results independent of fused multiply-add (ADR-0007).
function(hp_set_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall -Wextra -Wpedantic -Werror -ffp-contract=off)
endfunction()

# Header-only unit types.
add_library(hp_units INTERFACE)
target_include_directories(hp_units INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/computation)
target_compile_features(hp_units INTERFACE cxx_std_23)

# Compile every unit header as the first include of its own translation unit, twice,
# to prove each header is self-contained and has an include guard.
file(GLOB HP_UNIT_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/computation
     ${CMAKE_CURRENT_SOURCE_DIR}/computation/units/*.hpp)
set(HP_HEADER_CHECK_SOURCES "")
foreach(header ${HP_UNIT_HEADERS})
    get_filename_component(name ${header} NAME_WE)
    set(tu ${CMAKE_BINARY_DIR}/header_check/${name}.cpp)
    file(GENERATE OUTPUT ${tu} CONTENT "#include \"${header}\"\n#include \"${header}\"\n")
    list(APPEND HP_HEADER_CHECK_SOURCES ${tu})
endforeach()
add_library(hp_units_check OBJECT ${HP_HEADER_CHECK_SOURCES})
target_link_libraries(hp_units_check PRIVATE hp_units)
hp_set_warnings(hp_units_check)

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
