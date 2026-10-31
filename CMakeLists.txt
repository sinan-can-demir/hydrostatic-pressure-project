cmake_minimum_required(VERSION 3.20)
project(hydrostatic_pressure LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only unit types for now; real targets are added as the design is approved.
add_library(hp_units INTERFACE)
target_include_directories(hp_units INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/computation)
target_compile_features(hp_units INTERFACE cxx_std_20)

# Compile-check each unit header standalone (catches missing includes/guards).
add_library(hp_units_check OBJECT computation/units/check_headers.cpp)
target_link_libraries(hp_units_check PRIVATE hp_units)
target_compile_options(hp_units_check PRIVATE -Wall -Wextra -Wpedantic -Werror)
