#pragma once

#include "ipq.h"  // Include the interface definition
#include <atomic>
#include <cassert>  // For assert()
#include <cstddef>
#include <functional>  // For std::less (default comparator)
#include <limits>
#include <memory>  // For placement new header <new> and std::destroy_at
#include <mutex>
#include <optional>  // For pop() return type
#include <utility>   // For std::pair, std::move

namespace DataStructure
{
namespace PriorityQueue
{

/**
 * @brief Node structure for the fine-grained locking priority queue.
 * @tparam T Element type.
 */
template <typename T>
struct FinePQNode
{
    T val;
    FinePQNode<T> *next;
    std::mutex node_mutex;  // Mutex protecting this node's state (primarily 'next')

    // Constructor for data nodes (moves value)
    FinePQNode(T v, FinePQNode<T> *n = nullptr) : val(std::move(v)), next(n) { }

    // Constructor for sentinel nodes (value initialized separately)
    FinePQNode() : next(nullptr) { }

    // Non-copyable and non-movable due to mutex
    FinePQNode(const FinePQNode &) = delete;
    FinePQNode &operator=(const FinePQNode &) = delete;
    FinePQNode(FinePQNode &&) = delete;
    FinePQNode &operator=(FinePQNode &&) = delete;

    // Lock/Unlock convenience methods
    void lock() { node_mutex.lock(); }

    void unlock() { node_mutex.unlock(); }
};

//----------------------------------------------------------------------------

/**
 * @brief A fine-grained locking priority queue based on a sorted linked list
 * with sentinel nodes.
 *
 * Implements the IPriorityQueue interface. Uses mutexes on each node.
 * Highest priority element is at the tail. Pop removes from the tail.
 * Supports concurrent push and pop operations. FIFO for equal priorities.
 *
 * @tparam T Element type. Must have numeric_limits specialized.
 * @tparam Compare Comparison function object type. Defaults to std::less<T>,
 * resulting in smaller values having lower priority (max heap).
 * Use std::greater<T> for a min heap. Comparison defines order.
 */
template <typename T, class Compare = std::less<T>>  // Added Compare parameter
class SortedLinkedList_FineLockPQ : public IPriorityQueue<T>
{
private:
    FinePQNode<T> *head;                  // Pointer to head sentinel (lowest value)
    FinePQNode<T> *tail;                  // Pointer to tail sentinel (highest value)
    std::atomic<size_t> current_size{0};  // Tracks number of *data* nodes
    Compare comp;                         // Comparator instance

    // Helper: Locates and locks predecessor and current nodes using Hand-over-Hand.
    // Finds position based on the 'comp' comparator to maintain sorted order.
    // Returns {pred, curr} locked. Caller must unlock. Sentinels ensure non-null.
    std::pair<FinePQNode<T> *, FinePQNode<T> *> find_and_lock_for_push(const T &val) const
    {
        FinePQNode<T> *pred = head;
        pred->lock();
        FinePQNode<T> *curr = head->next;
        assert(curr != nullptr);  // Sentinel invariant
        curr->lock();

        // Traverse using hand-over-hand, using the comparator 'comp'
        // Stops when 'curr' is tail or comp(curr->val, val) is false (i.e., curr->val >= val)
        while (curr != tail && comp(curr->val, val))  // Use comparator comp(a, b) for a < b
        {
            pred->unlock();
            pred = curr;  // pred remains locked
            curr = curr->next;
            assert(curr != nullptr);  // Sentinel invariant
            curr->lock();
        }
        // Found position: pred locked, curr locked. Insert between pred and curr.
        return {pred, curr};
    }

public:
    // Constructor: Initialize with sentinels
    SortedLinkedList_FineLockPQ()
        : head(nullptr), tail(nullptr), comp()  // Default construct comparator
    {
        // Check if T supports numeric_limits - required for sentinels
        static_assert(std::numeric_limits<T>::is_specialized,
                      "Type T requires std::numeric_limits specialization for sentinels.");

        T min_val = std::numeric_limits<T>::lowest();
        T max_val = std::numeric_limits<T>::max();

        // Allocate sentinels (can throw std::bad_alloc)
        head = new FinePQNode<T>();
        tail = new FinePQNode<T>();

        // Use placement new to construct values within sentinels
        try
        {
            new (&head->val) T(min_val);
            new (&tail->val) T(max_val);
        }
        catch (...)
        {
            delete head;  // Cleanup if value construction fails
            delete tail;
            throw;  // Re-throw the exception from T's constructor
        }

        head->next = tail;  // Link head to tail
    }

    // Destructor: Cleans up all nodes including sentinels
    ~SortedLinkedList_FineLockPQ() override
    {
        FinePQNode<T> *current = head;
        while (current != nullptr)
        {
            FinePQNode<T> *next_node = current->next;
            // Explicitly call destructor for sentinel values constructed with placement new
            if (current == head || current == tail)
            {
                std::destroy_at(&current->val);  // C++20
            }
            delete current;
            current = next_node;
        }
    }

    // Disable copy/move due to pointers and mutexes
    SortedLinkedList_FineLockPQ(const SortedLinkedList_FineLockPQ &) = delete;
    SortedLinkedList_FineLockPQ &operator=(const SortedLinkedList_FineLockPQ &) = delete;
    SortedLinkedList_FineLockPQ(SortedLinkedList_FineLockPQ &&) = delete;
    SortedLinkedList_FineLockPQ &operator=(SortedLinkedList_FineLockPQ &&) = delete;

    // --- IPriorityQueue Interface Methods ---

    void push(const T &val) override
    {
        // Allocate node first (can throw bad_alloc)
        FinePQNode<T> *new_node = new FinePQNode<T>(val);
        FinePQNode<T> *pred = nullptr;
        FinePQNode<T> *curr = nullptr;

        try
        {
            // Find insertion point using helper (pred and curr will be locked)
            std::tie(pred, curr) = find_and_lock_for_push(val);

            // Link the new node: pred -> new_node -> curr
            new_node->next = curr;
            pred->next = new_node;

            current_size.fetch_add(1, std::memory_order_relaxed);

            // Unlock nodes
            curr->unlock();
            pred->unlock();
        }
        catch (...)
        {
            // If find or locking fails, ensure allocated node is deleted
            delete new_node;
            // Attempt cleanup - robust error handling here is complex
            if (curr)
                curr->unlock();
            if (pred)
                pred->unlock();
            throw;  // Re-throw
        }
    }

    std::optional<T> pop() override
    {
        FinePQNode<T> *pred_pred = nullptr;  // Node before the node to delete
        FinePQNode<T> *node_to_delete = nullptr;
        FinePQNode<T> *tail_sentinel_locked = nullptr;  // Tail sentinel alias

        // Acquire locks hand-over-hand to find node before tail
        pred_pred = head;
        pred_pred->lock();
        node_to_delete = head->next;
        assert(node_to_delete != nullptr);  // Invariant check
        node_to_delete->lock();

        // Check if empty (only sentinels exist)
        if (node_to_delete == tail)
        {
            node_to_delete->unlock();  // Unlock tail
            pred_pred->unlock();       // Unlock head
            return std::nullopt;       // Return empty optional
        }

        // Traverse until node_to_delete->next is the tail sentinel
        while (node_to_delete->next != tail)
        {
            FinePQNode<T> *next_node = node_to_delete->next;
            assert(next_node != nullptr);  // Invariant check
            next_node->lock();
            pred_pred->unlock();
            pred_pred = node_to_delete;  // pred_pred is locked
            node_to_delete = next_node;  // node_to_delete is locked
        }

        // Now: pred_pred locked, node_to_delete locked, tail not locked yet
        tail_sentinel_locked = node_to_delete->next;  // Tail sentinel pointer
        tail_sentinel_locked->lock();                 // Lock tail sentinel

        // Retrieve value before unlinking using move semantics
        T value_to_return = std::move(node_to_delete->val);

        // Perform the unlink
        pred_pred->next = tail_sentinel_locked;  // Link node before target to tail

        current_size.fetch_sub(1, std::memory_order_relaxed);

        // Unlock all three nodes *before* deleting
        tail_sentinel_locked->unlock();
        node_to_delete->unlock();
        pred_pred->unlock();

        // Delete the node (Consider deferred reclamation for production systems)
        delete node_to_delete;

        return {std::move(value_to_return)};  // Return moved value in optional
    }

    bool empty() const override
    {
        // Reading atomic variable is thread-safe.
        return current_size.load(std::memory_order_acquire) == 0;
    }

    size_t size() const override
    {
        // Reading atomic variable is thread-safe.
        return current_size.load(std::memory_order_acquire);
    }

    bool check_invariants() const override
    {
        // WARNING: NOT THREAD-SAFE - Assumes list is quiescent. No locks acquired.
        [[maybe_unused]] FinePQNode<T> *pred = head;  // Decorated to avoid unused warning
        FinePQNode<T> *curr = head->next;
        assert(head != nullptr && tail != nullptr);  // Basic sentinel check

        [[maybe_unused]] size_t count = 0;  // Decorated to avoid unused warning

        while (curr != tail)
        {
            assert(curr != nullptr);  // Should not encounter null before tail

            // Check non-decreasing order using the comparator 'comp'
            // comp(a, b) means a < b. We want !(curr < pred).
            assert(!comp(curr->val, pred->val));

            pred = curr;
            curr = curr->next;
            count++;
        }

        // Final check: last data node must point to tail
        assert(pred->next == tail);
        // Check size consistency
        assert(count == current_size.load(std::memory_order_relaxed));

        return true;  // If all asserts pass
    }
};

}  // namespace PriorityQueue
}  // namespace DataStructure
