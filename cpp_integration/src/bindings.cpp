#include "demo/astar.h"    // A* algorithm implementation
#include "demo/aStarWithDynamicCostFunction.h"
#include "graph_types.h"   // Node, Edge definitions
#include "road_network.h"  // RoadNetwork class definition

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // Automatic conversion for STL containers (vector, map)

#include <sstream>  // Required for std::ostringstream in __repr__

namespace py = pybind11;

// ==============================================================================
// Module Definition
// ==============================================================================
PYBIND11_MODULE(assignment2_cpp, m)
{
    m.doc() = "C++ pathfinding module with A* implementation and graph structures";

    // ==========================================================================
    // Graph Structure Bindings (Optional but useful for inspection)
    // ==========================================================================

    // --- Bind Node struct ---
    py::class_<Node>(m, "Node", "Represents a node in the graph with ID and coordinates")
        .def(py::init<long long, double, double>(), py::arg("id") = 0, py::arg("lat") = 0.0,
             py::arg("lon") = 0.0, "Node constructor")
        .def_readwrite("id", &Node::id, "Unique node identifier")
        .def_readwrite("lat", &Node::lat, "Latitude coordinate")
        .def_readwrite("lon", &Node::lon, "Longitude coordinate")
        .def(
            "__repr__",
            [](const Node &n)
            {
                std::ostringstream oss;
                oss << "<Node id=" << n.id << " lat=" << n.lat << " lon=" << n.lon << ">";
                return oss.str();
            },
            "String representation of the Node");

    // --- Bind Edge struct ---
    py::class_<Edge>(m, "Edge", "Represents a directed edge with target node and weight")
        .def(py::init<long long, double>(), py::arg("target_node_id") = 0, py::arg("weight") = 0.0,
             "Edge constructor")
        .def_readwrite("target_node_id", &Edge::target_node_id,
                       "ID of the node this edge points to")
        .def_readwrite("weight", &Edge::weight, "Cost associated with traversing this edge")
        .def(
            "__repr__",
            [](const Edge &e)
            {
                std::ostringstream oss;
                oss << "<Edge target=" << e.target_node_id << " weight=" << e.weight << ">";
                return oss.str();
            },
            "String representation of the Edge");

    // ==========================================================================
    // RoadNetwork Class Binding
    // ==========================================================================
    py::class_<RoadNetwork>(m, "RoadNetwork", "Manages the road network graph and node data")
        // Bind the constructor taking Python dictionaries
        .def(py::init<const py::dict &, const py::dict &>(), py::arg("graph_dict"),
             py::arg("nodes_dict"),
             R"(Constructs the RoadNetwork from Python dictionaries.
                graph_dict format: {node_id: [(neighbor_id, weight), ...]}
                nodes_dict format: {node_id: (latitude, longitude)})")

        // Bind accessor methods (useful for inspection from Python)
        // Use reference_internal policy: Python gets access but C++ (RoadNetwork) owns the memory
        .def("get_node", &RoadNetwork::get_node, py::return_value_policy::reference_internal,
             "Get Node details by ID, returns None if not found.", py::arg("node_id"))
        .def("get_neighbors", &RoadNetwork::get_neighbors,
             py::return_value_policy::reference_internal,
             "Get a list of outgoing Edges for a node ID, returns None if node not found.",
             py::arg("node_id"));

    // ==========================================================================
    // Algorithm Bindings (within a submodule)
    // ==========================================================================
    py::module_ demo_m = m.def_submodule("demo", "Submodule for demo algorithm implementations");

    // --- Bind A* search function ---
    demo_m.def("astar_search_demo",
               &Demo::astar_search,  // The C++ function to bind
               "Find the shortest path using the A* algorithm (Demo Implementation). Returns a "
               "list of node IDs.",
               py::arg("network"),            // Expects a RoadNetwork object from Python
               py::arg("start_node"),         // Starting node ID
               py::arg("goal_node"),          // Goal node ID
               py::return_value_policy::move  // Efficiently move the resulting vector to Python
    );

    demo_m.def("astar_search_demo_with_dynamic_cost_function",
        &AStarEnhancement::astar_search,  // The C++ function to bind
        "Find the shortest path using the A* algorithm (Demo Implementation). Returns a "
        "list of node IDs.",
        py::arg("network"),            // Expects a RoadNetwork object from Python
        py::arg("start_node"),         // Starting node ID
        py::arg("goal_node"),          // Goal node ID
        py::return_value_policy::move  // Efficiently move the resulting vector to Python
);

    demo_m.def("astar_search_sequential",
               &Demo::Sequential_astar_search,  // The C++ function to bind
               "Find the shortest path using the A* algorithm (Demo Implementation). Returns a "
               "list of node IDs.",
               py::arg("network"),            // Expects a RoadNetwork object from Python
               py::arg("start_node"),         // Starting node ID
               py::arg("goal_node"),          // Goal node ID
               py::return_value_policy::move  // Efficiently move the resulting vector to Python
    );

    demo_m.def("astar_search_parallel",
               &Demo::Parallel_astar_search,  // The C++ function to bind
               "Find the shortest path using the A* algorithm (Demo Implementation). Returns a "
               "list of node IDs.",
               py::arg("network"),            // Expects a RoadNetwork object from Python
               py::arg("start_node"),         // Starting node ID
               py::arg("goal_node"),          // Goal node ID
               py::arg("num_threads"),         // Number of threads
               py::return_value_policy::move  // Efficiently move the resulting vector to Python
    );
}
