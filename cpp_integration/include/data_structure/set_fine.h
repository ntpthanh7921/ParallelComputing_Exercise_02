#pragma once

#include "iset.h"  // Include the common interface
#include <limits>
#include <mutex>  // For std::mutex, std::unique_lock
#include <new>
#include <stdexcept>
#include <utility>  // For std::pair

namespace DataStructure
{
namespace Set
{

// --- Node Structure for Fine-Grained Implementation ---
template <typename T>
struct FineNode
{
    T val;
    FineNode<T> *next;
    std::mutex node_mutex;  // Mutex for each node

    FineNode(T v, FineNode<T> *n = nullptr) : val(v), next(n) { }

    // No copy/move for mutex, implicitly deleted/defaulted ok for now
    // ~FineNode() = default; // Default destructor ok

    void lock() { node_mutex.lock(); }

    void unlock() { node_mutex.unlock(); }
};

// --- Fine-Grained Locking Implementation ---
template <typename T>
class SortedLinkedList_FineLock : public ISet<T>
{
private:
    FineNode<T> *head;
    FineNode<T> *tail;

    // Helper: Locates and locks predecessor and current nodes using Hand-over-Hand.
    // Returns a pair of locked nodes {pred, curr}.
    // The caller is responsible for unlocking pred and curr.
    std::pair<FineNode<T> *, FineNode<T> *> find_and_lock_hoh(const T &val) const
    {
        FineNode<T> *pred = nullptr, *curr = nullptr;
        pred = head;
        pred->lock();
        curr = pred->next;
        curr->lock();

        while (curr->val < val)
        {
            pred->unlock();  // Unlock previous predecessor
            pred = curr;
            curr = curr->next;
            curr->lock();  // Lock new current node
        }
        // Return with pred and curr locked
        return {pred, curr};
    }

public:
    // Constructor
    SortedLinkedList_FineLock() : head(nullptr), tail(nullptr)
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
                throw std::logic_error(
                    "Type T requires std::numeric_limits specialization for sentinels.");
            }
            // Note: Node mutexes inside FineNode are default-constructed by FineNode constructor
            tail = new FineNode<T>(max_val, nullptr);
            head = new FineNode<T>(min_val, tail);
        }
        catch (...)
        {
            delete head;
            delete tail;
            // Note: Node mutexes are managed by FineNode destructors during cleanup
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

    // Default move semantics might be problematic with mutex members in nodes
    // Prefer explicit definition or deletion if moves are needed.
    SortedLinkedList_FineLock(SortedLinkedList_FineLock &&) = delete;
    SortedLinkedList_FineLock &operator=(SortedLinkedList_FineLock &&) = delete;

    // --- Interface Methods (with fine-grained locking) ---

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
        catch (...)
        {
            // Ensure locks are released if find_and_lock_hoh throws after locking
            if (curr)
                curr->unlock();
            if (pred)
                pred->unlock();
            throw;  // Rethrow original exception
        }
        // Unlock normally
        curr->unlock();
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
            // Other potential exceptions
            if (curr)
                curr->unlock();
            if (pred)
                pred->unlock();
            throw;
        }
        // Unlock normally
        curr->unlock();
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
        curr->unlock();
        pred->unlock();

        // Delete the node after releasing locks
        // Note: Potential ABA/memory reclamation issues in real systems,
        // might need epochs, hazard pointers, or GC. Simple delete here.
        delete node_to_delete;

        return success;
    }
};

}  // namespace Set
}  // namespace DataStructure
