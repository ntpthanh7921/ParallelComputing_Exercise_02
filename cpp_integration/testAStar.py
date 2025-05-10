import sys
import os
import time
import random
import networkx as nx
import osmnx as ox
import collections
import logging
import csv

# --- Configuration ---
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Default GraphML file - consider using argparse for flexibility
GRAPHML_FILENAME = "london_drive_simplified.graphml"
GRAPHML_PATH = os.path.abspath(
    os.path.join(SCRIPT_DIR, "..", "osm_data", GRAPHML_FILENAME)
)
WEIGHT_ATTRIBUTE = "length"  # Edge attribute used for pathfinding weight
# Default CMake preset name to look for the build artifacts
DEFAULT_PRESET_NAME = "release"

SCRIPT_BASE = os.path.splitext(os.path.basename(__file__))[0]
LOG_FILE = f"{SCRIPT_BASE}.log"
CSV_FILE = f"{SCRIPT_BASE}.summary.csv"
NUM_TESTS = 100  # Number of random start/end node pairs to test

# --- Helper: Add Build Directory to Path ---
def add_custom_module_path(preset_name=DEFAULT_PRESET_NAME):
    """Adds the CMake build directory (based on preset) to the Python path."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Construct path based on preset convention: build/<presetName>
    build_dir = os.path.join(script_dir, "build", preset_name)
    if not os.path.isdir(build_dir):
        print(f"Build directory for preset '{preset_name}' not found at {build_dir}")
        print(
            f"Please build the C++ module using the '{preset_name}' preset first (e.g., using 'cmake --build --preset {preset_name}')."
        )
        # Fallback check for old simple build directory
        old_build_dir = os.path.join(script_dir, "build")
        if os.path.isdir(old_build_dir):
            print(
                f"Found legacy build directory at {old_build_dir}. Adding it instead, but preset structure is recommended."
            )
            build_dir = old_build_dir
        else:
            sys.exit(1)

    sys.path.insert(0, build_dir)
    print(f"Added build directory to path: {build_dir}")


# --- Helper: Load Graph ---
def load_graph_from_graphml(graphml_path):
    """Loads a NetworkX graph from a GraphML file using osmnx."""
    print(f"Loading road network from '{graphml_path}'...")
    if not os.path.exists(graphml_path):
        print(f"Error: GraphML file not found at '{graphml_path}'")
        print("Please ensure the file exists or specify the correct path.")
        sys.exit(1)

    start_time = time.time()
    try:
        G_nx = ox.load_graphml(graphml_path)
        print(f"Network loaded from GraphML in {time.time() - start_time:.2f} seconds.")
        print(
            f"Network has {G_nx.number_of_nodes()} nodes and {G_nx.number_of_edges()} edges."
        )
        return G_nx
    except Exception as e:
        print(f"Error loading GraphML file '{graphml_path}': {e}")
        sys.exit(1)


# --- Helper: Prepare Data for C++ Module ---
def prepare_cpp_data(G_nx, weight_attribute):
    """Converts NetworkX graph data to dictionaries suitable for the C++ module."""
    print("Preparing data for C++ module...")
    start_time = time.time()
    nodes_dict = {}
    graph_dict = {}
    missing_coords = 0
    missing_weights = 0

    # Extract node coordinates
    for node, data in G_nx.nodes(data=True):
        if "y" in data and "x" in data:
            nodes_dict[node] = (data["y"], data["x"])  # (lat, lon)
        else:
            missing_coords += 1

    if missing_coords > 0:
        print(f"Warning: {missing_coords} nodes missing coordinate data ('x' or 'y').")

    # Extract edges and weights, handling parallel edges by keeping the lowest weight
    for u, v, data in G_nx.edges(data=True):
        weight = data.get(weight_attribute)
        if weight is None:
            missing_weights += 1
            continue

        if u not in graph_dict:
            graph_dict[u] = []

        # Check if edge (u, v) already exists; update if new weight is lower
        found = False
        for i, (target, existing_weight) in enumerate(graph_dict[u]):
            if target == v:
                if weight < existing_weight:
                    graph_dict[u][i] = (v, weight)
                found = True
                break
        if not found:
            graph_dict[u].append((v, weight))

    if missing_weights > 0:
        print(f"Warning: Skipped {missing_weights} edges missing weight attribute '{weight_attribute}'.")

    # Ensure all nodes from nodes_dict exist as keys in graph_dict (even if no outgoing edges)
    for node_id in nodes_dict:
        if node_id not in graph_dict:
            graph_dict[node_id] = []

    print(f"Data preparation finished in {time.time() - start_time:.2f} seconds.")
    return nodes_dict, graph_dict


# --- Helper: Run C++ Sequential A* Search ---
def run_cpp_sequential_astar(cpp_module, cpp_network, start_node, end_node):
    """Runs the C++ A* implementation and returns the path and execution time."""
    log_lines = []
    log_lines.append("")
    log_lines.append(f"Running C++ Sequential A* implementation...")
    start_time = time.time()
    cpp_path = None
    try:
        # Access the function within the 'demo' submodule
        cpp_path = cpp_module.demo.AStar_search(cpp_network, start_node, end_node)
        cpp_time = time.time() - start_time
        if not cpp_path:  # C++ returns empty list [] if no path
            log_lines.append(f"C++ A*: No path found in {cpp_time:.4f} seconds.")
        else:
            log_lines.append(f"C++ A* found path (length {len(cpp_path)}) in {cpp_time:.4f} seconds.")
    except Exception as e:
        log_lines.append(f"Error running C++ A*: {e}")
        cpp_time = float("inf")  # Indicate failure
        cpp_path = None  # Ensure path is None on error
        
    # Print and log
    for line in log_lines:
        # print(line)
        logging.info(line)
        
    return cpp_path, cpp_time


# --- Helper: Run C++ Parallel A* Search ---
def run_cpp_parallel_astar(cpp_module, cpp_network, start_node, end_node, number_of_threads, parallel_type):
    """Runs the C++ A* implementation and returns the path and execution time."""
    log_lines = []
    log_lines.append("")
    log_lines.append(f"Running C++ Parallel A* implementation with {parallel_type} type and {number_of_threads} threads...")
    start_time = time.time()
    cpp_path = None
    try:
        # Access the function within the 'demo' submodule        
        parallel_dispatch = {
            "TPool___CppLib"  : cpp_module.demo.AStarParallel_search_TPool_CppLib,
            "TVector_CppLib": cpp_module.demo.AStarParallel_search_TVector_CppLib,
            "TPool___PqFine"  : cpp_module.demo.AStarParallel_search_TPool_PqFine,
            "TVector_PqFine": cpp_module.demo.AStarParallel_search_TVector_PqFine,
        }

        search_func = parallel_dispatch[parallel_type]
        cpp_path = search_func(cpp_network, start_node, end_node, number_of_threads)
        cpp_time = time.time() - start_time
        if not cpp_path:  # C++ returns empty list [] if no path
            log_lines.append(f"C++ A*: No path found in {cpp_time:.4f} seconds.")
        else:
            log_lines.append(f"C++ A* found path (length {len(cpp_path)}) in {cpp_time:.4f} seconds.")

    except KeyError:
        raise ValueError(f"Unsupported parallel type: {parallel_type}")
            
    except Exception as e:
        log_lines.append(f"Error running C++ A*: {e}")
        cpp_time = float("inf")  # Indicate failure
        cpp_path = None  # Ensure path is None on error
    
    # Print and log
    for line in log_lines:
        # print(line)
        logging.info(line)
        
    return cpp_path, cpp_time


# --- Helper: Define Heuristic for NetworkX ---
def nx_heuristic(u, v, G_nx):
    """Heuristic function (Haversine distance) for networkx.astar_path."""
    try:
        u_coords = G_nx.nodes[u]
        v_coords = G_nx.nodes[v]
        # Use osmnx great_circle which expects (lat, lon)
        return ox.distance.great_circle(
            u_coords["y"], u_coords["x"], v_coords["y"], v_coords["x"]
        )
    except KeyError as e:
        print(f"Error: Node {e} missing coordinate data ('x' or 'y') for heuristic.")
        return float("inf")  # Return infinity if data is missing


# --- Helper: Run NetworkX A* Search ---
def run_nx_astar(G_nx, start_node, end_node, weight_attribute):
    """Runs the NetworkX A* implementation and returns the path and execution time."""
    log_lines = []
    log_lines.append("")
    log_lines.append("Running NetworkX A* implementation...")
    start_time = time.time()
    nx_path = None
    try:
        # Pass the heuristic function correctly, using a lambda to include G_nx
        nx_path = nx.astar_path(
            G_nx,
            start_node,
            end_node,
            weight=weight_attribute,
            heuristic=lambda u, v: nx_heuristic(u, v, G_nx),  # Pass G_nx context
        )
        nx_time = time.time() - start_time
        log_lines.append(f"NetworkX A* found path (length {len(nx_path)}) in {nx_time:.4f} seconds.")
    except nx.NetworkXNoPath:
        nx_time = time.time() - start_time
        log_lines.append(f"NetworkX A*: No path found in {nx_time:.4f} seconds.")
        nx_path = None  # Ensure path is None
    except Exception as e:
        log_lines.append(f"Error running NetworkX A*: {e}")
        nx_time = float("inf")  # Indicate failure
        nx_path = None  # Ensure path is None on error
    
    # Print and log
    for line in log_lines:
        # print(line)
        logging.info(line)
        
    return nx_path, nx_time


# --- Helper: Compare Results ---
def compare_results(cpp_path, cpp_time, nx_path, nx_time, G_nx, weight_attribute):
    """Compares the paths and timings from both implementations and returns summary info."""
    log_lines = []
    log_lines.append("")
    log_lines.append("--- Comparison ---")
    log_lines.append(f"C++ Time: {cpp_time:.4f} s")
    log_lines.append(f"NX Time:  {nx_time:.4f} s")

    path_match = False
    cpp_path_found = bool(cpp_path)
    nx_path_found = nx_path is not None
    path_cost = None

    # Check if both found a path (cpp_path is non-empty list, nx_path is not None)
    if cpp_path_found and nx_path_found:
        if cpp_path == nx_path:
            log_lines.append("Paths are identical.")
            path_match = True
        else:
            log_lines.append("Paths DIFFER:")
            log_lines.append(f"  C++ Path Length: {len(cpp_path)}")
            log_lines.append(f"  NX Path Length:  {len(nx_path)}")
    elif not cpp_path_found and not nx_path_found:
        log_lines.append("Neither implementation found a path.")
        path_match = True  # This is also considered a match in terms of result
    else:
        log_lines.append("Paths DIFFER: One implementation found a path, the other did not.")
        log_lines.append(
            f"  C++ Path found: {'Yes' if cpp_path_found else 'No'} (Length: {len(cpp_path) if cpp_path_found else 'N/A'})"
        )
        log_lines.append(
            f"  NX Path found:  {'Yes' if nx_path_found else 'No'} (Length: {len(nx_path) if nx_path_found else 'N/A'})"
        )

    if cpp_path_found:
        try:
            path_cost = nx.path_weight(G_nx, cpp_path, weight=weight_attribute)
            log_lines.append(f"Path cost (using '{weight_attribute}'): {path_cost:.2f}")
        except Exception as e:
            log_lines.append(f"Could not calculate path cost: {e}")

    # Print and log
    for line in log_lines:
        # print(line)
        logging.info(line)

    return {
        "cpp_time_ms": cpp_time * 1000,
        "nx_time_ms": nx_time * 1000,
        "cpp_path_len": len(cpp_path) if cpp_path else 0,
        "nx_path_len": len(nx_path) if nx_path else 0,
        "cost": path_cost,
        "match": path_match,
        "cpp_found": cpp_path_found,
        "nx_found": nx_path_found,
    }

# --- Main Execution Block ---
def main():
    # Setup logging once (at top-level or main)
    logging.basicConfig(
        filename=LOG_FILE,
        level=logging.INFO,
        format="%(asctime)s - %(message)s",
        filemode="a"  # Append mode
    )

    # Setup: Add build dir to path (defaults to 'release' preset)
    add_custom_module_path()
    try:
        import assignment2_cpp
        print(f"Successfully imported C++ module 'assignment2_cpp'")
    except ImportError as e:
        print(f"Failed to import 'assignment2_cpp': {e}")
        print("Make sure the module is built and the path is correct.")
        sys.exit(1)

    # Load graph data
    G_nx = load_graph_from_graphml(GRAPHML_PATH)

    # Prepare data structures for C++
    nodes_dict, graph_dict = prepare_cpp_data(G_nx, WEIGHT_ATTRIBUTE)

    # Create C++ RoadNetwork object
    try:
        cpp_road_network = assignment2_cpp.RoadNetwork(graph_dict, nodes_dict)
        print("C++ RoadNetwork object created successfully.")
    except Exception as e:
        print(f"Error creating C++ RoadNetwork object: {e}")
        sys.exit(1)

    # Validate enough nodes
    node_list = list(nodes_dict.keys())
    if len(node_list) < 2:
        print("Error: Not enough nodes with coordinates in the graph to test pathfinding.")
        sys.exit(1)

    # Prepare summary storage
    results_summary = collections.defaultdict(lambda: {
        "total_time_ms": 0.0, "runs": 0, "total_cost": 0.0,
        "total_path_len": 0, "match_count": 0, "num_threads": ""
    })

    # Define test variants
    test_variants = {
        "Sequential": {"fn": lambda *args: run_cpp_sequential_astar(assignment2_cpp, *args), "threads": ""},
    }
    for num_threads in [2, 4, 6]:
        for variant in ["TPool___CppLib", "TPool___PqFine", "TVector_CppLib", "TVector_PqFine"]:
            name = f"{variant}_{num_threads}thr"
            test_variants[name] = {
                "fn": lambda net, s, e, nt=num_threads, v=variant: run_cpp_parallel_astar(
                    assignment2_cpp, net, s, e, nt, v
                ),
                "threads": num_threads
            }

    # Log file for compare_results output
    for i in range(NUM_TESTS):
        start_node, end_node = random.sample(node_list, 2)
        print(f"\nTest {i}: start_node is {start_node}, end_node is {end_node}")

        try:
            nx_path, nx_time = run_nx_astar(G_nx, start_node, end_node, WEIGHT_ATTRIBUTE)
        except Exception as e:
            print(f"Error in NX run {i+1}: {e}")
            continue

        result = compare_results(nx_path, nx_time, nx_path, nx_time, G_nx, WEIGHT_ATTRIBUTE)
        summary = results_summary["NX"]
        summary["total_time_ms"] += result["cpp_time_ms"]
        summary["total_path_len"] += result["cpp_path_len"] if result["cpp_path_len"] is not None else 0
        summary["total_cost"] += result["cost"] if result["cost"] is not None else 0
        summary["match_count"] += int(result["match"])
        summary["runs"] += 1
        summary["num_threads"] = ""

        for name, meta in test_variants.items():
            try:
                cpp_path, cpp_time = meta["fn"](cpp_road_network, start_node, end_node)
                result = compare_results(cpp_path, cpp_time, nx_path, nx_time, G_nx, WEIGHT_ATTRIBUTE)
            except Exception as e:
                print(f"Error in {name} run {i+1}: {e}")
                continue

            summary = results_summary[name]
            summary["total_time_ms"] += result["cpp_time_ms"]
            summary["total_path_len"] += result["cpp_path_len"] if result["cpp_path_len"] is not None else 0
            summary["total_cost"] += result["cost"] if result["cost"] is not None else 0
            summary["match_count"] += int(result["match"])
            summary["runs"] += 1
            summary["num_threads"] = meta["threads"]

    # Print summary
    print(f"\n=== Summary of All Runs (averaged over {NUM_TESTS} pairs) ===")
    print(f"{'Variant':<25} {'Threads':>8} {'Avg Time':>10} {'Avg Len':>10} {'Avg Cost':>10} {'Match %':>12}")
    print("-" * 85)
    for name, s in results_summary.items():
        if s["runs"] == 0:
            continue
        avg_time = s["total_time_ms"] / s["runs"]
        avg_len = s["total_path_len"] / s["runs"]
        avg_cost = s["total_cost"] / s["runs"]
        match_pct = 100.0 * s["match_count"] / s["runs"]
        print(f"{name:<25} {str(s['num_threads']):>8} {avg_time:10.2f} "
              f"{avg_len:10.2f} {avg_cost:10.2f} {match_pct:12.1f}")

    # Write CSV summary
    csv_filename = CSV_FILE
    with open(csv_filename, mode="w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["Variant", "Threads", "Avg Time (ms)", "Avg Path Len", "Avg Cost", "Match (%)"])
        for name, s in results_summary.items():
            if s["runs"] == 0:
                continue
            avg_time = s["total_time_ms"] / s["runs"]
            avg_len = s["total_path_len"] / s["runs"]
            avg_cost = s["total_cost"] / s["runs"]
            match_pct = 100.0 * s["match_count"] / s["runs"]
            writer.writerow([
                name, s["num_threads"], f"{avg_time:.2f}", f"{avg_len:.2f}",
                f"{avg_cost:.2f}", f"{match_pct:.1f}"
            ])
    print(f"\nCSV summary written to '{csv_filename}'")


if __name__ == "__main__":
    main()
