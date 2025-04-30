# Include GoogleTest (already present via FetchContent in parent CMakeLists.txt)
include(GoogleTest)

# --- Executable 1: Set Sequential Tests ---
add_executable(
  run_set_sequential_tests      # Target name
  set_sequential_test.cpp   # Source file
)
target_link_libraries(
  run_set_sequential_tests
  PRIVATE
  GTest::gtest_main         # Use gtest_main to provide main()
  data_structures_lib      # Your library target
)
gtest_discover_tests(run_set_sequential_tests)


# --- Executable 2: Set Concurrent Tests ---
add_executable(
  run_set_concurrent_tests      # Target name
  set_concurrent_test.cpp   # Source file
)
# Link against GTest, your library, and potentially Threads if needed explicitly
# (Though modern CMake/C++ usually handle std::thread linkage)
target_link_libraries(
  run_set_concurrent_tests
  PRIVATE
  GTest::gtest_main
  data_structures_lib
  # Threads::Threads # Usually not needed explicitly unless specific issues arise
)
gtest_discover_tests(run_set_concurrent_tests)


# --- Executable 3: Priority Queue Sequential Tests ---
add_executable(
  run_pq_sequential_tests      # Target name
  pq_sequential_test.cpp   # Source file for PQ sequential tests
)
target_link_libraries(
  run_pq_sequential_tests
  PRIVATE
  GTest::gtest_main
  data_structures_lib      # Link against the same library
)
gtest_discover_tests(run_pq_sequential_tests)


# --- Executable 4: Priority Queue Concurrent Tests ---
add_executable(
  run_pq_concurrent_tests      # Target name
  pq_concurrent_test.cpp   # Source file for PQ concurrent tests
)
target_link_libraries(
  run_pq_concurrent_tests
  PRIVATE
  GTest::gtest_main
  data_structures_lib
  # Threads::Threads # Usually not needed explicitly
)
gtest_discover_tests(run_pq_concurrent_tests)
