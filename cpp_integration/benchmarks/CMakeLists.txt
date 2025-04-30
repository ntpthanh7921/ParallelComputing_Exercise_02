# Find Threads package (needed by Google Benchmark for threading support)
find_package(Threads REQUIRED)

# --- Set Benchmark ---
add_executable(
  set_benchmarks_executable
  set_benchmark.cpp
)

target_link_libraries(
  set_benchmarks_executable
  PRIVATE
  benchmark::benchmark
  Threads::Threads
  data_structures_lib
)

set_target_properties(set_benchmarks_executable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks"
)

add_custom_target(
  run_set_benchmarks
  COMMAND ${CMAKE_COMMAND} -E echo "Running Set benchmarks..."
  COMMAND $<TARGET_FILE:set_benchmarks_executable>
      --benchmark_out_format=json
      --benchmark_out=${CMAKE_SOURCE_DIR}/benchmarks/set_benchmarks_result.json
      --benchmark_repetitions=5
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
  DEPENDS set_benchmarks_executable
  COMMENT "Running set benchmarks..."
  VERBATIM
)

# --- Priority Queue Benchmark (NEW) ---
add_executable(
  pq_benchmarks_executable     # New executable name
  pq_benchmark.cpp            # New source file
)

target_link_libraries(
  pq_benchmarks_executable    # Link the new executable
  PRIVATE
  benchmark::benchmark
  Threads::Threads
  data_structures_lib        # Also links against your library (for custom PQ)
)

set_target_properties(pq_benchmarks_executable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks"
)

add_custom_target(
  run_pq_benchmarks           # New custom target name
  COMMAND ${CMAKE_COMMAND} -E echo "Running Priority Queue benchmarks..."
  COMMAND $<TARGET_FILE:pq_benchmarks_executable> # Run the new executable
      # Use different output file for PQ results
      --benchmark_out_format=json
      --benchmark_out=${CMAKE_SOURCE_DIR}/benchmarks/pq_benchmarks_result.json
      --benchmark_repetitions=5 # Or adjust as needed
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
  DEPENDS pq_benchmarks_executable  # Depends on the new executable
  COMMENT "Running priority queue benchmarks..."
  VERBATIM
)


message(STATUS "Set benchmark executable 'set_benchmarks_executable' will be built.")
message(STATUS "Run set benchmarks using: cmake --build <build_dir> --target run_set_benchmarks")
message(STATUS "Priority Queue benchmark executable 'pq_benchmarks_executable' will be built.")
message(STATUS "Run PQ benchmarks using: cmake --build <build_dir> --target run_pq_benchmarks")
