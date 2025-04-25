cmake_minimum_required(VERSION 3.15)
project(assignment2_cpp LANGUAGES CXX) # Renamed project

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
  string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE " -flto=auto")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(extern/pybind11)

find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Using Python: ${Python_EXECUTABLE}")
message(STATUS "Using Python includes: ${Python_INCLUDE_DIRS}")

# --- A* Demo Library ---
add_library(astar_demo_lib src/demo/astar.cpp)
set_target_properties(astar_demo_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(astar_demo_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(astar_demo_lib PRIVATE
    pybind11::headers
    Python::Python
)

# --- Main Python Module ---
set(MODULE_SOURCES
    src/bindings.cpp
)
pybind11_add_module(assignment2_cpp MODULE ${MODULE_SOURCES}) # Renamed module

# Link the Python module against the A* library
target_link_libraries(assignment2_cpp PRIVATE astar_demo_lib)

# Include directories for the main module (bindings.cpp)
target_include_directories(assignment2_cpp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

message(STATUS "The Python module '${PROJECT_NAME}' will be built in the CMake build directory.")

if (NOT MSVC)
  target_compile_options(assignment2_cpp PRIVATE "-fvisibility=hidden") # Renamed module
  target_compile_options(astar_demo_lib PRIVATE "-fvisibility=hidden")
endif()
