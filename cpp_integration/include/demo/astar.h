#pragma once

#include "../graph_types.h"   // Node/Edge types used by heuristic/Graph
#include "../road_network.h"  // RoadNetwork class header
#include <cmath>
#include <limits>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AStar {

    inline double heuristic(const Node &a, const Node &b)
    {
        // Haversine implementation
        if (a.id == 0 || b.id == 0)
        {
            return std::numeric_limits<double>::max();
        }
        double lat1_rad = a.lat * M_PI / 180.0;
        double lon1_rad = a.lon * M_PI / 180.0;
        double lat2_rad = b.lat * M_PI / 180.0;
        double lon2_rad = b.lon * M_PI / 180.0;

        double dlon = lon2_rad - lon1_rad;
        double dlat = lat2_rad - lat1_rad;

        double haversine_a =
            pow(sin(dlat / 2.0), 2) + cos(lat1_rad) * cos(lat2_rad) * pow(sin(dlon / 2.0), 2);
        double c = 2.0 * asin(sqrt(haversine_a));

        double r = 6371.0;  // Earth radius in kilometers
        return c * r;
    }

    std::vector<long long> search(const RoadNetwork &network,
                                        long long start_node_id, long long goal_node_id);
} 

namespace AStarParallel {

    inline double heuristic(const Node &a, const Node &b)
    {
        // Haversine implementation
        if (a.id == 0 || b.id == 0)
        {
            return std::numeric_limits<double>::max();
        }
        double lat1_rad = a.lat * M_PI / 180.0;
        double lon1_rad = a.lon * M_PI / 180.0;
        double lat2_rad = b.lat * M_PI / 180.0;
        double lon2_rad = b.lon * M_PI / 180.0;

        double dlon = lon2_rad - lon1_rad;
        double dlat = lat2_rad - lat1_rad;

        double haversine_a =
            pow(sin(dlat / 2.0), 2) + cos(lat1_rad) * cos(lat2_rad) * pow(sin(dlon / 2.0), 2);
        double c = 2.0 * asin(sqrt(haversine_a));

        double r = 6371.0;  // Earth radius in kilometers
        return c * r;
    }

    std::vector<long long> search_TPool_CppLib(const RoadNetwork& network,
                                        long long start_node_id, long long goal_node_id, int NUM_THREADS);

    std::vector<long long> search_TVector_CppLib(const RoadNetwork &network, 
                                        long long start_node_id, long long goal_node_id, int NUM_THREADS);

    std::vector<long long> search_TPool_PqFine(const RoadNetwork& network,
                                            long long start_node_id, long long goal_node_id, int NUM_THREADS);

    std::vector<long long> search_TVector_PqFine(const RoadNetwork &network, 
                                        long long start_node_id, long long goal_node_id, int NUM_THREADS);

}
