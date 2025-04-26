# ==============================================================================
# Project Setup
# ==============================================================================
cmake_minimum_required(VERSION 3.16)
project(assignment2_cpp LANGUAGES CXX)

# --- Global Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True) # Enforce C++17 standard
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Needed for modules/shared libs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For tooling

# --- Enable Testing ---
enable_testing()

# ==============================================================================
# Compiler Flags
# ==============================================================================

# --- Warning Flags ---
if(MSVC)
  # Microsoft Visual C++ Compiler
  add_compile_options(/W4)
else()
  # GCC / Clang
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options(-fvisibility=hidden)
endif()

# --- Optimization Flags ---
# Enable Link-Time Optimization (LTO) for Release builds on GCC/Clang
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
  string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE " -flto=auto")
endif()

# ==============================================================================
# Dependencies
# ==============================================================================
include(FetchContent) # Ensure FetchContent module is included

# --- pybind11 (via FetchContent) ---
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.13.6 # Or other desired release tag
)
FetchContent_MakeAvailable(pybind11)

# --- Python ---
find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)

# --- GoogleTest (via FetchContent) ---
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0 # Or other desired release tag
)
FetchContent_MakeAvailable(googletest)

# ==============================================================================
# Library Targets
# ==============================================================================

# --- A* Demo Library (Static Library) ---
add_library(demo_lib STATIC src/demo/astar.cpp)
target_include_directories(demo_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
# Link pybind11 headers because astar.h includes road_network.h which uses pybind11 types
target_link_libraries(demo_lib PRIVATE
    pybind11::headers
    Python::Python
)

# --- Data Structures Library (Header-Only) ---
add_library(data_structures_lib INTERFACE)
target_include_directories(data_structures_lib INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ==============================================================================
# Main Python Module Target
# ==============================================================================
set(MODULE_SOURCES
    src/bindings.cpp
)
pybind11_add_module(assignment2_cpp MODULE ${MODULE_SOURCES})

# --- Linking for the Python Module ---
target_link_libraries(assignment2_cpp PRIVATE
    demo_lib            # Link the A* implementation
    data_structures_lib # Link the header-only data structures (propagates include dirs)
    # pybind11 targets (like pybind11::module) are handled by pybind11_add_module
)

# --- Include Directories for the Python Module ---
target_include_directories(assignment2_cpp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ==============================================================================
# Testing Setup
# ==============================================================================
# Add the tests subdirectory (contains test executable definitions)
add_subdirectory(tests)

# ==============================================================================
# Build Information Message
# ==============================================================================
message(STATUS "Python module '${PROJECT_NAME}' will be built in the CMake build directory.")
