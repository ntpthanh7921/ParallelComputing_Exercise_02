import sys
import os
import time
import random
import networkx as nx
import osmnx as ox

# --- Configuration ---
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Default GraphML file - consider using argparse for flexibility
GRAPHML_FILENAME = "shinjuku_tokyo_drive_simplified.graphml"
GRAPHML_PATH = os.path.abspath(
    os.path.join(SCRIPT_DIR, "..", "osm_data", GRAPHML_FILENAME)
)
WEIGHT_ATTRIBUTE = "length"  # Edge attribute used for pathfinding weight


# --- Helper: Add Build Directory to Path ---
def add_custom_module_path(build_dir_name="build"):
    """Adds the CMake build directory to the Python path."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(script_dir, build_dir_name)
    if not os.path.isdir(build_dir):
        print(f"Build directory not found at {build_dir}")
        print("Please build the C++ module first (e.g., using CMake).")
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
        print(
            f"Warning: Skipped {missing_weights} edges missing weight attribute '{weight_attribute}'."
        )

    # Ensure all nodes from nodes_dict exist as keys in graph_dict (even if no outgoing edges)
    for node_id in nodes_dict:
        if node_id not in graph_dict:
            graph_dict[node_id] = []

    print(f"Data preparation finished in {time.time() - start_time:.2f} seconds.")
    return nodes_dict, graph_dict


# --- Helper: Run C++ A* Search ---
def run_cpp_astar(cpp_module, cpp_network, start_node, end_node):
    """Runs the C++ A* implementation and returns the path and execution time."""
    print("\nRunning C++ A* implementation...")
    start_time = time.time()
    cpp_path = None
    try:
        # Access the function within the 'demo' submodule
        cpp_path = cpp_module.demo.astar_search_demo(cpp_network, start_node, end_node)
        cpp_time = time.time() - start_time
        if not cpp_path:  # C++ returns empty list [] if no path
            print(f"C++ A*: No path found in {cpp_time:.4f} seconds.")
        else:
            print(
                f"C++ A* found path (length {len(cpp_path)}) in {cpp_time:.4f} seconds."
            )
    except Exception as e:
        print(f"Error running C++ A*: {e}")
        cpp_time = float("inf")  # Indicate failure
        cpp_path = None  # Ensure path is None on error
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
    print("\nRunning NetworkX A* implementation...")
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
        print(
            f"NetworkX A* found path (length {len(nx_path)}) in {nx_time:.4f} seconds."
        )
    except nx.NetworkXNoPath:
        nx_time = time.time() - start_time
        print(f"NetworkX A*: No path found in {nx_time:.4f} seconds.")
        nx_path = None  # Ensure path is None
    except Exception as e:
        print(f"Error running NetworkX A*: {e}")
        nx_time = float("inf")  # Indicate failure
        nx_path = None  # Ensure path is None on error
    return nx_path, nx_time


# --- Helper: Compare Results ---
def compare_results(cpp_path, cpp_time, nx_path, nx_time, G_nx, weight_attribute):
    """Compares the paths and timings from both implementations."""
    print("\n--- Comparison ---")
    print(f"C++ Time: {cpp_time:.4f} s")
    print(f"NX Time:  {nx_time:.4f} s")

    # Check if both found a path (cpp_path is non-empty list, nx_path is not None)
    if cpp_path and nx_path is not None:
        if cpp_path == nx_path:
            print("Paths are identical.")
            try:
                path_cost = nx.path_weight(G_nx, cpp_path, weight=weight_attribute)
                print(f"Path cost (using '{weight_attribute}'): {path_cost:.2f}")
            except Exception as e:
                print(f"Could not calculate path cost: {e}")
        else:
            print("Paths DIFFER:")
            print(f"  C++ Path Length: {len(cpp_path)}")
            print(f"  NX Path Length:  {len(nx_path)}")

    # Check if NEITHER found a path (cpp_path is empty/None, nx_path is None)
    elif (not cpp_path or cpp_path is None) and nx_path is None:
        print("Neither implementation found a path.")

    # Otherwise, one found a path and the other did not
    else:
        print("Paths DIFFER: One implementation found a path, the other did not.")
        print(
            f"  C++ Path found: {'Yes' if cpp_path else 'No'} (Length: {len(cpp_path) if cpp_path else 'N/A'})"
        )
        print(
            f"  NX Path found:  {'Yes' if nx_path is not None else 'No'} (Length: {len(nx_path) if nx_path is not None else 'N/A'})"
        )


# --- Main Execution Block ---
def main():
    # Setup: Add build dir to path and import C++ module
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

    # Select random start/end nodes
    node_list = list(nodes_dict.keys())
    if len(node_list) < 2:
        print(
            "Error: Not enough nodes with coordinates in the graph to test pathfinding."
        )
        sys.exit(1)
    start_node = random.choice(node_list)
    end_node = random.choice(node_list)
    while start_node == end_node:  # Ensure start and end are different
        end_node = random.choice(node_list)
    print(f"\nSelected random nodes:\n  Start: {start_node}\n  End:   {end_node}")

    # Run A* implementations
    cpp_path, cpp_time = run_cpp_astar(
        assignment2_cpp, cpp_road_network, start_node, end_node
    )
    nx_path, nx_time = run_nx_astar(G_nx, start_node, end_node, WEIGHT_ATTRIBUTE)

    # Compare results
    compare_results(cpp_path, cpp_time, nx_path, nx_time, G_nx, WEIGHT_ATTRIBUTE)


if __name__ == "__main__":
    main()
