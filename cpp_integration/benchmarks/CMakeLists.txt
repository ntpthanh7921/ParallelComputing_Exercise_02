# Find Threads package (needed by Google Benchmark for threading support)
find_package(Threads REQUIRED)

# Add the benchmark executable with the new name
add_executable(
  set_benchmarks_executable    # New name for the benchmark executable
  set_benchmark.cpp            # Source file for benchmarks
)

# Link benchmark executable against Google Benchmark, Threads, and your library
target_link_libraries(
  set_benchmarks_executable    # Use the new name here
  PRIVATE
  benchmark::benchmark         # Google Benchmark library target
  Threads::Threads             # Threads library
  data_structures_lib          # Your set implementations library (defined in parent CMakeLists.txt)
)

# Optional: Improve executable location (places it in build/benchmarks)
set_target_properties(set_benchmarks_executable PROPERTIES # Use the new name here
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks"
)

# --- Custom Target to Run Benchmarks ---
add_custom_target(
  run_set_benchmarks  # Name of the custom target (can stay the same or change)
  COMMAND         # Command to execute
      ${CMAKE_COMMAND} -E echo "Running benchmarks..." # Optional: Print a message
  COMMAND         # The actual benchmark command
      $<TARGET_FILE:set_benchmarks_executable> # Use the new executable target name
      # Add default arguments for Google Benchmark here if desired
      --benchmark_out_format=json
      --benchmark_out=${CMAKE_SOURCE_DIR}/benchmarks/set_benchmarks_result.json
      --benchmark_repetitions=5
  WORKING_DIRECTORY # Set the working directory
      ${CMAKE_BINARY_DIR}/benchmarks # Where the executable is usually placed
  DEPENDS           # Ensure the new benchmark executable is built first
      set_benchmarks_executable # Use the new name here
  COMMENT           # Message printed during build
      "Running set benchmarks..."
  VERBATIM          # Pass commands as-is
)

message(STATUS "Benchmark executable 'set_benchmarks_executable' will be built.") # Updated message
message(STATUS "Run benchmarks using: cmake --build <build_dir> --target run_benchmarks")
