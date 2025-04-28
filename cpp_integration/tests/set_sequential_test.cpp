#include <algorithm>  // For shuffle
#include <cstddef>    // For size_t
#include <gtest/gtest.h>
#include <memory>   // For std::unique_ptr
#include <numeric>  // For iota
#include <random>   // For mt19937
#include <vector>   // For vector

// Include the interface and all implementations
#include "data_structure/iset.h"
#include "data_structure/set_coarse.h"  // Included to allow testing sequential logic
#include "data_structure/set_fine.h"    // Included to allow testing sequential logic
#include "data_structure/set_sequential.h"

// Define the type for the elements in the set for testing
using TestSetElement = int;

// --- Test Fixture (Merged from set_test_fixtures.h) ---
template <typename SetType>
class SequentialSetLogicTest : public ::testing::Test
{
protected:
    // Use a pointer to the interface type
    std::unique_ptr<DataStructure::Set::ISet<TestSetElement>> set;

    void SetUp() override
    {
        // Create an instance of the specific SetType
        set = std::make_unique<SetType>();
    }

    // TearDown is handled automatically by std::unique_ptr
};

// --- Test Suite Definition ---

// Define the list of all set implementation types to be tested for sequential correctness
using AllSetImplementations =
    ::testing::Types<DataStructure::Set::SortedLinkedList_Sequential<TestSetElement>,
                     DataStructure::Set::SortedLinkedList_CoarseLock<TestSetElement>,
                     DataStructure::Set::SortedLinkedList_FineLock<TestSetElement>>;

// Register the typed test suite
TYPED_TEST_SUITE(SequentialSetLogicTest, AllSetImplementations);

// --- Typed Tests for Interface Conformance ---

TYPED_TEST(SequentialSetLogicTest, InitialIsEmpty)
{
    EXPECT_FALSE(this->set->contains(10));
    EXPECT_FALSE(this->set->contains(0));
    EXPECT_FALSE(this->set->contains(-10));
    EXPECT_EQ(this->set->size(), 0);  // Check initial size
}

TYPED_TEST(SequentialSetLogicTest, AddSingleElement)
{
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_TRUE(this->set->add(10));
    EXPECT_TRUE(this->set->contains(10));
    EXPECT_FALSE(this->set->contains(20));
    EXPECT_EQ(this->set->size(), 1);  // Check size after add
}

TYPED_TEST(SequentialSetLogicTest, AddMultipleElements)
{
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_TRUE(this->set->add(10));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_TRUE(this->set->add(5));
    EXPECT_EQ(this->set->size(), 2);
    EXPECT_TRUE(this->set->add(15));
    EXPECT_EQ(this->set->size(), 3);  // Check final size

    EXPECT_TRUE(this->set->contains(5));
    EXPECT_TRUE(this->set->contains(10));
    EXPECT_TRUE(this->set->contains(15));
    EXPECT_FALSE(this->set->contains(0));
    EXPECT_FALSE(this->set->contains(20));
}

TYPED_TEST(SequentialSetLogicTest, AddDuplicate)
{
    EXPECT_TRUE(this->set->add(20));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_TRUE(this->set->contains(20));
    EXPECT_FALSE(this->set->add(20));  // Adding duplicate should return false
    EXPECT_TRUE(this->set->contains(20));
    EXPECT_EQ(this->set->size(), 1);  // Check size remains unchanged
}

TYPED_TEST(SequentialSetLogicTest, RemoveExisting)
{
    this->set->add(30);
    EXPECT_EQ(this->set->size(), 1);  // Size before remove
    EXPECT_TRUE(this->set->contains(30));
    EXPECT_TRUE(this->set->remove(30));  // Should return true on successful remove
    EXPECT_FALSE(this->set->contains(30));
    EXPECT_EQ(this->set->size(), 0);  // Size after remove
}

TYPED_TEST(SequentialSetLogicTest, RemoveFromMultiple)
{
    this->set->add(10);
    this->set->add(20);
    this->set->add(30);
    EXPECT_EQ(this->set->size(), 3);  // Initial size

    EXPECT_TRUE(this->set->remove(20));  // Remove middle element
    EXPECT_TRUE(this->set->contains(10));
    EXPECT_FALSE(this->set->contains(20));
    EXPECT_TRUE(this->set->contains(30));
    EXPECT_EQ(this->set->size(), 2);  // Size after first remove

    EXPECT_TRUE(this->set->remove(10));  // Remove first element
    EXPECT_FALSE(this->set->contains(10));
    EXPECT_FALSE(this->set->contains(20));
    EXPECT_TRUE(this->set->contains(30));
    EXPECT_EQ(this->set->size(), 1);  // Size after second remove

    EXPECT_TRUE(this->set->remove(30));  // Remove last element
    EXPECT_FALSE(this->set->contains(10));
    EXPECT_FALSE(this->set->contains(20));
    EXPECT_FALSE(this->set->contains(30));
    EXPECT_EQ(this->set->size(), 0);  // Size after final remove
}

TYPED_TEST(SequentialSetLogicTest, RemoveNonExisting)
{
    EXPECT_TRUE(this->set->add(40));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_FALSE(this->set->remove(50));  // Element not present, should return false
    EXPECT_TRUE(this->set->contains(40));
    EXPECT_EQ(this->set->size(), 1);  // Size should remain unchanged
}

TYPED_TEST(SequentialSetLogicTest, RemoveFromEmpty)
{
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_FALSE(this->set->remove(10));
    EXPECT_EQ(this->set->size(), 0);  // Size remains 0
}

TYPED_TEST(SequentialSetLogicTest, AddRemoveSequence)
{
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_TRUE(this->set->add(1));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_TRUE(this->set->add(2));
    EXPECT_EQ(this->set->size(), 2);
    EXPECT_TRUE(this->set->contains(1));
    EXPECT_TRUE(this->set->contains(2));

    EXPECT_TRUE(this->set->remove(1));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_FALSE(this->set->contains(1));
    EXPECT_TRUE(this->set->contains(2));

    EXPECT_FALSE(this->set->add(2));
    EXPECT_EQ(this->set->size(), 1);

    EXPECT_TRUE(this->set->add(3));
    EXPECT_EQ(this->set->size(), 2);
    EXPECT_TRUE(this->set->contains(3));

    EXPECT_TRUE(this->set->remove(2));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_FALSE(this->set->contains(2));
    EXPECT_TRUE(this->set->contains(3));

    EXPECT_TRUE(this->set->remove(3));
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_FALSE(this->set->contains(3));

    EXPECT_FALSE(this->set->remove(1));
    EXPECT_FALSE(this->set->remove(2));
    EXPECT_FALSE(this->set->remove(3));
    EXPECT_EQ(this->set->size(), 0);
}

TYPED_TEST(SequentialSetLogicTest, ContainsAfterRemove)
{
    EXPECT_TRUE(this->set->add(100));
    EXPECT_TRUE(this->set->add(200));
    EXPECT_EQ(this->set->size(), 2);
    EXPECT_TRUE(this->set->contains(100));
    EXPECT_TRUE(this->set->contains(200));

    EXPECT_TRUE(this->set->remove(100));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_FALSE(this->set->contains(100));
    EXPECT_TRUE(this->set->contains(200));

    EXPECT_TRUE(this->set->remove(200));
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_FALSE(this->set->contains(100));
    EXPECT_FALSE(this->set->contains(200));
}

TYPED_TEST(SequentialSetLogicTest, LargeDataSetOperations)
{
    const int num_elements = 10000;
    std::vector<TestSetElement> values(num_elements);
    std::iota(values.begin(), values.end(), 0);

    std::mt19937 g(static_cast<unsigned int>(42));
    std::shuffle(values.begin(), values.end(), g);

    // Add all elements
    for (int i = 0; i < num_elements; ++i)
    {
        EXPECT_TRUE(this->set->add(values[i]));
        // Check size increases correctly, maybe less frequently for performance
        // EXPECT_EQ(this->set->size(), static_cast<size_t>(i + 1));
    }
    EXPECT_EQ(this->set->size(), static_cast<size_t>(num_elements));  // Check final size after adds

    // Verify all elements are contained
    for (int i = 0; i < num_elements; ++i)
    {
        EXPECT_TRUE(this->set->contains(i));
    }

    // Verify non-existent elements are not contained
    EXPECT_FALSE(this->set->contains(-1));
    EXPECT_FALSE(this->set->contains(num_elements));

    // Shuffle again before removing
    std::shuffle(values.begin(), values.end(), g);

    // Remove half the elements
    int mid_point = num_elements / 2;
    for (int i = 0; i < mid_point; ++i)
    {
        EXPECT_TRUE(this->set->remove(values[i]));
    }
    size_t expected_size_after_half_remove = static_cast<size_t>(num_elements - mid_point);
    EXPECT_EQ(this->set->size(),
              expected_size_after_half_remove);  // Check size after removing half

    // Verify removed/remaining elements
    std::vector<bool> should_be_present(num_elements, true);
    for (int i = 0; i < mid_point; ++i)
    {
        if (values[i] >= 0 && values[i] < num_elements)
        {
            should_be_present[values[i]] = false;
        }
    }
    for (int i = 0; i < num_elements; ++i)
    {
        EXPECT_EQ(this->set->contains(i), should_be_present[i]);
    }

    // Remove the remaining elements
    for (int i = mid_point; i < num_elements; ++i)
    {
        EXPECT_TRUE(this->set->remove(values[i]));
    }
    EXPECT_EQ(this->set->size(), 0);  // Check size is zero at the end

    // Verify the set is empty
    EXPECT_FALSE(this->set->contains(0));
    EXPECT_FALSE(this->set->contains(num_elements - 1));
}

TYPED_TEST(SequentialSetLogicTest, InterleavedAddRemove)
{
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_TRUE(this->set->add(10));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_TRUE(this->set->add(30));
    EXPECT_EQ(this->set->size(), 2);

    EXPECT_TRUE(this->set->add(20));
    EXPECT_EQ(this->set->size(), 3);

    EXPECT_TRUE(this->set->remove(10));
    EXPECT_EQ(this->set->size(), 2);
    EXPECT_FALSE(this->set->remove(10));  // Remove again fails
    EXPECT_EQ(this->set->size(), 2);

    EXPECT_TRUE(this->set->add(10));
    EXPECT_EQ(this->set->size(), 3);

    EXPECT_TRUE(this->set->remove(30));
    EXPECT_EQ(this->set->size(), 2);
    EXPECT_TRUE(this->set->remove(20));
    EXPECT_EQ(this->set->size(), 1);
    EXPECT_FALSE(this->set->add(10));  // Add duplicate fails
    EXPECT_EQ(this->set->size(), 1);

    EXPECT_TRUE(this->set->remove(10));
    EXPECT_EQ(this->set->size(), 0);
    EXPECT_FALSE(this->set->remove(10));  // Remove from empty fails
    EXPECT_FALSE(this->set->remove(20));
    EXPECT_EQ(this->set->size(), 0);
}
