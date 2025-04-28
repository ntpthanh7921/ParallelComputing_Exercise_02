#pragma once

#include "iset.h"   // Include the common interface
#include <atomic>   // For std::atomic
#include <cstddef>  // For size_t
#include <limits>
#include <mutex>  // For std::mutex, std::unique_lock
#include <new>
#include <stdexcept>
#include <utility>  // For std::pair

namespace DataStructure
{
namespace Set
{

// Node Structure for Fine-Grained Implementation
template <typename T>
struct FineNode
{
    T val;
    FineNode<T> *next;
    std::mutex node_mutex;  // Mutex for each node

    FineNode(T v, FineNode<T> *n = nullptr) : val(v), next(n) { }

    // Default destructor ok
    // No copy/move for mutex (implicitly deleted/defaulted)

    void lock() { node_mutex.lock(); }

    void unlock() { node_mutex.unlock(); }
};

// Fine-Grained Locking Implementation
template <typename T>
class SortedLinkedList_FineLock : public ISet<T>
{
private:
    FineNode<T> *head;
    FineNode<T> *tail;
    std::atomic<size_t> current_size{0};  // Atomic counter for size

    // Helper: Locates and locks predecessor and current nodes using Hand-over-Hand.
    // Returns {pred, curr} locked. Caller must unlock.
    std::pair<FineNode<T> *, FineNode<T> *> find_and_lock_hoh(const T &val) const
    {
        FineNode<T> *pred = nullptr, *curr = nullptr;
        pred = head;
        pred->lock();
        curr = pred->next;
        curr->lock();

        while (curr != tail && curr->val < val)  // Stop if we reach tail or pass val
        {
            pred->unlock();
            pred = curr;
            // pred is already locked (it was the old curr)
            curr = curr->next;
            if (curr)
            {  // Check if curr became null (should only happen if tail is reached improperly)
                curr->lock();
            }
            else
            {
                // This case indicates a problem, potentially tail deletion or bug
                // For safety, unlock pred and maybe throw or handle error
                pred->unlock();
                // Handle error appropriately, e.g., throw exception
                throw std::runtime_error("Fine-grained search encountered null node before tail.");
            }
        }
        // Return with pred and curr locked
        return {pred, curr};
    }

    // Internal unsafe getter for head (needed for unsafe check_invariants)
    // USE WITH EXTREME CAUTION - only when list is quiescent.
    FineNode<T> *get_head_unsafe() const { return head; }

    // Internal unsafe getter for tail (needed for unsafe check_invariants)
    FineNode<T> *get_tail_unsafe() const { return tail; }

public:
    // Constructor
    SortedLinkedList_FineLock() : head(nullptr), tail(nullptr)
    {  // current_size is zero initialized
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
                throw std::logic_error(
                    "Type T requires std::numeric_limits specialization for sentinels.");
            }
            tail = new FineNode<T>(max_val, nullptr);
            head = new FineNode<T>(min_val, tail);
        }
        catch (...)
        {
            delete head;
            delete tail;
            throw std::runtime_error("SortedLinkedList_FineLock construction failed.");
        }
    }

    // Destructor
    ~SortedLinkedList_FineLock() override
    {
        // No locking needed during destruction (assume single-threaded context)
        FineNode<T> *current = head;
        while (current != nullptr)
        {
            FineNode<T> *next_node = current->next;
            delete current;
            current = next_node;
        }
    }

    // Disable copy semantics
    SortedLinkedList_FineLock(const SortedLinkedList_FineLock &) = delete;
    SortedLinkedList_FineLock &operator=(const SortedLinkedList_FineLock &) = delete;

    // Explicitly delete move semantics due to mutex members in nodes and potential complexity
    SortedLinkedList_FineLock(SortedLinkedList_FineLock &&) = delete;
    SortedLinkedList_FineLock &operator=(SortedLinkedList_FineLock &&) = delete;

    // --- ISet Interface Methods ---

    bool contains(const T &val) const override
    {
        FineNode<T> *pred = nullptr, *curr = nullptr;
        bool result = false;
        try
        {
            auto locked_pair = find_and_lock_hoh(val);
            pred = locked_pair.first;
            curr = locked_pair.second;
            result = (curr != tail && curr->val == val);
        }
        catch (...)  // Catch potential exceptions from find_and_lock_hoh
        {
            // Ensure locks are released if find_and_lock_hoh throws after locking
            if (curr)
                curr->unlock();
            if (pred)
                pred->unlock();
            throw;  // Rethrow original exception
        }
        // Unlock normally
        if (curr)
            curr->unlock();
        if (pred)
            pred->unlock();
        return result;
    }

    bool add(const T &val) override
    {
        FineNode<T> *pred = nullptr, *curr = nullptr;
        bool success = false;
        try
        {
            auto locked_pair = find_and_lock_hoh(val);
            pred = locked_pair.first;
            curr = locked_pair.second;

            if (curr != tail && curr->val == val)
            {
                success = false;  // Already exists
            }
            else
            {
                // Value doesn't exist, add it
                FineNode<T> *new_node = new FineNode<T>(val, curr);
                pred->next = new_node;
                // Update size atomically while locks are held
                current_size.fetch_add(1, std::memory_order_relaxed);
                success = true;
            }
        }
        catch (const std::bad_alloc &e)
        {
            // Exception during 'new FineNode'. Locks must be released.
            if (curr)
                curr->unlock();
            if (pred)
                pred->unlock();
            throw std::runtime_error("Failed node allocation in add: " + std::string(e.what()));
        }
        catch (...)
        {
            // Other potential exceptions from find_and_lock_hoh
            if (curr)
                curr->unlock();
            if (pred)
                pred->unlock();
            throw;
        }
        // Unlock normally
        if (curr)
            curr->unlock();
        if (pred)
            pred->unlock();
        return success;
    }

    bool remove(const T &val) override
    {
        FineNode<T> *pred = nullptr, *curr = nullptr;
        FineNode<T> *node_to_delete = nullptr;
        bool success = false;
        try
        {
            auto locked_pair = find_and_lock_hoh(val);
            pred = locked_pair.first;
            curr = locked_pair.second;

            if (curr != tail && curr->val == val)
            {
                // Found node to remove
                pred->next = curr->next;
                // Update size atomically while locks are held
                current_size.fetch_sub(1, std::memory_order_relaxed);
                node_to_delete = curr;  // Keep track to delete later
                success = true;
            }
            else
            {
                success = false;  // Not found
            }
        }
        catch (...)
        {
            // Exception during find/lock
            if (curr)
                curr->unlock();
            if (pred)
                pred->unlock();
            throw;
        }

        // Unlock nodes *before* deleting
        // Note: curr might be the node_to_delete
        if (curr)
            curr->unlock();  // Unlock current node found by search
        if (pred)
            pred->unlock();  // Unlock predecessor node

        // Delete the node after releasing locks
        // Note: Potential ABA/memory reclamation issues in robust systems. Simple delete here.
        delete node_to_delete;

        return success;
    }

    // Returns the number of elements (thread-safe read).
    size_t size() const override
    {
        // Relaxed memory order is sufficient for reading the counter
        return current_size.load(std::memory_order_relaxed);
    }

    // Checks invariants.
    // WARNING: NOT THREAD-SAFE if called concurrently
    // with add/remove. Assumes list is quiescent (e.g., called after test threads join).
    bool check_invariants() const override
    {
        // This check traverses without locks. It is inherently unsafe
        // if the list is being modified concurrently. Use only for post-test validation.
        FineNode<T> *h = get_head_unsafe();  // Use unsafe getter
        FineNode<T> *t = get_tail_unsafe();  // Use unsafe getter
        FineNode<T> *pred = h;
        FineNode<T> *curr = h->next;

        if (!h || !t || h->next == nullptr)
            return false;  // Basic structure check

        while (curr != t)
        {
            if (!curr)
                return false;  // Should not encounter null before tail
            if (pred->val > curr->val)
                return false;  // Check sorted order
            pred = curr;
            curr = curr->next;
        }
        // Final check: last non-tail node should point to tail
        return (pred->next == t);
    }
};

}  // namespace Set
}  // namespace DataStructure
