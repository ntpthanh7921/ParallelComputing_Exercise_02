import osmnx as ox
import networkx as nx
import sys

print(f"OSMnx version: {ox.__version__}")
print(f"NetworkX version: {nx.__version__}")


def download_and_process_location(place_name, output_filename):
    """Downloads, simplifies, and saves the road network for a given location."""
    print("-" * 30)
    print(f"Processing: {place_name}")
    print("-" * 30)

    # 1. Download the driving road network
    print(f"Downloading driving network for {place_name} from OpenStreetMap...")
    try:
        G = ox.graph_from_place(place_name, network_type="drive", simplify=False)
        print("Successfully downloaded graph.")
        print(f"Original graph node count: {len(G.nodes)}")
        print(f"Original graph edge count: {len(G.edges)}")
    except Exception as e:
        print(f"An error occurred during download for {place_name}: {e}")
        return  # Skip to next location if download fails

    # 2. Simplify the network topology
    print("Simplifying the network topology...")
    try:
        # Using strict=False is generally recommended for drivability analysis
        G_simplified = ox.simplify_graph(G)
        print("Simplification complete.")
        print(f"Simplified graph node count: {len(G_simplified.nodes)}")
        print(f"Simplified graph edge count: {len(G_simplified.edges)}")
    except Exception as e:
        print(f"An error occurred during simplification for {place_name}: {e}")
        return  # Skip if simplification fails

    # 3. Save the simplified network to a GraphML file
    filepath = f"./{output_filename}"  # Save in the current directory
    print(f"Saving the simplified graph to {filepath}...")
    try:
        ox.save_graphml(G_simplified, filepath=filepath)
        print(f"Successfully saved the simplified network to {filepath}.")
    except Exception as e:
        print(f"An error occurred while saving the graph for {place_name}: {e}")

    print(f"\nFinished processing {place_name}.")


# --- Configuration for Locations ---
locations = {
    "shinjuku": {
        "place_name": "Shinjuku, Tokyo, Japan",
        "output_filename": "shinjuku_tokyo_drive_simplified.graphml",
    },
    "london": {
        "place_name": "London, UK",
        "output_filename": "london_drive_simplified.graphml",
    },
}

# --- Interactive Choice ---
selected_locations = []
while True:
    print("\nSelect which road network(s) to download and process from OpenStreetMap:")
    print("1: Shinjuku, Tokyo (Test Data)")
    print("2: London, UK (Full Data)")
    print("3: Both Shinjuku and London")
    print("4: Exit")
    choice = input("Enter your choice (1, 2, 3, or 4): ").strip()

    if choice == "1":
        selected_locations = ["shinjuku"]
        break
    elif choice == "2":
        selected_locations = ["london"]
        break
    elif choice == "3":
        selected_locations = ["shinjuku", "london"]
        break
    elif choice == "4":
        print("Exiting script.")
        sys.exit()
    else:
        print("Invalid choice. Please enter 1, 2, 3, or 4.")

# --- Main Execution ---
if not selected_locations:
    print("No locations selected. Exiting.")
else:
    print(f"\nSelected locations: {', '.join(selected_locations)}")
    print("Starting download and processing...")
    print("Note: Downloading large areas like London can take significant time.")

    for key in selected_locations:
        if key in locations:
            config = locations[key]
            download_and_process_location(
                config["place_name"], config["output_filename"]
            )
        else:
            print(f"Warning: Configuration for '{key}' not found. Skipping.")

    print("\nOverall process finished.")
