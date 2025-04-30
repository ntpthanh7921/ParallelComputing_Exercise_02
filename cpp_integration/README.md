# C++ Python Module (assignment2_cpp)

This project contains C++ implementations of data structures (Sets and Priority Queues) and algorithms (A*pathfinding), along with Python bindings using pybind11. It includes unit tests (GoogleTest), benchmarks (Google Benchmark), and a demonstration A* pathfinding algorithm.

## Prerequisites

* **CMake:** Version 3.16 or higher.
* **Ninja:** Required as the build system generator.
* **C++ Compiler:** A C++20 compatible compiler (e.g., GCC, Clang, MSVC).
* **Python:** Python 3 (including development headers).

## Directory Structure

```
cpp_integration/
├── CMakeLists.txt              # Main CMake configuration
├── CMakePresets.json           # CMake presets for configuring/building/testing
├── README.md                   # This file
├── benchmarks/                 # Benchmark code and results
│   ├── CMakeLists.txt          # CMake for benchmarks
│   ├── pq_benchmark.cpp        # Benchmark source for Priority Queue implementations
│   ├── pq_benchmarks_result.json # Default output file for PQ benchmark results
│   ├── set_benchmark.cpp       # Benchmark source for Set implementations
│   └── set_benchmarks_result.json # Default output file for Set benchmark results
├── include/                    # Header files
│   ├── data_structure/         # Core data structure implementations
│   │   ├── ipq.h               # Interface for Priority Queue data structures
│   │   ├── iset.h              # Interface for Set data structures
│   │   ├── pq_fine.h           # Fine-grained locking Priority Queue
│   │   ├── set_coarse.h        # Coarse-grained locking Set
│   │   ├── set_fine.h          # Fine-grained locking Set
│   │   └── set_sequential.h    # Sequential Set
│   ├── demo/                   # Demo algorithm headers
│   │   └── astar.h             # A* algorithm header
│   ├── graph_types.h           # Node/Edge/Graph type definitions
│   └── road_network.h          # RoadNetwork class for graph handling
├── src/                        # Source files
│   ├── bindings.cpp            # pybind11 Python module bindings
│   └── demo/                   # Demo algorithm implementations
│       └── astar.cpp           # A* algorithm implementation
├── test.py                     # Python script to test/compare A* implementations
└── tests/                      # Unit tests (GoogleTest)
    ├── CMakeLists.txt          # CMake for tests
    ├── pq_concurrent_test.cpp  # Tests for concurrent Priority Queue behavior
    ├── pq_sequential_test.cpp  # Tests for sequential Priority Queue logic
    ├── set_concurrent_test.cpp # Tests for concurrent Set behavior
    └── set_sequential_test.cpp # Tests for sequential Set logic
```

## Building the Project

This project uses CMake and presets for configuration and building, utilizing Ninja as the generator.

1. **Configure CMake:**
    Choose a preset from `CMakePresets.json`. For general use and benchmarking, `release` or `release-clang` are recommended. For testing with sanitizers, use presets like `debug-asan`, `debug-tsan`, etc.

    ```bash
    # Example using the 'release' preset
    cmake --preset release
    ```

    This will configure the build in the `build/release/` directory.

2. **Build:**
    Build the project using the same preset. CMake will use Ninja automatically based on the preset.

    ```bash
    # Example using the 'release' preset
    cmake --build --preset release
    ```

    This will compile the C++ code and create the Python module (`assignment2_cpp.so` or similar) in the build directory (`build/release/` in this example).

## Running Tests

Tests use GoogleTest and are managed via CTest using CMake presets. There are separate test suites for Set and Priority Queue implementations, testing both sequential logic and concurrent behavior.

1. **Configure & Build:** Use a debug preset, preferably with sanitizers enabled (TSan for concurrency issues, ASan for memory issues). Recommended presets for testing: `debug-tsan`, `debug-tsan-clang`, `debug-asan`, `debug-asan-clang`.

    ```bash
    # Example using ThreadSanitizer preset with Clang
    cmake --preset debug-tsan-clang
    cmake --build --preset debug-tsan-clang
    ```

2. **Run Tests:** Execute tests using CTest and the chosen preset. CTest will automatically discover and run all defined test executables (`run_set_sequential_tests`, `run_set_concurrent_tests`, `run_pq_sequential_tests`, `run_pq_concurrent_tests`).

    ```bash
    # Example using the 'debug-tsan-clang' preset
    ctest --preset debug-tsan-clang
    ```

## Running Benchmarks

Benchmarks use Google Benchmark. There are separate benchmarks for Set and Priority Queue implementations.

1. **Configure & Build:** Use a release preset for accurate performance measurement (e.g., `release` or `release-clang`).

    ```bash
    # Example using the 'release' preset
    cmake --preset release
    cmake --build --preset release --target run_set_benchmarks
    cmake --build --preset release --target run_pq_benchmarks
    ```

    This builds the benchmark executables and runs them using the respective custom targets (`run_set_benchmarks`, `run_pq_benchmarks`).

2. **Output:**

    * Set benchmark results are saved to `benchmarks/set_benchmarks_result.json`.
    * Priority Queue benchmark results are saved to `benchmarks/pq_benchmarks_result.json`.
        These files are intended to be committed to source control to track performance changes.

## Using the Python Module

The C++ code is exposed to Python via the `assignment2_cpp` module.

Ensure the build directory containing the module (e.g., `build/release/`) is in your `PYTHONPATH` or run Python scripts from the project root directory (`cpp_integration/`) after running the `test.py` script once (which adds the build path).

**Example Usage (derived from `test.py`):**

```python
import sys
import os

# Assume build was done using 'release' preset
build_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), 'build', 'release'))
if build_dir not in sys.path:
    sys.path.insert(0, build_dir) #

try:
    import assignment2_cpp #
    print("Successfully imported assignment2_cpp")
except ImportError as e:
    print(f"Failed to import module: {e}")
    print(f"Ensure the module is built in '{build_dir}' and path is correct.")
    sys.exit(1)

# --- Prepare Data (Example) ---
# These dictionaries mimic the structure needed by RoadNetwork
# Nodes: {node_id: (latitude, longitude)}
nodes_dict = {
    1: (40.7128, -74.0060),  # NYC
    2: (34.0522, -118.2437), # LA
    3: (41.8781, -87.6298)   # Chicago
} #

# Graph: {node_id: [(neighbor_id, weight), ...]}
graph_dict = {
    1: [(3, 790.0)],        # Edge from NYC to Chicago
    2: [(3, 2015.0)],       # Edge from LA to Chicago
    3: [(1, 790.0), (2, 2015.0)] # Edges from Chicago
} #

# --- Use the C++ Module ---
try:
    # Create RoadNetwork object
    cpp_network = assignment2_cpp.RoadNetwork(graph_dict, nodes_dict) #
    print("Created C++ RoadNetwork object.")

    start_node = 1 #
    end_node = 2 # Find path from NYC to LA (via Chicago in this example)

    # Run A* search
    # Note: The demo function is in a submodule 'demo'
    path = assignment2_cpp.demo.astar_search_demo(cpp_network, start_node, end_node) #

    if path:
        print(f"C++ A* Path found: {path}")
    else:
        print("C++ A* found no path.")

except Exception as e:
    print(f"An error occurred: {e}")
```

## Dependencies

External dependencies are managed via CMake's `FetchContent`:

* **pybind11:** v2.13.6 (for Python bindings)
* **GoogleTest:** v1.16.0 (for unit testing)
* **Google Benchmark:** v1.9.2 (for performance benchmarking)
