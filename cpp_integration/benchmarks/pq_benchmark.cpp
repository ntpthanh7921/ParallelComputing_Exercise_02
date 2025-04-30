#include <algorithm>  // For std::max
#include <benchmark/benchmark.h>
#include <limits>
#include <queue>    // For std::priority_queue
#include <random>   // For random numbers
#include <thread>   // For std::thread::hardware_concurrency
#include <utility>  // For std::pair
#include <vector>

// Include your PQ implementations and interface (though interface isn't strictly needed here)
#include "data_structure/pq_fine.h"  // Contains SortedLinkedList_FineLockPQ

// --- Configuration & Test Element Setup ---
using TestPQElement = std::pair<int, int>;  // {priority, sequence_id}

// Comparator for custom PQ and std::priority_queue (Max Heap based on priority)
// Note: std::priority_queue uses std::less by default for Max Heap.
// Our custom comparator logic needs to mimic that if we want comparable behavior.
// If std::less is used, it compares pairs lexicographically by default.
// We need a comparator that compares ONLY priority for std::priority_queue too.
struct ComparePriorityOnly
{
    // For std::priority_queue (Max Heap), Comparison should return true if lhs is "less" than rhs
    // meaning lhs has lower priority.
    bool operator()(const TestPQElement &lhs, const TestPQElement &rhs) const
    {
        return lhs.first < rhs.first;  // Lower numeric value = lower priority
    }
};

// Required numeric_limits specialization for sentinel nodes in custom PQ
namespace std
{
template <>
struct numeric_limits<TestPQElement>
{
    static constexpr bool is_specialized = true;

    static constexpr TestPQElement lowest() noexcept { return {numeric_limits<int>::lowest(), 0}; }

    static constexpr TestPQElement max() noexcept { return {numeric_limits<int>::max(), 0}; }

    static constexpr int digits = numeric_limits<int>::digits;
    // Define others if necessary, matching 'int'
};
}  // namespace std

// --- Benchmark Configuration ---
const size_t PQ_NUM_OPERATIONS = 100000;
const size_t PQ_WARMUP_OPERATIONS = PQ_NUM_OPERATIONS / 10;
const int PQ_VALUE_RANGE = 10000;   // Priority range
const double PQ_PUSH_RATIO = 0.50;  // 50% push, 50% pop
const double PQ_EXECUTION_WARMUP_SECONDS = 0.25;

// --- Define Operation Structure ---
struct PQOperation
{
    enum OpType
    {
        PUSH,
        POP
    } type;

    TestPQElement value;  // Only relevant for PUSH
};

// --- Generate Operations Helper ---
std::vector<PQOperation> generate_pq_operations(size_t count, int value_range, double push_ratio)
{
    std::vector<PQOperation> ops;
    ops.reserve(count);
    std::mt19937 gen(std::random_device{}());  // Seed PRNG
    std::uniform_int_distribution<> prio_dist(0, value_range - 1);
    std::uniform_real_distribution<> op_dist(0.0, 1.0);
    int sequence_id_counter = 0;  // Unique ID for values

    for (size_t i = 0; i < count; ++i)
    {
        double op_choice = op_dist(gen);
        if (op_choice < push_ratio)
        {
            int priority = prio_dist(gen);
            ops.push_back({PQOperation::OpType::PUSH, {priority, sequence_id_counter++}});
        }
        else
        {
            // Value for POP is irrelevant, can use default TestPQElement
            ops.push_back({PQOperation::OpType::POP, {}});
        }
    }
    return ops;
}

// --- Globally Generated Workloads ---
const std::vector<PQOperation> PQ_WARMUP_WORKLOAD =
    generate_pq_operations(PQ_WARMUP_OPERATIONS, PQ_VALUE_RANGE, PQ_PUSH_RATIO);
const std::vector<PQOperation> PQ_FIXED_WORKLOAD =
    generate_pq_operations(PQ_NUM_OPERATIONS, PQ_VALUE_RANGE, PQ_PUSH_RATIO);

// --- Benchmark for Custom Fine-Grained Priority Queue ---
static void BM_CustomFineLockPQ(benchmark::State &state)
{
    // Type alias for clarity, includes the necessary comparator
    using CustomPQ = DataStructure::PriorityQueue::SortedLinkedList_FineLockPQ<TestPQElement,
                                                                               ComparePriorityOnly>;
    CustomPQ pq;  // Create instance

    // Warmup phase (single threaded)
    for (const auto &op : PQ_WARMUP_WORKLOAD)
    {
        if (op.type == PQOperation::OpType::PUSH)
        {
            pq.push(op.value);
        }
        else
        {
            pq.pop();  // Ignore result during warmup
        }
    }

    // Benchmark loop
    int num_threads = state.threads();
    size_t total_ops = PQ_FIXED_WORKLOAD.size();
    size_t ops_per_thread = total_ops / num_threads;
    size_t start_index = state.thread_index() * ops_per_thread;
    size_t end_index =
        (state.thread_index() == num_threads - 1) ? total_ops : (start_index + ops_per_thread);
    if (start_index >= total_ops)
        start_index = total_ops;
    if (end_index > total_ops)
        end_index = total_ops;

    for (auto _ : state)
    {
        for (size_t i = start_index; i < end_index; ++i)
        {
            const auto &op = PQ_FIXED_WORKLOAD[i];
            if (op.type == PQOperation::OpType::PUSH)
            {
                pq.push(op.value);
            }
            else
            {
                // Pop and prevent optimization of the optional check/value access
                benchmark::DoNotOptimize(pq.pop());
            }
        }
    }
    if (end_index > start_index)
        state.SetItemsProcessed(end_index - start_index);
    else
        state.SetItemsProcessed(0);
    state.SetComplexityN(total_ops);
}

// --- Benchmark for std::priority_queue ---
static void BM_StdPriorityQueue(benchmark::State &state)
{
    // Use the custom comparator to match behavior (max heap based on priority only)
    using StdPQ =
        std::priority_queue<TestPQElement, std::vector<TestPQElement>, ComparePriorityOnly>;
    StdPQ pq;  // Create instance

    // Warmup phase (single threaded)
    for (const auto &op : PQ_WARMUP_WORKLOAD)
    {
        if (op.type == PQOperation::OpType::PUSH)
        {
            pq.push(op.value);
        }
        else
        {
            if (!pq.empty())
            {
                pq.pop();
            }
        }
    }

    // Benchmark loop (this benchmark is registered single-threaded only)
    size_t total_ops = PQ_FIXED_WORKLOAD.size();
    for (auto _ : state)
    {
        for (size_t i = 0; i < total_ops; ++i)
        {  // Single thread processes all ops
            const auto &op = PQ_FIXED_WORKLOAD[i];
            if (op.type == PQOperation::OpType::PUSH)
            {
                pq.push(op.value);
            }
            else
            {
                if (!pq.empty())
                {
                    // Need to access top() before pop() if value needed, but not required for
                    // timing pop itself benchmark::DoNotOptimize(pq.top()); // Uncomment if top()
                    // access is part of workload
                    pq.pop();
                }
            }
        }
    }
    state.SetItemsProcessed(total_ops);
    state.SetComplexityN(total_ops);
}

// --- Register Benchmarks ---
const int num_hardware_threads = std::max(1u, std::thread::hardware_concurrency());

// Register Custom Fine-Grained PQ (Multi-threaded)
BENCHMARK(BM_CustomFineLockPQ)
    ->ThreadRange(1, num_hardware_threads)
    ->MinWarmUpTime(PQ_EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Complexity();

// Register std::priority_queue (Single-threaded ONLY)
BENCHMARK(BM_StdPriorityQueue)
    ->DenseThreadRange(1, 1)  // IMPORTANT: Enforce single thread
    ->MinWarmUpTime(PQ_EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Complexity();

// --- Main Function ---
BENCHMARK_MAIN();
