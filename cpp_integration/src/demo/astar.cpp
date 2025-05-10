#include "demo/astar.h"
#include "data_structure/pq_fine.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <functional>
#include <atomic>
#include <iostream>

namespace AStar {

    struct AStarNode {
        long long id;
        double f_score;

        bool operator>(const AStarNode &other) const { return f_score > other.f_score; }
    };

    std::vector<long long> search(const RoadNetwork &network,  // Accepts RoadNetwork
                                        long long start_node_id, long long goal_node_id)
    {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr) throw std::runtime_error("Start node ID not found in NodeMap.");
        if (!goal_node_ptr) throw std::runtime_error("Goal node ID not found in NodeMap.");
        
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
            if (!neighbors_ptr) continue;  // Node has no outgoing edges

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

} 

namespace AStarParallel {

    class ThreadPool {

        using Task = std::function<void()>;

    private:
        std::vector<std::thread> workers;
        std::queue<Task> tasks;

        std::mutex queue_mutex;
        std::condition_variable condition;
        std::atomic<bool> stop;

    public:
        ThreadPool(size_t threads) : stop(false) {
            for (size_t i = 0; i < threads; ++i) {
                workers.emplace_back([this] {
                    while (true) {
                        Task task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);
                            condition.wait(lock, [this] {
                                return stop || !tasks.empty();
                            });
                            if (stop && tasks.empty()) return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
            using return_type = decltype(f(args...));

            auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<return_type> res = task_ptr->get_future();
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
                tasks.emplace([task_ptr]() { (*task_ptr)(); });
            }
            condition.notify_one();
            return res;
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            for (auto& worker : workers) worker.join();
        }
    };

    struct AStarNode {
        long long id;
        double f_score;

        bool operator>(const AStarNode& other) const { return f_score > other.f_score; }
    };

    std::mutex mtx_open_set, mtx_g_score, mtx_came_from;

    void neighbor_search_task_CppLib(std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>>& open_set,
                            std::unordered_map<long long, double>& g_score,
                            std::unordered_map<long long, long long>& came_from,
                            const RoadNetwork& network, const std::vector<Edge>& neighbors,
                            size_t begin, size_t end, double current_g_score, long long current_id,
                            const Node& goal_node_data) {

        for (size_t i = begin; i < end; ++i) {
            const Edge& edge = neighbors[i];
                    
            long long neighbor_id = edge.target_node_id;
            double tentative_g_score = current_g_score + edge.weight;

            bool update = false;
            {
                std::lock_guard<std::mutex> lock(mtx_g_score);
                if (!g_score.count(neighbor_id) || tentative_g_score < g_score[neighbor_id]) {
                    g_score[neighbor_id] = tentative_g_score;
                    update = true;
                }
            }

            if (update) {
                {
                    std::lock_guard<std::mutex> lock(mtx_came_from);
                    came_from[neighbor_id] = current_id;
                }

                const Node *neighbor_node_ptr = network.get_node(neighbor_id);
                if (neighbor_node_ptr) {  // Check if neighbor node data exists
                    double h_score = heuristic(*neighbor_node_ptr, goal_node_data);
                    // Check for potential infinity issues if heuristic returns max()
                    if (h_score < std::numeric_limits<double>::max()) {
                        double f_score = tentative_g_score + h_score;

                        {
                            std::lock_guard<std::mutex> lock(mtx_open_set);
                            open_set.push({ neighbor_id, f_score });
                        }
                    }
                } else {
                    // Node exists in graph edges but not in NodeMap - indicates inconsistent data
                    // Option: Throw an error, log a warning, or assign max heuristic
                    // For now, let's skip adding it to open_set if data is missing
                    // std::cerr << "Warning: Node " << neighbor_id << " missing coordinate data."
                    // << std::endl;
                }
            }
        }
    }

    void neighbor_search_task_PqFine(DataStructure::PriorityQueue::SortedLinkedList_FineLockPQ<AStarNode, std::greater<AStarNode>>& open_set,
                            std::unordered_map<long long, double>& g_score,
                            std::unordered_map<long long, long long>& came_from,
                            const RoadNetwork& network, const std::vector<Edge>& neighbors,
                            size_t begin, size_t end, double current_g_score, long long current_id,
                            const Node& goal_node_data) {

        for (size_t i = begin; i < end; ++i) {
            const Edge& edge = neighbors[i];
                    
            long long neighbor_id = edge.target_node_id;
            double tentative_g_score = current_g_score + edge.weight;

            bool update = false;
            {
                std::lock_guard<std::mutex> lock(mtx_g_score);
                if (!g_score.count(neighbor_id) || tentative_g_score < g_score[neighbor_id]) {
                    g_score[neighbor_id] = tentative_g_score;
                    update = true;
                }
            }

            if (update) {
                {
                    std::lock_guard<std::mutex> lock(mtx_came_from);
                    came_from[neighbor_id] = current_id;
                }

                const Node *neighbor_node_ptr = network.get_node(neighbor_id);
                if (neighbor_node_ptr) {  // Check if neighbor node data exists
                    double h_score = heuristic(*neighbor_node_ptr, goal_node_data);
                    // Check for potential infinity issues if heuristic returns max()
                    if (h_score < std::numeric_limits<double>::max()) {
                        double f_score = tentative_g_score + h_score;

                        {
                            // std::lock_guard<std::mutex> lock(mtx_open_set);
                            open_set.push({ neighbor_id, f_score });
                        }
                    }
                } else {
                    // Node exists in graph edges but not in NodeMap - indicates inconsistent data
                    // Option: Throw an error, log a warning, or assign max heuristic
                    // For now, let's skip adding it to open_set if data is missing
                    // std::cerr << "Warning: Node " << neighbor_id << " missing coordinate data."
                    // << std::endl;
                }
            }
        }
    }

    std::vector<long long> search_TPool_CppLib(const RoadNetwork& network,
                                        long long start_node_id, long long goal_node_id, int NUM_THREADS) {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr) throw std::runtime_error("Start node ID not found in NodeMap.");
        if (!goal_node_ptr) throw std::runtime_error("Goal node ID not found in NodeMap.");

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

        ThreadPool pool(NUM_THREADS);

        while (!open_set.empty()) {
            AStarNode current;
            {
                std::lock_guard<std::mutex> lock(mtx_open_set);
                current = open_set.top();
                open_set.pop();
            }

            long long current_id = current.id;

            // Goal reached (same as before)
            if (current_id == goal_node_id) {
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
            double current_g_score = (g_score.count(current_id)) ? g_score[current_id] : std::numeric_limits<double>::max();

            // Check if the current node exists in the graph's adjacency list
            const std::vector<Edge> *neighbors_ptr = network.get_neighbors(current_id);
            if (!neighbors_ptr) continue;  // Node has no outgoing edges

            // Parallel explore
            const std::vector<Edge>& neighbors = *neighbors_ptr;

            size_t total = neighbors.size();
            size_t chunk_size = (total + NUM_THREADS - 1) / NUM_THREADS;
            std::vector<std::future<void>> results;

            for (int t = 0; t < NUM_THREADS; ++t) {
                size_t begin = t * chunk_size;
                size_t end = std::min(begin + chunk_size, total);
                if (begin >= end) continue;

                results.emplace_back(pool.enqueue(neighbor_search_task_CppLib,
                    std::ref(open_set), std::ref(g_score), std::ref(came_from),
                    std::ref(network), std::ref(neighbors),
                    begin, end, current_g_score, current_id, std::ref(goal_node_data)));
            }

            for (auto& f : results) f.get();
        }

        // Open set empty, goal not reached
        return {};
    }

    std::vector<long long> search_TVector_CppLib(const RoadNetwork &network, 
                                        long long start_node_id, long long goal_node_id, int NUM_THREADS)
    {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr) throw std::runtime_error("Start node ID not found in NodeMap.");
        if (!goal_node_ptr) throw std::runtime_error("Goal node ID not found in NodeMap.");

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

        while (!open_set.empty()) {
            AStarNode current = open_set.top();
            open_set.pop();
            long long current_id = current.id;

            // Goal reached (same as before)
            if (current_id == goal_node_id) {
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
            double current_g_score = (g_score.count(current_id)) ? g_score[current_id] : std::numeric_limits<double>::max();
 
            // Check if the current node exists in the graph's adjacency list
            const std::vector<Edge> *neighbors_ptr = network.get_neighbors(current_id);
            if (!neighbors_ptr) continue;  // Node has no outgoing edges

            // Parallel explore
            std::vector<std::thread> threads;
            const std::vector<Edge>& neighbors = *neighbors_ptr;

            long long total = neighbors.size();
            long long chunk_size = (total + NUM_THREADS - 1) / NUM_THREADS;
            for (int t = 0; t < NUM_THREADS; ++t) {
                long long start = t * chunk_size;
                long long end = std::min(start + chunk_size, total);

                threads.emplace_back(neighbor_search_task_CppLib,
                    std::ref(open_set), std::ref(g_score), std::ref(came_from),
                    std::ref(network), std::ref(neighbors),
                    start, end, current_g_score, current_id, std::ref(goal_node_data)
                );
            }

            for (auto& t : threads) t.join();
        }

        // Open set empty, goal not reached
        return {};
    }

    std::vector<long long> search_TPool_PqFine(const RoadNetwork& network,
                                            long long start_node_id, long long goal_node_id, int NUM_THREADS) {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr) throw std::runtime_error("Start node ID not found in NodeMap.");
        if (!goal_node_ptr) throw std::runtime_error("Goal node ID not found in NodeMap.");

        const Node &start_node_data = *start_node_ptr;
        const Node &goal_node_data = *goal_node_ptr;

        // Priority queue, g_score, came_from setup (as before)
        // std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
        DataStructure::PriorityQueue::SortedLinkedList_FineLockPQ<AStarNode, std::greater<AStarNode>> open_set;
        std::unordered_map<long long, double> g_score;
        std::unordered_map<long long, long long> came_from;

        // Initialize g_scores - only need to track nodes encountered
        // Set start node g_score
        g_score[start_node_id] = 0.0;

        // Add start node to the open set
        open_set.push({start_node_id, heuristic(start_node_data, goal_node_data)});

        ThreadPool pool(NUM_THREADS);

        while (!open_set.empty()) {
            AStarNode current;
            {
                // std::lock_guard<std::mutex> lock(mtx_open_set);
                // current = open_set.top();
                // open_set.pop();
                std::optional<AStarNode> current_opt = open_set.pop();
                current = current_opt.value();
            }

            long long current_id = current.id;

            // Goal reached (same as before)
            if (current_id == goal_node_id) {
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
            double current_g_score = (g_score.count(current_id)) ? g_score[current_id] : std::numeric_limits<double>::max();

            // Check if the current node exists in the graph's adjacency list
            const std::vector<Edge> *neighbors_ptr = network.get_neighbors(current_id);
            if (!neighbors_ptr) continue;  // Node has no outgoing edges

            // Parallel explore
            const std::vector<Edge>& neighbors = *neighbors_ptr;

            size_t total = neighbors.size();
            size_t chunk_size = (total + NUM_THREADS - 1) / NUM_THREADS;
            std::vector<std::future<void>> results;

            for (int t = 0; t < NUM_THREADS; ++t) {
                size_t begin = t * chunk_size;
                size_t end = std::min(begin + chunk_size, total);
                if (begin >= end) continue;

                results.emplace_back(pool.enqueue(neighbor_search_task_PqFine,
                    std::ref(open_set), std::ref(g_score), std::ref(came_from),
                    std::ref(network), std::ref(neighbors),
                    begin, end, current_g_score, current_id, std::ref(goal_node_data)));
            }

            for (auto& f : results) f.get();
        }

        // Open set empty, goal not reached
        return {};
    }

    std::vector<long long> search_TVector_PqFine(const RoadNetwork &network, 
                                        long long start_node_id, long long goal_node_id, int NUM_THREADS)
    {
        // Check if start and goal nodes exist in the nodes map
        const Node *start_node_ptr = network.get_node(start_node_id);
        const Node *goal_node_ptr = network.get_node(goal_node_id);

        if (!start_node_ptr) throw std::runtime_error("Start node ID not found in NodeMap.");
        if (!goal_node_ptr) throw std::runtime_error("Goal node ID not found in NodeMap.");

        const Node &start_node_data = *start_node_ptr;
        const Node &goal_node_data = *goal_node_ptr;

        // Priority queue, g_score, came_from setup (as before)
        // std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
        DataStructure::PriorityQueue::SortedLinkedList_FineLockPQ<AStarNode, std::greater<AStarNode>> open_set;
        std::unordered_map<long long, double> g_score;
        std::unordered_map<long long, long long> came_from;

        // Initialize g_scores - only need to track nodes encountered
        // Set start node g_score
        g_score[start_node_id] = 0.0;

        // Add start node to the open set
        open_set.push({start_node_id, heuristic(start_node_data, goal_node_data)});

        while (!open_set.empty()) {
            // AStarNode current = open_set.top();
            // open_set.pop();
            AStarNode current;
            std::optional<AStarNode> current_opt = open_set.pop();
            current = current_opt.value();
            long long current_id = current.id;

            // Goal reached (same as before)
            if (current_id == goal_node_id) {
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
            double current_g_score = (g_score.count(current_id)) ? g_score[current_id] : std::numeric_limits<double>::max();
 
            // Check if the current node exists in the graph's adjacency list
            const std::vector<Edge> *neighbors_ptr = network.get_neighbors(current_id);
            if (!neighbors_ptr) continue;  // Node has no outgoing edges

            // Parallel explore
            std::vector<std::thread> threads;
            const std::vector<Edge>& neighbors = *neighbors_ptr;

            long long total = neighbors.size();
            long long chunk_size = (total + NUM_THREADS - 1) / NUM_THREADS;
            for (int t = 0; t < NUM_THREADS; ++t) {
                long long start = t * chunk_size;
                long long end = std::min(start + chunk_size, total);

                threads.emplace_back(neighbor_search_task_PqFine,
                    std::ref(open_set), std::ref(g_score), std::ref(came_from),
                    std::ref(network), std::ref(neighbors),
                    start, end, current_g_score, current_id, std::ref(goal_node_data)
                );
            }

            for (auto& t : threads) t.join();
        }

        // Open set empty, goal not reached
        return {};
    }

}

namespace std {
    template <>
    class numeric_limits<AStarParallel::AStarNode> {
    public:
        static constexpr bool is_specialized = true;

        static constexpr AStarParallel::AStarNode min() noexcept {
            return { std::numeric_limits<long long>::min(), std::numeric_limits<double>::lowest() };
        }

        static constexpr AStarParallel::AStarNode max() noexcept {
            return { std::numeric_limits<long long>::max(), std::numeric_limits<double>::max() };
        }

        static constexpr AStarParallel::AStarNode lowest() noexcept {
            return { std::numeric_limits<long long>::lowest(), std::numeric_limits<double>::lowest() };
        }

        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;

        // Others can remain defaulted or false as appropriate
    };
}
