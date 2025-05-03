#include "demo/astar.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>  // For runtime_error
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>

namespace Demo
{

    struct AStarNode
    {
        long long id;
        double f_score;

        bool operator>(const AStarNode &other) const { return f_score > other.f_score; }
    };

    std::vector<long long> astar_search(const RoadNetwork &network,  // Accepts RoadNetwork
                                        long long start_node_id, long long goal_node_id)
    {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr)
        {
            throw std::runtime_error("Start node ID not found in NodeMap.");
        }
        if (!goal_node_ptr)
        {
            throw std::runtime_error("Goal node ID not found in NodeMap.");
        }
        const Node &start_node_data = *start_node_ptr;
        const Node &goal_node_data = *goal_node_ptr;

        // Priority queue, g_score, came_from setup (as before)
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
        std::unordered_map<long long, double> g_score;
        std::unordered_map<long long, long long> came_from;

        // Initialize g_scores - only need to track nodes encountered
        // Set start node g_score
        g_score[start_node_id] = 0.0;

        // Add start node to the open set
        open_set.push({start_node_id, heuristic(start_node_data, goal_node_data)});

        while (!open_set.empty())
        {
            AStarNode current = open_set.top();
            open_set.pop();
            long long current_id = current.id;

            // Goal reached (same as before)
            if (current_id == goal_node_id)
            {
                std::vector<long long> path;
                long long temp = current_id;
                // Check count using find to avoid creating entry if temp is not found
                while (came_from.find(temp) != came_from.end())
                {
                    path.push_back(temp);
                    temp = came_from[temp];
                }
                path.push_back(start_node_id);
                std::reverse(path.begin(), path.end());
                return path;
            }

            // Get current node g_score, default to infinity if not present (should be present if
            // reached via open_set)
            double current_g_score =
                (g_score.count(current_id)) ? g_score[current_id] : std::numeric_limits<double>::max();

            // Check if the current node exists in the graph's adjacency list
            const std::vector<Edge> *neighbors_ptr = network.get_neighbors(current_id);
            if (!neighbors_ptr)
            {
                continue;  // Node has no outgoing edges
            }

            // Explore neighbors
            for (const Edge &edge : *neighbors_ptr)
            {
                long long neighbor_id = edge.target_node_id;
                double tentative_g_score = current_g_score + edge.weight;

                // Get neighbor g_score, default to infinity if not seen before
                double neighbor_g_score = (g_score.count(neighbor_id))
                                            ? g_score[neighbor_id]
                                            : std::numeric_limits<double>::max();

                if (tentative_g_score < neighbor_g_score)
                {
                    // Found a better path
                    came_from[neighbor_id] = current_id;
                    g_score[neighbor_id] = tentative_g_score;  // Update or insert g_score

                    const Node *neighbor_node_ptr = network.get_node(neighbor_id);
                    if (neighbor_node_ptr)
                    {  // Check if neighbor node data exists
                        double h_score = heuristic(*neighbor_node_ptr, goal_node_data);
                        // Check for potential infinity issues if heuristic returns max()
                        if (h_score < std::numeric_limits<double>::max())
                        {
                            open_set.push({neighbor_id, tentative_g_score + h_score});
                        }  // else: node is likely unreachable or heuristic failed, don't add
                    }
                    else
                    {
                        // Node exists in graph edges but not in NodeMap - indicates inconsistent data
                        // Option: Throw an error, log a warning, or assign max heuristic
                        // For now, let's skip adding it to open_set if data is missing
                        // std::cerr << "Warning: Node " << neighbor_id << " missing coordinate data."
                        // << std::endl;
                    }
                }
            }
        }

        // Open set empty, goal not reached
        return {};
    }

    std::vector<long long> Sequential_astar_search(const RoadNetwork &network,  // Accepts RoadNetwork
                                        long long start_node_id, long long goal_node_id)
    {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr)
        {
            throw std::runtime_error("Start node ID not found in NodeMap.");
        }
        if (!goal_node_ptr)
        {
            throw std::runtime_error("Goal node ID not found in NodeMap.");
        }
        const Node &start_node_data = *start_node_ptr;
        const Node &goal_node_data = *goal_node_ptr;

        // Priority queue, g_score, came_from setup (as before)
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
        std::unordered_map<long long, double> g_score;
        std::unordered_map<long long, long long> came_from;

        // Initialize g_scores - only need to track nodes encountered
        // Set start node g_score
        g_score[start_node_id] = 0.0;

        // Add start node to the open set
        open_set.push({start_node_id, heuristic(start_node_data, goal_node_data)});

        while (!open_set.empty())
        {
            AStarNode current = open_set.top();
            open_set.pop();
            long long current_id = current.id;

            // Goal reached (same as before)
            if (current_id == goal_node_id)
            {
                std::vector<long long> path;
                long long temp = current_id;
                // Check count using find to avoid creating entry if temp is not found
                while (came_from.find(temp) != came_from.end())
                {
                    path.push_back(temp);
                    temp = came_from[temp];
                }
                path.push_back(start_node_id);
                std::reverse(path.begin(), path.end());
                return path;
            }

            // Get current node g_score, default to infinity if not present (should be present if
            // reached via open_set)
            double current_g_score =
                (g_score.count(current_id)) ? g_score[current_id] : std::numeric_limits<double>::max();

            // Check if the current node exists in the graph's adjacency list
            const std::vector<Edge> *neighbors_ptr = network.get_neighbors(current_id);
            if (!neighbors_ptr)
            {
                continue;  // Node has no outgoing edges
            }

            // Explore neighbors
            for (const Edge &edge : *neighbors_ptr)
            {
                long long neighbor_id = edge.target_node_id;
                double tentative_g_score = current_g_score + edge.weight;

                // Get neighbor g_score, default to infinity if not seen before
                double neighbor_g_score = (g_score.count(neighbor_id))
                                            ? g_score[neighbor_id]
                                            : std::numeric_limits<double>::max();

                if (tentative_g_score < neighbor_g_score)
                {
                    // Found a better path
                    came_from[neighbor_id] = current_id;
                    g_score[neighbor_id] = tentative_g_score;  // Update or insert g_score

                    const Node *neighbor_node_ptr = network.get_node(neighbor_id);
                    if (neighbor_node_ptr)
                    {  // Check if neighbor node data exists
                        double h_score = heuristic(*neighbor_node_ptr, goal_node_data);
                        // Check for potential infinity issues if heuristic returns max()
                        if (h_score < std::numeric_limits<double>::max())
                        {
                            open_set.push({neighbor_id, tentative_g_score + h_score});
                        }  // else: node is likely unreachable or heuristic failed, don't add
                    }
                    else
                    {
                        // Node exists in graph edges but not in NodeMap - indicates inconsistent data
                        // Option: Throw an error, log a warning, or assign max heuristic
                        // For now, let's skip adding it to open_set if data is missing
                        // std::cerr << "Warning: Node " << neighbor_id << " missing coordinate data."
                        // << std::endl;
                    }
                }
            }
        }

        // Open set empty, goal not reached
        return {};
    }

    std::mutex pq_mutex;
    std::mutex um_mutex_0, um_mutex_1;

    void neighbor_search_task(std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> &open_set,
                              std::unordered_map<long long, double> &g_score, 
                              std::unordered_map<long long, long long> &came_from,
                              const RoadNetwork &network, const std::vector<Edge> &neighbors,
                              long long start, long long end, double current_g_score, long long current_id,
                              const Node &goal_node_data) 
    {

        for (long long i = start; i < end; ++i) 
        {
            const Edge& edge = neighbors[i];
                    
            long long neighbor_id = edge.target_node_id;
            double tentative_g_score = current_g_score + edge.weight;

            // Get neighbor g_score, default to infinity if not seen before
            double neighbor_g_score = (g_score.count(neighbor_id))
                                        ? g_score[neighbor_id]
                                        : std::numeric_limits<double>::max();

            if (tentative_g_score < neighbor_g_score)
            {
                // Found a better path
                {
                    std::lock_guard<std::mutex> lock(um_mutex_0);
                    came_from[neighbor_id] = current_id;
                }
                {
                    std::lock_guard<std::mutex> lock(um_mutex_1);
                    g_score[neighbor_id] = tentative_g_score;  // Update or insert g_score
                }

                const Node *neighbor_node_ptr = network.get_node(neighbor_id);
                if (neighbor_node_ptr)
                {  // Check if neighbor node data exists
                    double h_score = heuristic(*neighbor_node_ptr, goal_node_data);
                    // Check for potential infinity issues if heuristic returns max()
                    if (h_score < std::numeric_limits<double>::max())
                    {
                        std::lock_guard<std::mutex> lock(pq_mutex);
                        open_set.push({neighbor_id, tentative_g_score + h_score});
                    }  // else: node is likely unreachable or heuristic failed, don't add
                }
                else
                {
                    // Node exists in graph edges but not in NodeMap - indicates inconsistent data
                    // Option: Throw an error, log a warning, or assign max heuristic
                    // For now, let's skip adding it to open_set if data is missing
                    // std::cerr << "Warning: Node " << neighbor_id << " missing coordinate data."
                    // << std::endl;
                }
            }            
        }
    }

    std::vector<long long> Parallel_astar_search(const RoadNetwork &network,  // Accepts RoadNetwork
                                        long long start_node_id, long long goal_node_id, int NUM_THREADS)
    {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr)
        {
            throw std::runtime_error("Start node ID not found in NodeMap.");
        }
        if (!goal_node_ptr)
        {
            throw std::runtime_error("Goal node ID not found in NodeMap.");
        }
        const Node &start_node_data = *start_node_ptr;
        const Node &goal_node_data = *goal_node_ptr;

        // Priority queue, g_score, came_from setup (as before)
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
        std::unordered_map<long long, double> g_score;
        std::unordered_map<long long, long long> came_from;

        // Initialize g_scores - only need to track nodes encountered
        // Set start node g_score
        g_score[start_node_id] = 0.0;

        // Add start node to the open set
        open_set.push({start_node_id, heuristic(start_node_data, goal_node_data)});

        while (!open_set.empty())
        {
            AStarNode current = open_set.top();
            open_set.pop();
            long long current_id = current.id;

            // Goal reached (same as before)
            if (current_id == goal_node_id)
            {
                std::vector<long long> path;
                long long temp = current_id;
                // Check count using find to avoid creating entry if temp is not found
                while (came_from.find(temp) != came_from.end())
                {
                    path.push_back(temp);
                    temp = came_from[temp];
                }
                path.push_back(start_node_id);
                std::reverse(path.begin(), path.end());
                return path;
            }

            // Get current node g_score, default to infinity if not present (should be present if
            // reached via open_set)
            double current_g_score =
                (g_score.count(current_id)) ? g_score[current_id] : std::numeric_limits<double>::max();

            // Check if the current node exists in the graph's adjacency list
            const std::vector<Edge> *neighbors_ptr = network.get_neighbors(current_id);
            if (!neighbors_ptr)
            {
                continue;  // Node has no outgoing edges
            }

            // Parallel explore
            std::vector<std::thread> threads;
            const std::vector<Edge>& neighbors = *neighbors_ptr;

            long long total = neighbors.size();
            long long chunk_size = (total + NUM_THREADS - 1) / NUM_THREADS;
            for (int t = 0; t < NUM_THREADS; ++t) 
            {
                long long start = t * chunk_size;
                long long end = std::min(start + chunk_size, total);

                threads.emplace_back(neighbor_search_task,
                         std::ref(open_set),
                         std::ref(g_score),
                         std::ref(came_from),
                         std::ref(network),
                         std::ref(neighbors),
                         start, end,
                         current_g_score,
                         current_id,
                         std::ref(goal_node_data));
                
                // neighbor_search_task(open_set, g_score, came_from, network, neighbors,
                //                         start, end, current_g_score, current_id, goal_node_data);
                // threads.emplace_back([&open_set, &g_score, &came_from, &network, &neighbors,
                //                         start, end, current_g_score, current_id, &goal_node_data]() 
                // {
                //     for (long long i = start; i < end; ++i) 
                //     {
                //         const Edge& edge = neighbors[i];
                    
                //         long long neighbor_id = edge.target_node_id;
                //         double tentative_g_score = current_g_score + edge.weight;

                //         // Get neighbor g_score, default to infinity if not seen before
                //         double neighbor_g_score = (g_score.count(neighbor_id))
                //                                     ? g_score[neighbor_id]
                //                                     : std::numeric_limits<double>::max();

                //         if (tentative_g_score < neighbor_g_score)
                //         {
                //             // Found a better path
                //             {
                //                 std::lock_guard<std::mutex> lock(um_mutex_0);
                //                 came_from[neighbor_id] = current_id;
                //             }
                //             {
                //                 std::lock_guard<std::mutex> lock(um_mutex_1);
                //                 g_score[neighbor_id] = tentative_g_score;  // Update or insert g_score
                //             }

                //             const Node *neighbor_node_ptr = network.get_node(neighbor_id);
                //             if (neighbor_node_ptr)
                //             {  // Check if neighbor node data exists
                //                 double h_score = heuristic(*neighbor_node_ptr, goal_node_data);
                //                 // Check for potential infinity issues if heuristic returns max()
                //                 if (h_score < std::numeric_limits<double>::max())
                //                 {
                //                     std::lock_guard<std::mutex> lock(pq_mutex);
                //                     open_set.push({neighbor_id, tentative_g_score + h_score});
                //                 }  // else: node is likely unreachable or heuristic failed, don't add
                //             }
                //             else
                //             {
                //                 // Node exists in graph edges but not in NodeMap - indicates inconsistent data
                //                 // Option: Throw an error, log a warning, or assign max heuristic
                //                 // For now, let's skip adding it to open_set if data is missing
                //                 // std::cerr << "Warning: Node " << neighbor_id << " missing coordinate data."
                //                 // << std::endl;
                //             }
                //         }
                    
                //     }
                // });
            }

            for (auto& t : threads)
                t.join();
            }

        // Open set empty, goal not reached
        return {};
    }

}  // namespace Demo