#include <algorithm>   // For std::shuffle, std::max
#include <atomic>      // For std::atomic
#include <chrono>      // For std::chrono
#include <cstddef>     // For size_t
#include <functional>  // For std::hash
#include <gtest/gtest.h>
#include <iostream>  // For std::cout in stress test
#include <memory>    // For std::unique_ptr
#include <numeric>   // For std::iota
#include <random>    // For std::random_device, std::mt19937, std::uniform_int_distribution
#include <thread>    // For std::thread, std::this_thread, std::hardware_concurrency
#include <vector>    // For std::vector

// Include necessary headers for set implementations and the interface
#include "data_structure/iset.h"
#include "data_structure/set_coarse.h"
#include "data_structure/set_fine.h"

// Define the type for the elements in the set for testing
using TestSetElement = int;

// --- Typed Test Fixture for Concurrent Tests ---
template <typename SetType>  // SetType will be a concurrent set implementation
class ConcurrentSetCorrectnessTest : public ::testing::Test
{
protected:
    // Use a pointer to the interface type
    std::unique_ptr<DataStructure::Set::ISet<TestSetElement>> set;

    // Constants for tests
    // Note: NUM_THREADS and OPERATIONS_PER_THREAD are now mainly used by ConcurrentAdds
    static constexpr int NUM_THREADS = 4;
    static constexpr int OPERATIONS_PER_THREAD = 500;  // Used for ConcurrentAdds test workload size

    // static constexpr int VALUE_RANGE = 1000; // Was used for mixed ops tests

    // Create an instance of the specific SetType for each test
    void SetUp() override { set = std::make_unique<SetType>(); }

    // TearDown is handled automatically by std::unique_ptr
};

// Define the list of concurrent set implementation types to be tested
using ConcurrentSetImplementations =
    ::testing::Types<DataStructure::Set::SortedLinkedList_CoarseLock<TestSetElement>,
                     DataStructure::Set::SortedLinkedList_FineLock<TestSetElement>
                     // Add future concurrent implementations here
                     >;

// Register the typed test suite
TYPED_TEST_SUITE(ConcurrentSetCorrectnessTest, ConcurrentSetImplementations);

// --- Typed Concurrent Tests ---

// Tests concurrent additions of unique values.
TYPED_TEST(ConcurrentSetCorrectnessTest, ConcurrentAdds)
{
    std::vector<std::thread> threads;
    std::atomic<int> successful_adds{0};
    int total_adds_attempted = this->NUM_THREADS * this->OPERATIONS_PER_THREAD;

    std::vector<TestSetElement> values_to_add(total_adds_attempted);
    std::iota(values_to_add.begin(), values_to_add.end(), 0);  // Fill with 0, 1, ..., N-1

    // Shuffle values to increase contention potential
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(values_to_add.begin(), values_to_add.end(), g);

    for (int i = 0; i < this->NUM_THREADS; ++i)
    {
        threads.emplace_back(
            [this, &successful_adds, &values_to_add, i]()
            {
                int start_index = i * this->OPERATIONS_PER_THREAD;
                int end_index = start_index + this->OPERATIONS_PER_THREAD;
                for (int k = start_index; k < end_index; ++k)
                {
                    // Ensure index is valid (though it should be)
                    if (k >= 0 && k < static_cast<int>(values_to_add.size()))
                    {
                        if (this->set->add(values_to_add[k]))
                        {
                            successful_adds++;
                        }
                    }
                }
            });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    // Verification
    ASSERT_EQ(successful_adds.load(), total_adds_attempted)
        << "Not all unique adds were successful.";
    ASSERT_EQ(this->set->size(), static_cast<size_t>(total_adds_attempted))
        << "Final size mismatch after concurrent adds.";
    // Verify all added elements are present
    for (int val = 0; val < total_adds_attempted; ++val)
    {
        EXPECT_TRUE(this->set->contains(val))
            << "Value " << val << " missing after concurrent adds.";
    }
    ASSERT_TRUE(this->set->check_invariants()) << "Invariant check failed after concurrent adds.";
}

// Tests concurrent mixed operations, checking only internal invariants after completion.
TYPED_TEST(ConcurrentSetCorrectnessTest, ConcurrentMixedOpsInvariantCheck)
{
    std::vector<std::thread> threads;
    const int VALUE_RANGE_INVARIANT = 1000;  // Value range for this test
    const int OPS_PER_THREAD_INVARIANT = 500;

    // Nested struct for operation type
    struct Operation
    {
        enum Type
        {
            ADD,
            REMOVE,
            CONTAINS
        } type;

        TestSetElement value;
    };

    // Helper to generate workload (can be member or static member if preferred)
    auto generate_workload = [&](int num_ops, int value_range)
    {
        std::vector<Operation> workload;
        workload.reserve(num_ops);
        std::random_device rd;
        std::mt19937 gen(rd()
                         ^ static_cast<unsigned int>(std::hash<std::thread::id>{}(
                             std::this_thread::get_id())));  // Per-thread seed
        std::uniform_int_distribution<> op_dist(0, 2);       // 0: ADD, 1: REMOVE, 2: CONTAINS
        std::uniform_int_distribution<> val_dist(0, value_range - 1);
        for (int i = 0; i < num_ops; ++i)
        {
            workload.push_back(
                {static_cast<typename Operation::Type>(op_dist(gen)), val_dist(gen)});
        }
        return workload;
    };

    for (int i = 0; i < this->NUM_THREADS; ++i)
    {
        threads.emplace_back(
            [this, generate_workload]()
            {
                std::vector<Operation> workload =
                    generate_workload(OPS_PER_THREAD_INVARIANT, VALUE_RANGE_INVARIANT);
                for (const auto &op : workload)
                {
                    switch (op.type)
                    {
                        case Operation::ADD:
                            this->set->add(op.value);
                            break;
                        case Operation::REMOVE:
                            this->set->remove(op.value);
                            break;
                        case Operation::CONTAINS:
                            this->set->contains(op.value);
                            break;
                    }
                }
            });
    }

    for (auto &t : threads)
    {
        t.join();
    }

    // Perform invariant check after concurrent execution
    ASSERT_TRUE(this->set->check_invariants()) << "Invariant check failed post-concurrency.";
    SUCCEED() << "ConcurrentMixedOps completed and passed invariant check.";
}

// Stress test running for a fixed duration. Checks for crashes and invariant violations.
TYPED_TEST(ConcurrentSetCorrectnessTest, StressTestDuration10Sec)
{
    // --- Configuration ---
    const int DURATION_SECONDS = 10;                                     // How long to run the test
    const int NUM_STRESS_THREADS = std::thread::hardware_concurrency();  // Use all thread
    const int VALUE_RANGE_STRESS = 500;  // Smaller range increases contention
    const int ADD_PERCENT = 45;          // Percentage chance of Add
    const int REMOVE_PERCENT = 45;       // Percentage chance of Remove
    // const int CONTAINS_PERCENT = 10; // Remainder is Contains

    // --- Setup ---
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};  // Signal for threads to stop

    // --- Thread Workload ---
    auto thread_func = [&]()
    {
        // Seed each thread's random number generator differently
        std::random_device rd;
        // Use a thread-local generator
        std::mt19937 gen(
            rd()
            ^ static_cast<unsigned int>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
        std::uniform_int_distribution<> op_dist(1, 100);
        std::uniform_int_distribution<> val_dist(0, VALUE_RANGE_STRESS - 1);

        // Loop until the stop flag is set
        // Use acquire semantics for visibility of the stop flag write
        while (!stop_flag.load(std::memory_order_acquire))
        {
            TestSetElement value = val_dist(gen);
            int op_choice = op_dist(gen);

            if (op_choice <= ADD_PERCENT)
            {
                this->set->add(value);  // Ignore return value
            }
            else if (op_choice <= ADD_PERCENT + REMOVE_PERCENT)
            {
                this->set->remove(value);  // Ignore return value
            }
            else
            {
                this->set->contains(value);  // Ignore return value
            }
        }
    };

    // --- Start Threads ---
    std::cout << "Starting stress test with " << NUM_STRESS_THREADS << " threads for "
              << DURATION_SECONDS << " seconds..." << std::endl;
    for (int i = 0; i < NUM_STRESS_THREADS; ++i)
    {
        threads.emplace_back(thread_func);
    }

    // --- Let threads run ---
    std::this_thread::sleep_for(std::chrono::seconds(DURATION_SECONDS));
    // Use release semantics to ensure the write is visible to threads using acquire load
    stop_flag.store(true, std::memory_order_release);

    // --- Join Threads ---
    for (auto &t : threads)
    {
        t.join();
    }
    std::cout << "Stress test threads joined." << std::endl;

    // --- Verification ---
    // Primary check: Did it survive and are invariants okay?
    ASSERT_TRUE(this->set->check_invariants()) << "Invariant check failed after stress test.";

    // Optional: Log final size for observation (not a strict check)
    size_t final_size = this->set->size();
    std::cout << "Stress test final size: " << final_size << std::endl;

    SUCCEED() << "Stress test completed without crashes and passed invariant check.";
}

// Add more TYPED_TEST cases here for other concurrency scenarios if needed
