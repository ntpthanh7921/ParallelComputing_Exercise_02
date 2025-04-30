#include <gtest/gtest.h>
#include <optional>
#include <utility>  // For std::pair
#include <vector>

// Include the interface and the implementation to be tested
#include "data_structure/ipq.h"
#include "data_structure/pq_fine.h"  // Header containing the implementation

// --- Test Element Type ---
// Using a pair to test FIFO: {priority, sequence_id}
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
class SequentialPriorityQueueLogicTest : public ::testing::Test
{
protected:
    // Use a pointer to the interface type
    std::unique_ptr<DataStructure::PriorityQueue::IPriorityQueue<TestPQElement>> pq;

    void SetUp() override
    {
        // Create an instance of the specific Priority Queue Type
        pq = std::make_unique<PQType>();
    }

    // TearDown is handled automatically by std::unique_ptr
};

// --- Test Suite Definition ---

// Define the list of priority queue implementation types to be tested sequentially.
// Add other sequential/concurrent implementations here later if needed for logic tests.
using PriorityQueueImplementations = ::testing::Types<
    DataStructure::PriorityQueue::SortedLinkedList_FineLockPQ<TestPQElement, ComparePriorityOnly>>;

// Register the typed test suite
TYPED_TEST_SUITE(SequentialPriorityQueueLogicTest, PriorityQueueImplementations);

// --- Typed Tests ---

TYPED_TEST(SequentialPriorityQueueLogicTest, InitialIsEmpty)
{
    EXPECT_TRUE(this->pq->empty());
    EXPECT_EQ(this->pq->size(), 0);
    EXPECT_FALSE(this->pq->pop().has_value());  // Pop on empty returns nullopt
    ASSERT_TRUE(this->pq->check_invariants());
}

TYPED_TEST(SequentialPriorityQueueLogicTest, PushOnePopOne)
{
    TestPQElement item = {10, 1};
    this->pq->push(item);

    EXPECT_FALSE(this->pq->empty());
    EXPECT_EQ(this->pq->size(), 1);
    ASSERT_TRUE(this->pq->check_invariants());

    std::optional<TestPQElement> popped = this->pq->pop();
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped.value(), item);  // Check value

    EXPECT_TRUE(this->pq->empty());
    EXPECT_EQ(this->pq->size(), 0);
    EXPECT_FALSE(this->pq->pop().has_value());
    ASSERT_TRUE(this->pq->check_invariants());
}

TYPED_TEST(SequentialPriorityQueueLogicTest, PushMultiplePopOrder)
{
    this->pq->push({5, 1});
    this->pq->push({1, 2});  // Lowest priority
    this->pq->push({3, 3});

    EXPECT_EQ(this->pq->size(), 3);
    ASSERT_TRUE(this->pq->check_invariants());

    // Expect highest priority (largest first element) first
    std::optional<TestPQElement> p1 = this->pq->pop();
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(p1.value().first, 5);
    EXPECT_EQ(this->pq->size(), 2);

    std::optional<TestPQElement> p2 = this->pq->pop();
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(p2.value().first, 3);
    EXPECT_EQ(this->pq->size(), 1);

    std::optional<TestPQElement> p3 = this->pq->pop();
    ASSERT_TRUE(p3.has_value());
    EXPECT_EQ(p3.value().first, 1);
    EXPECT_EQ(this->pq->size(), 0);

    EXPECT_TRUE(this->pq->empty());
    EXPECT_FALSE(this->pq->pop().has_value());
    ASSERT_TRUE(this->pq->check_invariants());
}

TYPED_TEST(SequentialPriorityQueueLogicTest, PushEqualPriorityFIFO)
{
    // Push three items with the same priority (5) but different sequence IDs
    TestPQElement itemA = {5, 101};  // First pushed
    TestPQElement itemB = {5, 102};  // Second pushed
    TestPQElement itemC = {5, 103};  // Third pushed

    this->pq->push(itemA);
    this->pq->push(itemB);
    this->pq->push(itemC);

    EXPECT_EQ(this->pq->size(), 3);
    ASSERT_TRUE(this->pq->check_invariants());

    // Pop them - expect FIFO order (A, then B, then C) because priority is equal
    std::optional<TestPQElement> p1 = this->pq->pop();
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(p1.value().first, 5);
    EXPECT_EQ(p1.value().second, 101);  // Expect A (first one pushed)

    std::optional<TestPQElement> p2 = this->pq->pop();
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(p2.value().first, 5);
    EXPECT_EQ(p2.value().second, 102);  // Expect B (second one pushed)

    std::optional<TestPQElement> p3 = this->pq->pop();
    ASSERT_TRUE(p3.has_value());
    EXPECT_EQ(p3.value().first, 5);
    EXPECT_EQ(p3.value().second, 103);  // Expect C (third one pushed)

    EXPECT_TRUE(this->pq->empty());
    ASSERT_TRUE(this->pq->check_invariants());
}

TYPED_TEST(SequentialPriorityQueueLogicTest, InterleavedPushPop)
{
    this->pq->push({10, 1});
    this->pq->push({30, 2});  // Pushed second, highest prio
    this->pq->push({20, 3});
    ASSERT_EQ(this->pq->size(), 3);

    // Pop highest (30)
    auto p1 = this->pq->pop();
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(p1.value().first, 30);
    ASSERT_EQ(this->pq->size(), 2);

    // Push another high one
    this->pq->push({40, 4});
    ASSERT_EQ(this->pq->size(), 3);

    // Pop highest (40)
    auto p2 = this->pq->pop();
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(p2.value().first, 40);
    ASSERT_EQ(this->pq->size(), 2);

    // Pop next highest (20)
    auto p3 = this->pq->pop();
    ASSERT_TRUE(p3.has_value());
    EXPECT_EQ(p3.value().first, 20);
    ASSERT_EQ(this->pq->size(), 1);

    // Push equal priority (10) - should come after existing 10 due to FIFO
    this->pq->push({10, 5});
    ASSERT_EQ(this->pq->size(), 2);

    // Pop original 10 (pushed first)
    auto p4 = this->pq->pop();
    ASSERT_TRUE(p4.has_value());
    EXPECT_EQ(p4.value().first, 10);
    EXPECT_EQ(p4.value().second, 1);  // Verify it's the first one pushed
    ASSERT_EQ(this->pq->size(), 1);

    // Pop second 10
    auto p5 = this->pq->pop();
    ASSERT_TRUE(p5.has_value());
    EXPECT_EQ(p5.value().first, 10);
    EXPECT_EQ(p5.value().second, 5);
    ASSERT_EQ(this->pq->size(), 0);

    EXPECT_TRUE(this->pq->empty());
    ASSERT_TRUE(this->pq->check_invariants());
}

TYPED_TEST(SequentialPriorityQueueLogicTest, PopEmpty)
{
    EXPECT_TRUE(this->pq->empty());
    EXPECT_FALSE(this->pq->pop().has_value());  // Pop on initially empty

    this->pq->push({1, 1});
    ASSERT_FALSE(this->pq->empty());
    auto p1 = this->pq->pop();
    ASSERT_TRUE(p1.has_value());
    EXPECT_TRUE(this->pq->empty());

    EXPECT_FALSE(this->pq->pop().has_value());  // Pop on queue that became empty
    ASSERT_TRUE(this->pq->check_invariants());
}

TYPED_TEST(SequentialPriorityQueueLogicTest, LargeDataSet)
{
    const int num_elements = 5000;
    std::vector<TestPQElement> values;
    values.reserve(num_elements);
    for (int i = 0; i < num_elements; ++i)
    {
        // Create elements with varying priorities and unique sequence IDs
        values.push_back({rand() % 1000, i});
    }

    // Push all elements
    for (const auto &val : values)
    {
        this->pq->push(val);
    }
    EXPECT_EQ(this->pq->size(), num_elements);
    ASSERT_TRUE(this->pq->check_invariants());

    // Pop all elements and check order
    TestPQElement last_popped = {std::numeric_limits<int>::max(),
                                 0};  // Initialize with max possible prio
    size_t pop_count = 0;
    while (true)
    {
        std::optional<TestPQElement> current_popped_opt = this->pq->pop();
        if (!current_popped_opt)
        {
            break;  // Queue is empty
        }
        pop_count++;
        TestPQElement current_popped = current_popped_opt.value();

        // Check that priorities are non-increasing
        EXPECT_LE(current_popped.first, last_popped.first);

        // If priorities are equal, check FIFO (sequence IDs non-decreasing)
        if (current_popped.first == last_popped.first)
        {
            EXPECT_LE(last_popped.second, current_popped.second);
        }

        last_popped = current_popped;
    }

    EXPECT_EQ(pop_count, num_elements);
    EXPECT_TRUE(this->pq->empty());
    EXPECT_EQ(this->pq->size(), 0);
    ASSERT_TRUE(this->pq->check_invariants());
}
