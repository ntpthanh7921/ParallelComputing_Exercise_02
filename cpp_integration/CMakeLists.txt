# ==============================================================================
# Project Setup
# ==============================================================================
cmake_minimum_required(VERSION 3.16)
project(assignment2_cpp LANGUAGES CXX)

# --- Global Settings ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For tooling

# --- Sanitizer Options ---
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# --- Enable Testing ---
enable_testing()

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

# --- Sanitizer Flags Setup ---
if(NOT MSVC)
  if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ASAN and TSAN cannot be enabled simultaneously.")
  endif()

  set(SANITIZER_COMPILE_FLAGS "")
  set(SANITIZER_LINK_FLAGS "")

  if(ENABLE_ASAN)
    list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=address")
    list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=address")
    message(STATUS "AddressSanitizer (ASAN) enabled.")
  endif()

  if(ENABLE_UBSAN)
    list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=undefined")
    list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=undefined")
    message(STATUS "UndefinedBehaviorSanitizer (UBSAN) enabled.")
  endif()

  if(ENABLE_TSAN)
    list(APPEND SANITIZER_COMPILE_FLAGS "-fsanitize=thread")
    list(APPEND SANITIZER_LINK_FLAGS "-fsanitize=thread")
    message(STATUS "ThreadSanitizer (TSAN) enabled.")
  endif()

  # Apply Sanitizer Flags
  if(SANITIZER_COMPILE_FLAGS)
    add_compile_options(${SANITIZER_COMPILE_FLAGS})
    add_link_options(${SANITIZER_LINK_FLAGS})
    # Recommend -O0 -g for sanitizer builds
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -g")
  endif()
else()
  if(ENABLE_ASAN OR ENABLE_UBSAN OR ENABLE_TSAN)
    message(WARNING "Sanitizers not automatically configured for MSVC.")
  endif()
endif()

# --- Optimization Flags (LTO Example) ---
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
  string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -flto=auto")
  string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE " -flto=auto")
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
