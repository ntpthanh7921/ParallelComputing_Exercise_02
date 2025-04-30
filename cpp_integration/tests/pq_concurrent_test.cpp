#include <algorithm>  // For std::max
#include <atomic>
#include <chrono>
#include <functional>  // For std::hash
#include <gtest/gtest.h>
#include <optional>
#include <random>
#include <thread>
#include <utility>  // For std::pair
#include <vector>

// Include the interface and the implementation to be tested
#include "data_structure/ipq.h"
#include "data_structure/pq_fine.h"  // Header containing the implementation

// --- Test Element Type ---
// Using a pair to test: {priority, sequence_id}
// Higher priority = larger first element.
using TestPQElement = std::pair<int, int>;

// --- std::numeric_limits Specialization for TestPQElement ---
// Required because the priority queue implementation uses lowest()/max() for sentinels.
// We define limits based on the priority (the first element of the pair).
namespace std
{  // Must be in the std namespace

template <>
struct numeric_limits<TestPQElement>
{
    static constexpr bool is_specialized = true;

    static constexpr TestPQElement lowest() noexcept
    {
        // Lowest priority based on int limits, sequence ID can be 0 or anything.
        return {numeric_limits<int>::lowest(), 0};
    }

    static constexpr TestPQElement max() noexcept
    {
        // Highest priority based on int limits, sequence ID can be 0 or anything.
        return {numeric_limits<int>::max(), 0};
    }

    // Add other members if needed, though lowest() and max() are the critical ones here.
    // Default values for other members are often sufficient if not used explicitly.
    static constexpr int digits = numeric_limits<int>::digits;
};

}  // namespace std

// Define a custom less-than comparison for TestPQElement (compares priority only)
// This determines the order within the SortedLinkedList implementation.
struct ComparePriorityOnly
{
    bool operator()(const TestPQElement &lhs, const TestPQElement &rhs) const
    {
        return lhs.first < rhs.first;
    }
};

// --- Test Fixture ---
template <typename PQType>
class ConcurrentPriorityQueueCorrectnessTest : public ::testing::Test
{
protected:
    // Use a pointer to the interface type
    std::unique_ptr<DataStructure::PriorityQueue::IPriorityQueue<TestPQElement>> pq;

    // Constants for tests
    static constexpr int DEFAULT_NUM_THREADS = 4;
    static constexpr int OPS_PER_THREAD = 500;

    void SetUp() override
    {
        // Create an instance of the specific Priority Queue Type
        pq = std::make_unique<PQType>();
    }

    // TearDown is handled automatically by std::unique_ptr
};

// --- Test Suite Definition ---

// Define the list of concurrent priority queue implementation types to be tested.
using ConcurrentPriorityQueueImplementations = ::testing::Types<
    DataStructure::PriorityQueue::SortedLinkedList_FineLockPQ<TestPQElement, ComparePriorityOnly>
    // Add other concurrent implementations here later if needed
    >;

// Register the typed test suite
TYPED_TEST_SUITE(ConcurrentPriorityQueueCorrectnessTest, ConcurrentPriorityQueueImplementations);

// --- Typed Concurrent Tests ---

// Tests concurrent pushes of unique items.
TYPED_TEST(ConcurrentPriorityQueueCorrectnessTest, ConcurrentPush)
{
    std::vector<std::thread> threads;
    int num_threads = this->DEFAULT_NUM_THREADS;
    int ops_per_thread = this->OPS_PER_THREAD;
    int total_items = num_threads * ops_per_thread;

    // Generate unique items for each thread to push
    std::vector<std::vector<TestPQElement>> thread_items(num_threads);
    int sequence_counter = 0;
    for (int i = 0; i < num_threads; ++i)
    {
        thread_items[i].reserve(ops_per_thread);
        for (int j = 0; j < ops_per_thread; ++j)
        {
            // Example: Assign priority based on thread, unique sequence ID
            thread_items[i].push_back({i * 10 + rand() % 10, sequence_counter++});
        }
    }

    // Launch threads
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(
            [this, &thread_items, i]()
            {
                for (const auto &item : thread_items[i])
                {
                    this->pq->push(item);
                }
            });
    }

    // Wait for threads to complete
    for (auto &t : threads)
    {
        t.join();
    }

    // Verification
    EXPECT_EQ(this->pq->size(), static_cast<size_t>(total_items));
    ASSERT_TRUE(this->pq->check_invariants());

    // Optional: Sequentially pop and verify count again
    size_t pop_count = 0;
    while (this->pq->pop().has_value())
    {
        pop_count++;
    }
    EXPECT_EQ(pop_count, static_cast<size_t>(total_items));
    EXPECT_TRUE(this->pq->empty());
}

// Tests concurrent pops from a pre-filled queue.
TYPED_TEST(ConcurrentPriorityQueueCorrectnessTest, ConcurrentPop)
{
    int num_threads = this->DEFAULT_NUM_THREADS;
    int initial_items = num_threads * this->OPS_PER_THREAD * 2;  // Fill with more items

    // Pre-populate the queue
    for (int i = 0; i < initial_items; ++i)
    {
        this->pq->push({rand() % 1000, i});
    }
    ASSERT_EQ(this->pq->size(), static_cast<size_t>(initial_items));
    ASSERT_TRUE(this->pq->check_invariants());

    std::vector<std::thread> threads;
    std::atomic<int> successful_pops{0};

    // Launch threads to pop concurrently
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(
            [this, &successful_pops]()
            {
                while (true)
                {
                    std::optional<TestPQElement> item = this->pq->pop();
                    if (item.has_value())
                    {
                        successful_pops++;
                    }
                    else
                    {
                        // Queue might be empty or becoming empty
                        // Check size again under potential race, or just break if pop failed
                        if (this->pq->empty())
                            break;
                        // Small yield might prevent busy-waiting on contention
                        std::this_thread::yield();
                    }
                }
            });
    }

    // Wait for threads
    for (auto &t : threads)
    {
        t.join();
    }

    // Verification
    EXPECT_EQ(successful_pops.load(), initial_items);
    EXPECT_TRUE(this->pq->empty());
    EXPECT_EQ(this->pq->size(), 0);
    ASSERT_TRUE(this->pq->check_invariants());
}

// Tests concurrent mixed push/pop operations, checking only invariants after completion.
TYPED_TEST(ConcurrentPriorityQueueCorrectnessTest, ConcurrentMixedOpsInvariantCheck)
{
    std::vector<std::thread> threads;
    int num_threads = this->DEFAULT_NUM_THREADS;
    int ops_per_thread = this->OPS_PER_THREAD;
    const int VALUE_RANGE = 1000;  // Priority range

    // Pre-populate slightly to ensure pops have something to target initially
    for (int i = 0; i < num_threads * ops_per_thread / 4; ++i)
    {
        this->pq->push({rand() % VALUE_RANGE, i});
    }
    int initial_seq_id = num_threads * ops_per_thread / 4;

    // Launch threads performing mixed operations
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(
            [this, ops_per_thread, i, initial_seq_id]()
            {
                std::mt19937 gen(std::random_device{}() ^ (i + 1));  // Per-thread seed
                std::uniform_int_distribution<> op_dist(0, 1);       // 0: push, 1: pop
                std::uniform_int_distribution<> prio_dist(0, VALUE_RANGE - 1);

                for (int k = 0; k < ops_per_thread; ++k)
                {
                    if (op_dist(gen) == 0)
                    {
                        // Push: Use thread-specific range for sequence IDs to ensure uniqueness
                        int seq_id = initial_seq_id + i * ops_per_thread + k;
                        this->pq->push({prio_dist(gen), seq_id});
                    }
                    else
                    {
                        // Pop: Ignore return value, just perform the operation
                        this->pq->pop();
                    }
                }
            });
    }

    // Wait for threads
    for (auto &t : threads)
    {
        t.join();
    }

    // Verification: Primarily check for crashes, deadlocks, and internal consistency
    ASSERT_TRUE(this->pq->check_invariants()) << "Invariant check failed post-concurrency.";
    // Final size is non-deterministic, but we can log it for observation
    // std::cout << "Final size after mixed ops: " << this->pq->size() << std::endl;
    SUCCEED() << "ConcurrentMixedOps completed and passed invariant check.";
}

// Stress test running for a fixed duration, checking for crashes and invariant violations.
TYPED_TEST(ConcurrentPriorityQueueCorrectnessTest, StressTestDuration10Seconds)
{
    // --- Configuration ---
    const int DURATION_SECONDS = 10;  // Shorter duration for typical test runs
    const int NUM_STRESS_THREADS =
        std::max(2u, std::thread::hardware_concurrency());  // Use available cores
    const int VALUE_RANGE_STRESS = 500;                     // Smaller range increases contention
    const int PUSH_PERCENT = 50;                            // Percentage chance of Push vs Pop

    // --- Setup ---
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> sequence_id_counter{0};  // Shared counter for unique IDs

    // --- Thread Workload ---
    auto thread_func = [&]()
    {
        // Seed each thread's random number generator differently
        std::mt19937 gen(
            std::random_device{}()
            ^ static_cast<unsigned int>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
        std::uniform_int_distribution<> op_dist(1, 100);
        std::uniform_int_distribution<> prio_dist(0, VALUE_RANGE_STRESS - 1);

        // Loop until the stop flag is set
        while (!stop_flag.load(std::memory_order_acquire))
        {
            int op_choice = op_dist(gen);

            if (op_choice <= PUSH_PERCENT)
            {
                int priority = prio_dist(gen);
                int seq_id = sequence_id_counter.fetch_add(1, std::memory_order_relaxed);
                this->pq->push({priority, seq_id});
            }
            else
            {
                this->pq->pop();  // Ignore return value
            }
            // Optional: Small yield/sleep to prevent pure busy-waiting if desired
            // std::this_thread::yield();
        }
    };

    // --- Start Threads ---
    // std::cout << "Starting stress test with " << NUM_STRESS_THREADS << " threads for "
    //           << DURATION_SECONDS << " seconds..." << std::endl;
    for (int i = 0; i < NUM_STRESS_THREADS; ++i)
    {
        threads.emplace_back(thread_func);
    }

    // --- Let threads run ---
    std::this_thread::sleep_for(std::chrono::seconds(DURATION_SECONDS));
    stop_flag.store(true, std::memory_order_release);  // Signal threads to stop

    // --- Join Threads ---
    for (auto &t : threads)
    {
        t.join();
    }
    // std::cout << "Stress test threads joined." << std::endl;

    // --- Verification ---
    // Primary check: Did it survive and are invariants okay?
    ASSERT_TRUE(this->pq->check_invariants()) << "Invariant check failed after stress test.";
    // Optional: Log final size
    // std::cout << "Stress test final size: " << this->pq->size() << std::endl;
    SUCCEED() << "Stress test completed without crashes and passed invariant check.";
}
