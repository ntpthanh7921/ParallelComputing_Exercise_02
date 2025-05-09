#include <algorithm>  // For std::shuffle, std::max
#include <benchmark/benchmark.h>
#include <memory>         // For std::unique_ptr
#include <mutex>          // For std::mutex, std::lock_guard (in fixture)
#include <random>         // For random numbers
#include <set>            // Include std::set
#include <thread>         // For std::thread::hardware_concurrency
#include <unordered_set>  // Include std::unordered_set
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
// CONTAINS_RATIO is implicitly 1.0 - ADD_RATIO - REMOVE_RATIO
const double EXECUTION_WARMUP_SECONDS = 0.25;

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
    std::mt19937 gen(std::random_device{}());  // Seed PRNG
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
// Generate workloads once to ensure all benchmarks use the same operations
const std::vector<Operation> WARMUP_WORKLOAD =
    generate_operations(WARMUP_OPERATIONS, VALUE_RANGE, ADD_RATIO, REMOVE_RATIO);
const std::vector<Operation> FIXED_WORKLOAD =
    generate_operations(NUM_OPERATIONS, VALUE_RANGE, ADD_RATIO, REMOVE_RATIO);

// --- Adapter for std::set ---
// Wraps std::set to conform to the ISet interface for benchmarking.
// Does NOT add thread safety - intended for single-thread benchmarks.
template <typename T>
class StdSetAdapter : public DataStructure::Set::ISet<T>
{
private:
    std::set<T> std_set;

public:
    StdSetAdapter() = default;
    ~StdSetAdapter() override = default;
    StdSetAdapter(const StdSetAdapter &) = delete;
    StdSetAdapter &operator=(const StdSetAdapter &) = delete;

    bool add(const T &val) override { return std_set.insert(val).second; }

    bool remove(const T &val) override { return std_set.erase(val) > 0; }

    bool contains(const T &val) const override { return std_set.count(val) > 0; }

    size_t size() const override { return std_set.size(); }

    bool check_invariants() const override { return true; }  // Trivial
};

// --- Adapter for std::unordered_set ---
// Wraps std::unordered_set to conform to the ISet interface for benchmarking.
// Does NOT add thread safety - intended for single-thread benchmarks.
template <typename T>
class StdUnorderedSetAdapter : public DataStructure::Set::ISet<T>
{
private:
    std::unordered_set<T> std_uset;

public:
    StdUnorderedSetAdapter() = default;
    ~StdUnorderedSetAdapter() override = default;
    StdUnorderedSetAdapter(const StdUnorderedSetAdapter &) = delete;
    StdUnorderedSetAdapter &operator=(const StdUnorderedSetAdapter &) = delete;

    bool add(const T &val) override { return std_uset.insert(val).second; }

    bool remove(const T &val) override { return std_uset.erase(val) > 0; }

    bool contains(const T &val) const override { return std_uset.count(val) > 0; }

    size_t size() const override { return std_uset.size(); }

    bool check_invariants() const override { return true; }  // Trivial
};

// --- Base Benchmark Fixture Definition (with Mutex for SetUp/TearDown safety) ---
template <typename SetType>
class SetBenchmarkFixture : public ::benchmark::Fixture
{
private:
    std::mutex fixture_mutex;  // Protects fixture setup/teardown if framework calls concurrently
public:
    std::unique_ptr<DataStructure::Set::ISet<TestSetElement>> set_instance;

    void SetUp(const ::benchmark::State & /* state */) override
    {
        std::lock_guard<std::mutex> lock(fixture_mutex);
        // Check if already initialized (mitigation for potential framework behavior)
        if (!set_instance)
        {
            set_instance = std::make_unique<SetType>();
            // Perform state warmup using the generated workload
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
    }

    void TearDown(const ::benchmark::State & /* state */) override
    {
        std::lock_guard<std::mutex> lock(fixture_mutex);
        set_instance.reset();
    }
};

// --- Concrete Set Types ---
using SequentialSet = DataStructure::Set::SortedLinkedList_Sequential<TestSetElement>;
using CoarseLockSet = DataStructure::Set::SortedLinkedList_CoarseLock<TestSetElement>;
using FineLockSet = DataStructure::Set::SortedLinkedList_FineLock<TestSetElement>;
using StdSet = StdSetAdapter<TestSetElement>;                    // Use adapter
using StdUnorderedSet = StdUnorderedSetAdapter<TestSetElement>;  // Use adapter

// --- Benchmark Definitions using BENCHMARK_TEMPLATE_DEFINE_F ---
// Define the benchmark body ONCE for each specific type, giving unique method names.

// Generic benchmark body function (avoids repetition)
void BenchmarkBody(benchmark::State &state, DataStructure::Set::ISet<TestSetElement> *set)
{
    if (!set)
    {
        state.SkipWithError("Fixture setup failed - set_instance is null");
        return;
    }
    int num_threads = state.threads();
    size_t total_ops = FIXED_WORKLOAD.size();
    // Integer division is fine here, thread 0 handles remainder if any
    size_t ops_per_thread = total_ops / num_threads;
    size_t start_index = state.thread_index() * ops_per_thread;
    // Ensure the last thread processes any remaining operations
    size_t end_index =
        (state.thread_index() == num_threads - 1) ? total_ops : (start_index + ops_per_thread);

    // Ensure indices are valid (safeguard against empty workload or thread issues)
    if (start_index >= total_ops)
    {
        start_index = total_ops;  // Prevent out-of-bounds if workload is too small
    }
    if (end_index > total_ops)
    {
        end_index = total_ops;
    }

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
                    benchmark::DoNotOptimize(set->contains(op.value));  // Prevent optimization
                    break;
            }
        }
    }
    // Only set items processed if this thread actually did work
    if (end_index > start_index)
    {
        state.SetItemsProcessed(end_index - start_index);
    }
    else
    {
        state.SetItemsProcessed(0);
    }
    state.SetComplexityN(total_ops);  // Optional: Report N for complexity analysis
}

// Define benchmarks using the common body function
BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_SequentialOps, SequentialSet)
(benchmark::State &state) { BenchmarkBody(state, this->set_instance.get()); }

BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_CoarseLockOps, CoarseLockSet)
(benchmark::State &state) { BenchmarkBody(state, this->set_instance.get()); }

BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_FineLockOps, FineLockSet)
(benchmark::State &state) { BenchmarkBody(state, this->set_instance.get()); }

BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_StdSetOps, StdSet)
(benchmark::State &state) { BenchmarkBody(state, this->set_instance.get()); }

BENCHMARK_TEMPLATE_DEFINE_F(SetBenchmarkFixture, BM_StdUnorderedSetOps, StdUnorderedSet)
(benchmark::State &state) { BenchmarkBody(state, this->set_instance.get()); }

// --- Register Benchmarks using BENCHMARK_REGISTER_F ---
// Register each uniquely named benchmark definition.

const int num_hardware_threads = std::max(1u, std::thread::hardware_concurrency());

// Register Sequential (only makes sense for 1 thread)
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_SequentialOps)
    ->DenseThreadRange(1, 1)  // Run only for 1 thread
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Register Coarse Lock (run for multiple threads)
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_CoarseLockOps)
    ->ThreadRange(1, num_hardware_threads)
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Register Fine Lock (run for multiple threads)
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_FineLockOps)
    ->ThreadRange(1, num_hardware_threads)
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Register std::set Adapter (RUN ONLY FOR 1 THREAD)
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_StdSetOps)
    ->DenseThreadRange(1, 1)  // IMPORTANT: Enforce single thread
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Register std::unordered_set Adapter (RUN ONLY FOR 1 THREAD)
BENCHMARK_REGISTER_F(SetBenchmarkFixture, BM_StdUnorderedSetOps)
    ->DenseThreadRange(1, 1)  // IMPORTANT: Enforce single thread
    ->MinWarmUpTime(EXECUTION_WARMUP_SECONDS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// --- Main Function ---
BENCHMARK_MAIN();
