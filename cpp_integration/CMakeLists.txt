# Minimum CMake version required (pybind11 needs ~3.4+, Python package needs ~3.12+)
# Let's use 3.15 for good modern practices and robust Python finding.
cmake_minimum_required(VERSION 3.15)

# Define the project name (can be anything, but often matches the module)
project(cpp_astar LANGUAGES CXX)

# --- C++ Standard ---
# pybind11 requires C++11 or newer. C++17 is a good modern choice.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Optional: Clang-specific flag often useful with pybind11
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(your_target PRIVATE -fno-strict-aliasing)
# endif()

# --- Add pybind11 ---
# Find the pybind11 submodule relative to this CMakeLists.txt location
# This makes pybind11 targets and functions available (like pybind11_add_module)
add_subdirectory(extern/pybind11)

# --- Find Python 3 ---
# This finds the Python interpreter, libraries, and include directories needed for building
# the extension module. Using REQUIRED ensures CMake stops if Python isn't found.
# The COMPONENTS ensure we get what pybind11 needs.
find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Found Python 3 Interpreter: ${Python_EXECUTABLE}")
message(STATUS "Found Python 3 Include Dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Found Python 3 Libraries: ${Python_LIBRARIES}")

# --- Define Source Files ---
# List all the C++ source files that need to be compiled for the module.
# Headers (.h) are NOT listed here; they are found via include_directories.
set(MODULE_SOURCES
    src/test.cpp
)

# --- Define the Python Module Target ---
# This is the core pybind11 command.
# It creates a shared library target with the correct name and suffix for Python
# (e.g., astar_module.cpython-310-x86_64-linux-gnu.so on Linux).
# 'MODULE' creates a Python module (no 'lib' prefix).
# 'SHARED' would create a standard shared library (e.g., libastar_module.so). Use MODULE.
pybind11_add_module(cpp_astar MODULE ${MODULE_SOURCES})

# --- Include Directories ---
# Tell the compiler where to find your project's header files (.h).
# pybind11_add_module typically handles including the Python and pybind11 headers automatically.
target_include_directories(cpp_astar PRIVATE
    # Path relative to this CMakeLists.txt
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# --- Output Location ---
# By default, CMake builds targets into the build directory (CMAKE_BINARY_DIR).
# This is the standard practice (out-of-source builds).
# The compiled module (e.g., .so file) will end up directly inside the build directory you create.
# There is usually NO need to explicitly set LIBRARY_OUTPUT_DIRECTORY unless you have specific requirements.
# Your Python script will need to add this *build* directory to its sys.path.

message(STATUS "The Python module 'astar_module' will be built in the CMake build directory.")
