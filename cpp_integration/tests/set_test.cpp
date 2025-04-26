#include <gtest/gtest.h>

// Include the headers for the data structures you want to test
#include "data_structure/iset.h"  // Include the interface
#include "data_structure/set_coarse.h"
#include "data_structure/set_fine.h"
#include "data_structure/set_sequential.h"

// Example Test Fixture (Optional but good practice)
class SetTest : public ::testing::Test
{
protected:
    // Can set up common objects here if needed
    // void SetUp() override {}
    // void TearDown() override {}

    // Example instances (using int for simplicity)
    DataStructure::Set::SortedLinkedList_Sequential<int> seq_set;
    DataStructure::Set::SortedLinkedList_CoarseLock<int> coarse_set;
    DataStructure::Set::SortedLinkedList_FineLock<int> fine_set;
};

// Basic test for the sequential implementation
TEST_F(SetTest, SequentialAddContains)
{
    EXPECT_FALSE(seq_set.contains(10));
    EXPECT_TRUE(seq_set.add(10));
    EXPECT_TRUE(seq_set.contains(10));
    EXPECT_FALSE(seq_set.add(10));  // Cannot add duplicate
}

TEST_F(SetTest, SequentialRemove)
{
    seq_set.add(20);
    EXPECT_TRUE(seq_set.contains(20));
    EXPECT_TRUE(seq_set.remove(20));
    EXPECT_FALSE(seq_set.contains(20));
    EXPECT_FALSE(seq_set.remove(20));  // Cannot remove non-existent
}

// --- Add similar tests for CoarseLock and FineLock ---

TEST_F(SetTest, CoarseAddContains)
{
    EXPECT_FALSE(coarse_set.contains(10));
    EXPECT_TRUE(coarse_set.add(10));
    EXPECT_TRUE(coarse_set.contains(10));
    EXPECT_FALSE(coarse_set.add(10));
}

TEST_F(SetTest, FineAddContains)
{
    EXPECT_FALSE(fine_set.contains(10));
    EXPECT_TRUE(fine_set.add(10));
    EXPECT_TRUE(fine_set.contains(10));
    EXPECT_FALSE(fine_set.add(10));
}

// You can add more tests using TEST() or TEST_F() macros
