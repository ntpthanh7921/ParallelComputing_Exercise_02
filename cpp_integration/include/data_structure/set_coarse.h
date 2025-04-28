#pragma once

#include "iset.h"   // Include the common interface
#include <atomic>   // For std::atomic
#include <cstddef>  // For size_t
#include <limits>
#include <mutex>  // For std::unique_lock, std::lock_guard
#include <new>
#include <shared_mutex>  // For std::shared_mutex, std::shared_lock
#include <stdexcept>

// Removed <thread>

namespace DataStructure
{
namespace Set
{

// Node structure
template <typename T>
struct CoarseNode
{
    T val;
    CoarseNode<T> *next;

    CoarseNode(T v, CoarseNode<T> *n = nullptr) : val(v), next(n) { }
};

// Coarse-Grained Locking Implementation
template <typename T>
class SortedLinkedList_CoarseLock : public ISet<T>
{
private:
    CoarseNode<T> *head;
    CoarseNode<T> *tail;
    mutable std::shared_mutex list_mutex;  // Mutex for the whole list
    std::atomic<size_t> current_size{0};   // Atomic counter for size

    // Helper: find predecessor node (unsafe, requires external lock)
    CoarseNode<T> *find_node_unsafe(const T &val) const
    {
        CoarseNode<T> *pred = head;
        CoarseNode<T> *curr = head->next;
        while (curr != tail && curr->val < val)
        {
            pred = curr;
            curr = curr->next;
        }
        return pred;
    }

public:
    // Constructor: Initializes sentinels
    SortedLinkedList_CoarseLock() : head(nullptr), tail(nullptr)
    {
        try
        {
            T min_val, max_val;
            if constexpr (std::numeric_limits<T>::is_specialized)
            {
                min_val = std::numeric_limits<T>::lowest();
                max_val = std::numeric_limits<T>::max();
            }
            else
            {
                throw std::logic_error("Type T requires std::numeric_limits specialization.");
            }
            // Allocate tail first, then head pointing to tail
            tail = new CoarseNode<T>(max_val, nullptr);
            head = new CoarseNode<T>(min_val, tail);
        }
        catch (...)
        {
            // Clean up if allocation fails
            delete head;
            delete tail;
            throw std::runtime_error("SortedLinkedList_CoarseLock construction failed.");
        }
    }

    // Destructor: Cleans up all nodes including sentinels
    ~SortedLinkedList_CoarseLock() override
    {
        // No locking needed during destruction (assume single-threaded context)
        CoarseNode<T> *current = head;
        while (current != nullptr)
        {
            CoarseNode<T> *next_node = current->next;
            delete current;
            current = next_node;
        }
    }

    // Deleted copy constructor and assignment operator
    SortedLinkedList_CoarseLock(const SortedLinkedList_CoarseLock &) = delete;
    SortedLinkedList_CoarseLock &operator=(const SortedLinkedList_CoarseLock &) = delete;

    // Default move constructor and assignment operator
    SortedLinkedList_CoarseLock(SortedLinkedList_CoarseLock &&) = default;
    SortedLinkedList_CoarseLock &operator=(SortedLinkedList_CoarseLock &&) = default;

    // --- ISet Interface Methods ---

    // Checks if a value is present in the set.
    bool contains(const T &val) const override
    {
        std::shared_lock<std::shared_mutex> lock(list_mutex);  // Read lock
        CoarseNode<T> *pred = find_node_unsafe(val);
        CoarseNode<T> *curr = pred->next;
        return (curr != tail && curr->val == val);
    }

    // Adds a value to the set. Returns true if added, false if already present.
    bool add(const T &val) override
    {
        std::unique_lock<std::shared_mutex> lock(list_mutex);  // Write lock
        CoarseNode<T> *pred = find_node_unsafe(val);
        CoarseNode<T> *curr = pred->next;
        bool success = false;

        if (curr != tail && curr->val == val)
        {
            // Value already exists
            success = false;
        }
        else
        {
            // Add the node
            try
            {
                CoarseNode<T> *new_node = new CoarseNode<T>(val, curr);
                pred->next = new_node;
                current_size.fetch_add(1, std::memory_order_relaxed);  // Increment size
                success = true;
            }
            catch (const std::bad_alloc &e)
            {
                // Lock released automatically by unique_lock destructor
                throw std::runtime_error("Failed node allocation in add: " + std::string(e.what()));
            }
        }
        return success;
    }

    // Removes a value from the set. Returns true if removed, false if not found.
    bool remove(const T &val) override
    {
        std::unique_lock<std::shared_mutex> lock(list_mutex);  // Write lock
        CoarseNode<T> *pred = find_node_unsafe(val);
        CoarseNode<T> *curr = pred->next;
        bool success = false;

        if (curr != tail && curr->val == val)
        {
            // Found node to remove
            pred->next = curr->next;
            // Decrement size *before* deleting node to maintain consistency if delete throws
            // (unlikely here)
            current_size.fetch_sub(1, std::memory_order_relaxed);
            delete curr;  // Deallocate the removed node
            success = true;
        }
        else
        {
            // Value not found
            success = false;
        }
        return success;
    }

    // Returns the number of elements currently in the set.
    size_t size() const override
    {
        // Relaxed memory order is sufficient for reading the counter
        return current_size.load(std::memory_order_relaxed);
    }

    // Checks internal invariants (e.g., sorted order). Requires read lock.
    bool check_invariants() const override
    {
        std::shared_lock<std::shared_mutex> lock(list_mutex);  // Read lock

        CoarseNode<T> *pred = head;
        CoarseNode<T> *curr = head->next;

        // Basic structural checks
        if (!head || !tail || head->next == nullptr)
            return false;

        // Traverse and check sorted order
        while (curr != tail)
        {
            if (!curr)
                return false;  // Should not encounter null before tail
            if (pred->val > curr->val)
                return false;  // Check sorted order
            pred = curr;
            curr = curr->next;
        }
        // Final check: last non-tail node must point to tail
        return (pred->next == tail);
    }
};

}  // namespace Set
}  // namespace DataStructure
