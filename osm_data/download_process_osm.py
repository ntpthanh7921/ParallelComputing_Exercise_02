import osmnx as ox
import networkx as nx  # osmnx uses NetworkX graphs

print(f"OSMnx version: {ox.__version__}")
print(f"NetworkX version: {nx.__version__}")

# 1. Define the location
# Using a clear query for London, UK helps OSMnx find the correct place.
place_name = "London, UK"
print(f"Target location: {place_name}")

# 2. Download the driving road network
# network_type='drive' fetches roads suitable for driving (excludes footpaths, etc.)
# This process might take a while as London's network is large.
print(f"Downloading driving network for {place_name} from OpenStreetMap...")
try:
    G = ox.graph_from_place(place_name, network_type="drive", simplify=False)
    print("Successfully downloaded graph.")
    print(f"Original graph node count: {len(G.nodes)}")
    print(f"Original graph edge count: {len(G.edges)}")
except Exception as e:
    print(f"An error occurred during download: {e}")
    exit()  # Exit if download fails

# 3. Simplify the network topology
# ox.simplify_graph removes interstitial nodes (nodes that are not intersections
# or dead-ends) and combines edges accordingly. This significantly reduces the
# graph size and complexity, making shortest-path algorithms like A* much faster
# by focusing on decision points (intersections).
print("Simplifying the network topology...")
# Using strict=False is generally recommended for drivability analysis
# to preserve accurate connectivity.
G_simplified = ox.simplify_graph(G)
print("Simplification complete.")
print(f"Simplified graph node count: {len(G_simplified.nodes)}")
print(f"Simplified graph edge count: {len(G_simplified.edges)}")

# 4. Save the simplified network to a GraphML file
# GraphML is a standard XML-based format for graphs.
# Saving the graph allows you to load it quickly later without re-downloading.
output_filename = "london_drive_simplified.graphml"
filepath = f"./{output_filename}"  # Save in the current directory
print(f"Saving the simplified graph to {filepath}...")
try:
    ox.save_graphml(G_simplified, filepath=filepath)
    print(f"Successfully saved the simplified network to {filepath}.")
except Exception as e:
    print(f"An error occurred while saving the graph: {e}")

print("\nProcess finished.")
