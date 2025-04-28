# Include GoogleTest (already present)
include(GoogleTest)

# --- Executable 1: Sequential Tests ---
add_executable(
  run_sequential_tests      # Name for the sequential test executable
  set_sequential_test.cpp   # Source file for sequential tests
)

# Link sequential tests against GTest and your library
target_link_libraries(
  run_sequential_tests
  PRIVATE
  GTest::gtest_main         # Use gtest_main to provide main() function
  data_structures_lib      # Your library target
)

# Discover tests within the sequential executable
gtest_discover_tests(run_sequential_tests)

# --- Executable 2: Concurrent Tests ---
add_executable(
  run_concurrent_tests      # Name for the concurrent test executable
  set_concurrent_test.cpp   # Source file for concurrent tests
)

# Link concurrent tests against GTest and your library
# Note: If concurrent tests require <thread>, ensure C++ standard supports it (C++17 is set)
# and link against pthreads if necessary on POSIX systems (CMake often handles this automatically)
target_link_libraries(
  run_concurrent_tests
  PRIVATE
  GTest::gtest_main
  data_structures_lib
  # Threads::Threads # Usually not needed explicitly with modern CMake/C++11 threads
)

# Discover tests within the concurrent executable
gtest_discover_tests(run_concurrent_tests)
