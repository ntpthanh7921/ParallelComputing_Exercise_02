# ==============================================================================
# Project Setup
# ==============================================================================
cmake_minimum_required(VERSION 3.16)
project(assignment2_cpp LANGUAGES CXX)

# --- Global Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For tooling like clang-tidy

# --- Enable Testing ---
enable_testing()

# ==============================================================================
# Tooling Setup (Clang-Tidy)
# ==============================================================================
find_program(CLANG_TIDY_EXE clang-tidy)
if(NOT CLANG_TIDY_EXE)
  message(WARNING "clang-tidy not found - static analysis skipped.")
endif()

# ==============================================================================
# Compiler Flags
# ==============================================================================

# --- Warning Flags ---
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options(-fvisibility=hidden)
endif()

# --- Optimization Flags (LTO Example) ---
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
  string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE " -flto=auto")
endif()

# ==============================================================================
# Apply clang-tidy (if found)
# ==============================================================================
# Applies to all targets defined after this point.
# Assumes a .clang-tidy file exists in the project root for configuration.
if(CLANG_TIDY_EXE)
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
  message(STATUS "clang-tidy found, enabling static analysis.")
endif()

# ==============================================================================
# Dependencies
# ==============================================================================
include(FetchContent)

# --- pybind11 ---
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.13.6 # Specify desired release tag
)
FetchContent_MakeAvailable(pybind11)

# --- Python ---
find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)

# --- GoogleTest ---
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0 # Specify desired release tag
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
    demo_lib
    data_structures_lib
)

# --- Include Directories for the Python Module ---
target_include_directories(assignment2_cpp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ==============================================================================
# Testing Setup
# ==============================================================================
add_subdirectory(tests)

# ==============================================================================
# Build Information Message
# ==============================================================================
message(STATUS "Python module '${PROJECT_NAME}' will be built in the CMake build directory.")
