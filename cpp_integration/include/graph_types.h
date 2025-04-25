#pragma once

#include <unordered_map>
#include <vector>

// Represents a node in the graph, potentially with coordinates
struct Node
{
    long long id;
    double lat;  // Latitude
    double lon;  // Longitude

    // Add a default constructor for convenience if needed by containers
    Node() : id(0), lat(0.0), lon(0.0) { }

    Node(long long id_val, double lat_val, double lon_val) : id(id_val), lat(lat_val), lon(lon_val)
    {
    }
};

// Represents a directed edge in the graph
struct Edge
{
    long long target_node_id;
    double weight;  // Cost to traverse the edge (e.g., distance, time)

    // Add a default constructor for convenience if needed by containers
    Edge() : target_node_id(0), weight(0.0) { }

    Edge(long long target_id, double w) : target_node_id(target_id), weight(w) { }
};

// Graph representation using an adjacency list: Node ID -> Vector of outgoing Edges
using Graph = std::unordered_map<long long, std::vector<Edge>>;

// Map from Node ID -> Node details (including coordinates)
using NodeMap = std::unordered_map<long long, Node>;
