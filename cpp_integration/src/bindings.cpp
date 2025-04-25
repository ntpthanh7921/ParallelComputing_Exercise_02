#include "demo/astar.h"    // Include the A* function header
#include "road_network.h"  // Include the new RoadNetwork class
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(assignment2_cpp, m)
{
    m.doc() = "C++ pathfinding module";

    // --- Bind the RoadNetwork class ---
    py::class_<RoadNetwork>(m, "RoadNetwork")
        // Bind the constructor that takes Python dictionaries
        .def(py::init<const py::dict &, const py::dict &>(), py::arg("graph_dict"),
             py::arg("nodes_dict"),
             "Constructs the RoadNetwork from Python dictionaries.\n"
             "graph_dict format: {node_id: [(neighbor_id, weight), ...]}\n"
             "nodes_dict format: {node_id: (latitude, longitude)}")
        // Add any other methods of RoadNetwork you want to expose, e.g.:
        // .def("get_node", &RoadNetwork::get_node, py::return_value_policy::reference_internal)
        // .def("get_neighbors", &RoadNetwork::get_neighbors,
        // py::return_value_policy::reference_internal)
        ;

    // --- Bindings in the 'demo' submodule ---
    py::module_ demo_m = m.def_submodule("demo", "Submodule for demo implementations");

    demo_m.def("astar_search_demo",
               &Demo::astar_search,  // Directly bind the C++ function
               "Find the shortest path using A* (Demo Implementation)",
               py::arg("network"),  // Expects a RoadNetwork object from Python
               py::arg("start_node"), py::arg("goal_node"),
               py::return_value_policy::move  // Keep move policy for the result vector
    );

    // --- Bind Node/Edge structs (optional, at top level 'm') ---
    py::class_<Node>(m, "Node")
        .def(py::init<long long, double, double>())
        .def_readwrite("id", &Node::id)
        .def_readwrite("lat", &Node::lat)
        .def_readwrite("lon", &Node::lon)
        .def("__repr__", [](const Node &n) { /* ... as before ... */ });

    py::class_<Edge>(m, "Edge")
        .def(py::init<long long, double>())
        .def_readwrite("target_node_id", &Edge::target_node_id)
        .def_readwrite("weight", &Edge::weight)
        .def("__repr__", [](const Edge &e) { /* ... as before ... */ });
}
