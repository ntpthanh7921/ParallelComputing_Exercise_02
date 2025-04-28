#include <benchmark/benchmark.h>
#include <memory>  // For std::unique_ptr
#include <random>  // For random numbers
#include <thread>  // For std::thread::hardware_concurrency
#include <vector>

// Include your set implementations and interface
#include "data_structure/set_coarse.h"
#include "data_structure/set_fine.h"
#include "data_structure/set_sequential.h"

using TestSetElement = int;  // Or the type you use

// --- Configuration ---
const int NUM_OPERATIONS = 100000;  // Total number of operations in the fixed workload
const int VALUE_RANGE = 10000;      // Range of values for elements
const double ADD_RATIO = 0.45;
const double REMOVE_RATIO = 0.45;

// const double CONTAINS_RATIO = 0.10; // Implicit

// --- Define Operation Structure ---
struct Operation
{
    enum OpType
    {
        ADD,
        REMOVE,
        CONTAINS
    } type;

    TestSetElement value;
};

// --- Generate Fixed Workload ---
// Generate this ONCE and reuse it for all benchmarks to ensure fairness
std::vector<Operation> generate_fixed_workload()
{
    std::vector<Operation> workload;
    workload.reserve(NUM_OPERATIONS);
    std::random_device rd;
    std::mt19937 gen(rd());  // Seed generator
    std::uniform_int_distribution<> val_dist(0, VALUE_RANGE - 1);
    std::uniform_real_distribution<> op_dist(0.0, 1.0);

    for (int i = 0; i < NUM_OPERATIONS; ++i)
    {
        double op_choice = op_dist(gen);
        TestSetElement value = val_dist(gen);
        if (op_choice < ADD_RATIO)
        {
            workload.push_back({Operation::OpType::ADD, value});
        }
        else if (op_choice < ADD_RATIO + REMOVE_RATIO)
        {
            workload.push_back({Operation::OpType::REMOVE, value});
        }
        else
        {
            workload.push_back({Operation::OpType::CONTAINS, value});
        }
    }
    // Optional: Shuffle if the order matters (likely doesn't for this setup)
    // std::shuffle(workload.begin(), workload.end(), gen);
    return workload;
}

// Store the globally generated workload
const std::vector<Operation> FIXED_WORKLOAD = generate_fixed_workload();

// --- Templated Benchmark Function ---
template <typename SetType>
static void BM_SetMixedOps(benchmark::State &state)
{
    // Create a new set instance for each benchmark run/thread if needed,
    // or share one if that's the scenario you want to test.
    // For isolated throughput, create per run.
    auto set = std::make_unique<SetType>();

    // Number of threads for this run (passed by ->Threads())
    int num_threads = state.threads();
    size_t ops_per_thread = FIXED_WORKLOAD.size() / num_threads;
    size_t start_index = state.thread_index() * ops_per_thread;
    size_t end_index = (state.thread_index() == num_threads - 1)
                           ? FIXED_WORKLOAD.size()  // Last thread takes remainder
                           : start_index + ops_per_thread;

    // The benchmark loop
    for (auto _ : state)
    {
        // This inner loop executes the assigned portion of the workload once per benchmark
        // iteration. Google Benchmark runs this outer loop multiple times to get stable timings.
        for (size_t i = start_index; i < end_index; ++i)
        {
            const auto &op = FIXED_WORKLOAD[i];
            switch (op.type)
            {
                case Operation::OpType::ADD:
                    set->add(op.value);
                    break;
                case Operation::OpType::REMOVE:
                    set->remove(op.value);
                    break;
                case Operation::OpType::CONTAINS:
                    // Use DoNotOptimize to prevent the compiler optimizing away the contains call
                    benchmark::DoNotOptimize(set->contains(op.value));
                    break;
            }
        }
    }

    // Optional: Set items processed if meaningful (here it's tricky due to fixed workload)
    // state.SetItemsProcessed(state.iterations() * (end_index - start_index));

    // Optional: Report total operations if needed (though benchmark name implies it)
    state.SetItemsProcessed(state.iterations() * FIXED_WORKLOAD.size()
                            / num_threads);  // Approximation per thread

    // Optional: Add custom counters if needed (e.g., final set size)
    // state.counters["FinalSize"] = set->size();
}

// --- Register Benchmarks ---

// Determine the number of threads to use
int num_hardware_threads = std::thread::hardware_concurrency();

// Register for Sequential implementation (run single-threaded)
BENCHMARK_TEMPLATE(BM_SetMixedOps, DataStructure::Set::SortedLinkedList_Sequential<TestSetElement>)
    ->Unit(benchmark::kMillisecond)  // Report time in milliseconds
    ->UseRealTime();                 // Measure wall time

// Register for CoarseLock implementation (run multi-threaded)
BENCHMARK_TEMPLATE(BM_SetMixedOps, DataStructure::Set::SortedLinkedList_CoarseLock<TestSetElement>)
    ->Unit(benchmark::kMillisecond)
    ->Threads(num_hardware_threads)  // Run with all available threads
    ->UseRealTime();                 // Measure wall time

// Register for FineLock implementation (run multi-threaded)
BENCHMARK_TEMPLATE(BM_SetMixedOps, DataStructure::Set::SortedLinkedList_FineLock<TestSetElement>)
    ->Unit(benchmark::kMillisecond)
    ->Threads(num_hardware_threads)  // Run with all available threads
    ->UseRealTime();                 // Measure wall time

// --- Main Function ---
BENCHMARK_MAIN();
