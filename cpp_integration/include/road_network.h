#pragma once

#include "graph_types.h"        // Uses Node, Edge, Graph, NodeMap
#include <pybind11/pybind11.h>  // Include for py::dict if needed in constructor/methods
#include <pybind11/stl.h>       // Needed if constructor directly uses stl converters

namespace py = pybind11;

// Helper function (can be here or in a separate .cpp)
// Converts Python dict {id: (lat, lon)} to NodeMap
inline NodeMap convert_py_nodes(const py::dict &py_nodes)
{
    NodeMap nodes;
    nodes.reserve(py_nodes.size());
    for (const auto &item : py_nodes)
    {
        long long node_id = item.first.cast<long long>();
        py::tuple coords = item.second.cast<py::tuple>();
        if (coords.size() == 2)
        {
            nodes.emplace(
                std::piecewise_construct, std::forward_as_tuple(node_id),
                std::forward_as_tuple(node_id, coords[0].cast<double>(), coords[1].cast<double>()));
        }
        else
        {
            throw py::value_error("Node data tuple must contain (latitude, longitude)");
        }
    }
    return nodes;
}

// Helper function (can be here or in a separate .cpp)
// Converts Python dict {id: [(neighbor_id, weight), ...]} to Graph
inline Graph convert_py_graph(const py::dict &py_graph)
{
    Graph graph;
    graph.reserve(py_graph.size());
    for (const auto &item : py_graph)
    {
        long long u_id = item.first.cast<long long>();
        py::list neighbors = item.second.cast<py::list>();
        std::vector<Edge> edges;
        edges.reserve(neighbors.size());
        for (const auto &neighbor_info : neighbors)
        {
            py::tuple info_tuple = neighbor_info.cast<py::tuple>();
            if (info_tuple.size() == 2)
            {
                long long v_id = info_tuple[0].cast<long long>();
                double weight = info_tuple[1].cast<double>();
                edges.emplace_back(v_id, weight);
            }
            else
            {
                throw py::value_error("Neighbor data tuple must contain (target_node_id, weight)");
            }
        }
        graph.emplace(u_id, std::move(edges));
    }
    return graph;
}

class RoadNetwork
{
public:
    // Constructor taking Python dictionaries directly
    RoadNetwork(const py::dict &py_graph, const py::dict &py_nodes)
        : graph_data_(convert_py_graph(py_graph)), node_data_(convert_py_nodes(py_nodes))
    {
        // Optional: Add validation here if needed
    }

    // Deleted copy constructor/assignment to prevent accidental copies
    RoadNetwork(const RoadNetwork &) = delete;
    RoadNetwork &operator=(const RoadNetwork &) = delete;

    // Default move constructor/assignment
    RoadNetwork(RoadNetwork &&) = default;
    RoadNetwork &operator=(RoadNetwork &&) = default;

    // Accessors for the A* algorithm (const references)
    const Graph &get_graph() const { return graph_data_; }

    const NodeMap &get_nodes() const { return node_data_; }

    // Optional: Method to get node details (could be useful)
    const Node *get_node(long long node_id) const
    {
        auto it = node_data_.find(node_id);
        return (it != node_data_.end()) ? &(it->second) : nullptr;
    }

    // Optional: Method to get neighbors (could be useful)
    const std::vector<Edge> *get_neighbors(long long node_id) const
    {
        auto it = graph_data_.find(node_id);
        return (it != graph_data_.end()) ? &(it->second) : nullptr;
    }

private:
    Graph graph_data_;
    NodeMap node_data_;
};
