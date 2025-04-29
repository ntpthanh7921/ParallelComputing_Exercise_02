#include <algorithm>  // For std::shuffle, std::max
#include <benchmark/benchmark.h>
#include <memory>  // For std::unique_ptr
#include <mutex>   // For std::mutex, std::lock_guard <<-- NEEDED
#include <random>  // For random numbers
#include <thread>  // For std::thread::hardware_concurrency
#include <vector>

// Include your set implementations and interface
#include "data_structure/iset.h"
#include "data_structure/set_coarse.h"
#include "data_structure/set_fine.h"
#include "data_structure/set_sequential.h"

// --- Configuration ---
using TestSetElement = int;
const size_t NUM_OPERATIONS = 100000;
const size_t WARMUP_OPERATIONS = NUM_OPERATIONS / 10;
const int VALUE_RANGE = 10000;
const double ADD_RATIO = 0.40;
const double REMOVE_RATIO = 0.40;
const double EXECUTION_WARMUP_SECONDS = 0.25;  // Use double for seconds

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

// --- Generate Operations Helper ---
std::vector<Operation> generate_operations(size_t count, int value_range, double add_ratio,
                                           double remove_ratio)
{
    std::vector<Operation> ops;
    ops.reserve(count);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> val_dist(0, value_range - 1);
    std::uniform_real_distribution<> op_dist(0.0, 1.0);
    for (size_t i = 0; i < count; ++i)
    {
        double op_choice = op_dist(gen);
        TestSetElement value = val_dist(gen);
        if (op_choice < add_ratio)
            ops.push_back({Operation::OpType::ADD, value});
        else if (op_choice < add_ratio + remove_ratio)
            ops.push_back({Operation::OpType::REMOVE, value});
        else
            ops.push_back({Operation::OpType::CONTAINS, value});
    }
    return ops;
}

// --- Globally Generated Workloads ---
const std::vector<Operation> WARMUP_WORKLOAD =
    generate_operations(WARMUP_OPERATIONS, VALUE_RANGE, ADD_RATIO, REMOVE_RATIO);
const std::vector<Operation> FIXED_WORKLOAD =
    generate_operations(NUM_OPERATIONS, VALUE_RANGE, ADD_RATIO, REMOVE_RATIO);

// --- Base Benchmark Fixture Definition (with Mutex) ---
template <typename SetType>
class SetBenchmarkFixture : public ::benchmark::Fixture
{
private:
    // Mutex to protect concurrent access during SetUp/TearDown if framework causes it
    std::mutex fixture_mutex;

public:
    std::unique_ptr<DataStructure::Set::ISet<TestSetElement>> set_instance;

    void SetUp(const ::benchmark::State & /* state */) override
    {
        // Lock the mutex before modifying set_instance
        std::lock_guard<std::mutex> lock(fixture_mutex);

        // Check if already initialized within this SetUp call context
        // (Mitigation for potential concurrent calls from framework)
        if (!set_instance)
        {
            set_instance = std::make_unique<SetType>();
            // Perform state warmup
            for (const auto &op : WARMUP_WORKLOAD)
            {
                switch (op.type)
                {
                    case Operation::OpType::ADD:
                        set_instance->add(op.value);
                        break;
                    case Operation::OpType::REMOVE:
                        set_instance->remove(op.value);
                        break;
                    case Operation::OpType::CONTAINS:
                        set_instance->contains(op.value);
                        break;
                }
            }
        }
        // Mutex released automatically
    }

    void TearDown(const ::benchmark::State & /* state */) override
    {
        // Lock the mutex before resetting (optional, but safer)
        std::lock_guard<std::mutex> lock(fixture_mutex);
        set_instance.reset();
    }
};

// --- Concrete Set Types ---
using SequentialSet = DataStructure::Set::SortedLinkedList_Sequential<TestSetElement>;
using CoarseLockSet = DataStructure::Set::SortedLinkedList_CoarseLock<TestSetElement>;
using FineLockSet = DataStructure::Set::SortedLinkedList_FineLock<TestSetElement>;

// --- Benchmark Definitions using BENCHMARK_TEMPLATE_DEFINE_F ---
// Define the benchmark body ONCE for each specific type, giving unique method names.

// Define for Sequential
BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_SequentialOps, SequentialSet)
(benchmark::State &state)
{
    DataStructure::Set::ISet<TestSetElement> *set = this->set_instance.get();
    // Defensive null check
    if (!set)
    {
        state.SkipWithError("Fixture setup failed - set_instance is null in BM_SequentialOps");
        return;
    }
    int num_threads = state.threads();
    size_t total_ops = FIXED_WORKLOAD.size();
    size_t ops_per_thread = total_ops / num_threads;
    size_t start_index = state.thread_index() * ops_per_thread;
    size_t end_index =
        (state.thread_index() == num_threads - 1) ? total_ops : (start_index + ops_per_thread);

    for (auto _ : state)
    {
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
                    benchmark::DoNotOptimize(set->contains(op.value));
                    break;
            }
        }
    }
    state.SetItemsProcessed(total_ops);
}

// Define for Coarse Lock
BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_CoarseLockOps, CoarseLockSet)
(benchmark::State &state)
{
    DataStructure::Set::ISet<TestSetElement> *set = this->set_instance.get();
    if (!set)
    {
        state.SkipWithError("Fixture setup failed - set_instance is null in BM_CoarseLockOps");
        return;
    }
    int num_threads = state.threads();
    size_t total_ops = FIXED_WORKLOAD.size();
    size_t ops_per_thread = total_ops / num_threads;
    size_t start_index = state.thread_index() * ops_per_thread;
    size_t end_index =
        (state.thread_index() == num_threads - 1) ? total_ops : (start_index + ops_per_thread);

    for (auto _ : state)
    {
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
                    benchmark::DoNotOptimize(set->contains(op.value));
                    break;
            }
        }
    }
    state.SetItemsProcessed(total_ops);
}

// Define for Fine Lock
BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_FineLockOps, FineLockSet)
(benchmark::State &state)
{
    DataStructure::Set::ISet<TestSetElement> *set = this->set_instance.get();
    if (!set)
    {
        state.SkipWithError("Fixture setup failed - set_instance is null in BM_FineLockOps");
        return;
    }
    int num_threads = state.threads();
    size_t total_ops = FIXED_WORKLOAD.size();
    size_t ops_per_thread = total_ops / num_threads;
    size_t start_index = state.thread_index() * ops_per_thread;
    size_t end_index =
        (state.thread_index() == num_threads - 1) ? total_ops : (start_index + ops_per_thread);

    for (auto _ : state)
    {
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
                    benchmark::DoNotOptimize(set->contains(op.value));
                    break;
            }
        }
    }
    state.SetItemsProcessed(total_ops);
}

// --- Register Benchmarks using BENCHMARK_REGISTER_F ---
// Register each uniquely named benchmark definition.
// Uses the BASE fixture name and the UNIQUE method name.

const int num_hardware_threads = std::max(1u, std::thread::hardware_concurrency());

// Register Sequential
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_SequentialOps)
    ->DenseThreadRange(1, 1)
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Register Coarse Lock
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_CoarseLockOps)
    ->ThreadRange(1, num_hardware_threads)
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Register Fine Lock
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_FineLockOps)
    ->ThreadRange(1, num_hardware_threads)
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// --- Main Function ---
BENCHMARK_MAIN();
