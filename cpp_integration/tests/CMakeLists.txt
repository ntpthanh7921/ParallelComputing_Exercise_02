# This CMake file is for building the test executable

# Include the GoogleTest module (provides gtest_discover_tests)
include(GoogleTest)

# Define the test executable
add_executable(
  run_set_tests # Name of your test executable
  set_test.cpp  # Source file(s) for the tests
)

# Link the test executable against:
# 1. Google Test's main library (provides main() function and test runner)
# 2. Your data_structures library (to access the set implementations)
target_link_libraries(
  run_set_tests
  PRIVATE
  GTest::gtest_main    # Provided by FetchContent_MakeAvailable(googletest)
  data_structures_lib # Your header-only library defined in the root CMakeLists.txt
)

# Discover tests in the executable and add them to CTest
# Requires CMake 3.10+
gtest_discover_tests(run_set_tests)
