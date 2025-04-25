import sys
import os
import time
import random
import networkx as nx
import osmnx as ox


# --- Function to add the build directory to Python's path ---
def add_custom_module_path():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(script_dir, "build")
    if not os.path.isdir(build_dir):
        print(f"Build directory not found at {build_dir}")
        print("Please build the C++ module first (e.g., using CMake).")
        sys.exit(1)
    sys.path.insert(0, build_dir)
    print(f"Added build directory to path: {build_dir}")


add_custom_module_path()

# --- Import the C++ module ---
try:
    import assignment2_cpp

    print(f"Successfully imported C++ module 'assignment2_cpp'")
except ImportError as e:
    print(f"Failed to import 'assignment2_cpp': {e}")
    print("Make sure the module is built and the path is correct.")
    sys.exit(1)

# --- Parameters ---
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GRAPHML_FILENAME = "shinjuku_tokyo_drive_simplified.graphml"
GRAPHML_PATH = os.path.abspath(
    os.path.join(SCRIPT_DIR, "..", "osm_data", GRAPHML_FILENAME)
)
WEIGHT_ATTRIBUTE = "length"

# --- Load OSM Data from GraphML file ---
print(f"Loading road network from '{GRAPHML_PATH}' using osmnx...")
# ... (loading code as before) ...
if not os.path.exists(GRAPHML_PATH):
    print(f"Error: GraphML file not found at '{GRAPHML_PATH}'")
    print(
        f"Please ensure the file exists. You might need to run one of the download scripts in 'osm_data'."
    )
    sys.exit(1)

start_time = time.time()
try:
    G_nx = ox.load_graphml(GRAPHML_PATH)
    print(f"Network loaded from GraphML in {time.time() - start_time:.2f} seconds.")
    print(
        f"Network has {G_nx.number_of_nodes()} nodes and {G_nx.number_of_edges()} edges."
    )
except Exception as e:
    print(f"Error loading GraphML file '{GRAPHML_PATH}': {e}")
    sys.exit(1)


# --- Prepare Data for C++ ---
print("Preparing data for C++ module...")
start_time = time.time()
try:
    nodes_dict = {
        node: (data["y"], data["x"])
        for node, data in G_nx.nodes(data=True)
        if "y" in data and "x" in data
    }
    if len(nodes_dict) != G_nx.number_of_nodes():
        print(
            "Warning: Some nodes in the loaded graph are missing coordinate data ('x' or 'y')."
        )

    graph_dict = {}
    missing_weights = 0
    for u, v, data in G_nx.edges(data=True):
        weight = data.get(WEIGHT_ATTRIBUTE)
        if weight is None:
            missing_weights += 1
            continue
        if u not in graph_dict:
            graph_dict[u] = []
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
            f"Warning: Skipped {missing_weights} edges missing the weight attribute '{WEIGHT_ATTRIBUTE}'."
        )

    for node_id in nodes_dict:
        if node_id not in graph_dict:
            graph_dict[node_id] = []

    cpp_road_network = assignment2_cpp.RoadNetwork(graph_dict, nodes_dict)
    print(f"C++ RoadNetwork object created in {time.time() - start_time:.2f} seconds.")
except Exception as e:
    print(f"Error creating C++ RoadNetwork object: {e}")
    sys.exit(1)

# --- Choose Random Start/End Nodes ---
node_list = list(nodes_dict.keys())
if len(node_list) < 2:
    print("Error: Not enough nodes with coordinates in the graph.")
    sys.exit(1)
start_node = random.choice(node_list)
end_node = random.choice(node_list)
while start_node == end_node:
    end_node = random.choice(node_list)
print(f"\nSelected random nodes:\n  Start: {start_node}\n  End:   {end_node}")


# --- Define Heuristic Wrapper for NetworkX A* ---
def nx_heuristic(u, v):
    """Heuristic function for networkx.astar_path"""
    # NetworkX A* calls heuristic with (node, goal_node)
    # We need lat/lon for both from our graph G_nx
    try:
        u_coords = G_nx.nodes[u]
        v_coords = G_nx.nodes[v]
        # Use Haversine distance (great circle) like in the C++ version
        return ox.distance.great_circle(
            u_coords["y"], u_coords["x"], v_coords["y"], v_coords["x"]
        )
    except KeyError as e:
        print(
            f"Error: Node {e} missing coordinate data ('x' or 'y') for heuristic calculation."
        )
        # Return a large value or handle as appropriate if node data might be missing
        return float("inf")


# --- Run C++ A* ---
print("\nRunning C++ A* implementation...")
start_time = time.time()
cpp_path = None  # Initialize in case of exception
try:
    cpp_path = assignment2_cpp.demo.astar_search_demo(
        cpp_road_network, start_node, end_node
    )
    cpp_time = time.time() - start_time
    if not cpp_path:
        print("C++ A*: No path found.")
    else:
        print(f"C++ A* found path (length {len(cpp_path)}) in {cpp_time:.4f} seconds.")
except Exception as e:
    print(f"Error running C++ A*: {e}")
    cpp_time = float("inf")


# --- Run osmnx A* (NetworkX backend) ---
print("\nRunning osmnx (NetworkX) A* implementation...")
start_time = time.time()
nx_path = None  # Initialize in case of exception
try:
    # ---> Use the wrapper function here <---
    nx_path = nx.astar_path(
        G_nx, start_node, end_node, weight=WEIGHT_ATTRIBUTE, heuristic=nx_heuristic
    )  # Pass the wrapper
    nx_time = time.time() - start_time
    if not nx_path:
        print("osmnx A*: No path found (should raise exception instead).")
    else:
        print(f"osmnx A* found path (length {len(nx_path)}) in {nx_time:.4f} seconds.")
except nx.NetworkXNoPath:
    print("osmnx A*: No path found.")
    nx_time = time.time() - start_time
except Exception as e:
    print(f"Error running osmnx A*: {e}")
    nx_time = float("inf")


# --- Compare Results ---
# ... (Comparison code remains the same) ...
print("\n--- Comparison ---")
print(f"C++ Time: {cpp_time:.4f} s")
print(f"NX Time:  {nx_time:.4f} s")

if cpp_path is not None and nx_path is not None:
    if cpp_path == nx_path:
        print("Paths are identical.")
        try:
            cpp_cost = nx.path_weight(G_nx, cpp_path, weight=WEIGHT_ATTRIBUTE)
            print(f"Path cost (using '{WEIGHT_ATTRIBUTE}'): {cpp_cost:.2f}")
        except Exception as e:
            print(f"Could not calculate path cost: {e}")
    else:
        print("Paths DIFFER:")
        print(f"  C++ Path Length: {len(cpp_path)}")
        print(f"  NX Path Length:  {len(nx_path)}")
        limit = 10
        print(f"  C++ Path (first {limit}): {cpp_path[:limit]}...")
        print(f"  NX Path (first {limit}):  {nx_path[:limit]}...")
elif cpp_path is None and nx_path is None:
    print("Neither implementation found a path.")
else:
    print("One implementation found a path, the other did not.")
