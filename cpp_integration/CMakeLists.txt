# ==============================================================================
# Project Setup
# ==============================================================================
cmake_minimum_required(VERSION 3.16)
project(assignment2_cpp LANGUAGES CXX)

# --- Global Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True) # Enforce C++17 standard
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Build position-independent code (needed for modules/shared libs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json for tooling

# ==============================================================================
# Compiler Flags
# ==============================================================================

# --- Warning Flags ---
if(MSVC)
  # Microsoft Visual C++ Compiler: Level 4 warnings
  add_compile_options(/W4)
else()
  # GCC / Clang: Enable common warnings
  add_compile_options(-Wall -Wextra -Wpedantic)
  # -fvisibility=hidden reduces exported symbols (good practice for libraries/modules)
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

# --- pybind11 ---
# Assumes pybind11 source is located in extern/pybind11
add_subdirectory(extern/pybind11)

# --- Python ---
find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)

# ==============================================================================
# Library Targets
# ==============================================================================

# --- A* Demo Library (Static Library) ---
add_library(demo_lib src/demo/astar.cpp)
# No need to set PIC property, inherited from global CMAKE_POSITION_INDEPENDENT_CODE
target_include_directories(demo_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include" # Makes 'include/' available to demo_lib and linkers
)
target_link_libraries(demo_lib PRIVATE
    pybind11::headers # Link pybind11 headers (needed if demo uses pybind types, otherwise remove)
    Python::Python    # Link Python libs (needed if demo uses Python C API, otherwise remove)
)


# --- Data Structures Library (Header-Only) ---
add_library(data_structures_lib INTERFACE)
target_include_directories(data_structures_lib INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include" # Makes 'include/' available via this target
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
)

# --- Include Directories for the Python Module ---
# Includes required directly by bindings.cpp
target_include_directories(assignment2_cpp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ==============================================================================
# Build Information Message
# ==============================================================================
message(STATUS "Python module '${PROJECT_NAME}' will be built in the CMake build directory.")
