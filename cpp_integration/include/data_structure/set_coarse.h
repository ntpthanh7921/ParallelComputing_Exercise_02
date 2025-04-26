#pragma once

#include "iset.h"  // Include the common interface
#include <limits>
#include <mutex>  // For std::unique_lock, std::shared_lock
#include <new>
#include <shared_mutex>  // For std::shared_mutex
#include <stdexcept>

namespace DataStructure
{
namespace Set
{

// Node structure (can reuse SeqNode or define CoarseNode, same structure needed here)
template <typename T>
struct CoarseNode  // Or reuse SeqNode if preferred
{
    T val;
    CoarseNode<T> *next;

    CoarseNode(T v, CoarseNode<T> *n = nullptr) : val(v), next(n) { }
};

// --- Coarse-Grained Locking Implementation ---
template <typename T>
class SortedLinkedList_CoarseLock : public ISet<T>
{
private:
    CoarseNode<T> *head;
    CoarseNode<T> *tail;
    mutable std::shared_mutex list_mutex;  // Mutex protecting the entire list

    // Helper: Finds node *before* potential position of val (NO LOCKING here)
    // This helper is only called by public methods which already hold the lock.
    CoarseNode<T> *find_node(const T &val) const
    {
        CoarseNode<T> *curr = head;
        CoarseNode<T> *next = curr->next;
        while (next->val < val)
        {
            curr = next;
            next = curr->next;
        }
        return curr;
    }

public:
    // Constructor
    SortedLinkedList_CoarseLock() : head(nullptr), tail(nullptr)
    {  // list_mutex is default constructed
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
            tail = new CoarseNode<T>(max_val, nullptr);
            head = new CoarseNode<T>(min_val, tail);
        }
        catch (...)
        {
            delete head;
            delete tail;
            // Note: list_mutex does not need manual cleanup here
            throw std::runtime_error("SortedLinkedList_CoarseLock construction failed.");
        }
    }

    // Destructor
    ~SortedLinkedList_CoarseLock() override
    {
        // No lock needed - object is being destroyed, assume no concurrent access
        while (head != nullptr)
        {
            CoarseNode<T> *next_node = head->next;
            delete head;
            head = next_node;
        }
    }

    // Disable copy semantics
    SortedLinkedList_CoarseLock(const SortedLinkedList_CoarseLock &) = delete;
    SortedLinkedList_CoarseLock &operator=(const SortedLinkedList_CoarseLock &) = delete;

    // Default move semantics (Requires careful thought if mutexes are involved,
    // but default might be okay if source is guaranteed not accessed after move)
    // Consider explicitly defining if complex state exists beyond head/tail/mutex.
    SortedLinkedList_CoarseLock(SortedLinkedList_CoarseLock &&) = default;
    SortedLinkedList_CoarseLock &operator=(SortedLinkedList_CoarseLock &&) = default;

    // --- Interface Methods (with coarse-grained locking) ---

    bool contains(const T &val) const override
    {
        std::shared_lock<std::shared_mutex> lock(list_mutex);  // Read lock
        CoarseNode<T> *pred = find_node(val);
        CoarseNode<T> *curr = pred->next;
        return (curr != tail && curr->val == val);
    }

    bool add(const T &val) override
    {
        std::unique_lock<std::shared_mutex> lock(list_mutex);  // Write lock
        CoarseNode<T> *pred = find_node(val);
        CoarseNode<T> *curr = pred->next;

        if (curr != tail && curr->val == val)
        {
            return false;  // Value already exists
        }

        try
        {
            CoarseNode<T> *new_node = new CoarseNode<T>(val, curr);
            pred->next = new_node;
        }
        catch (const std::bad_alloc &e)
        {
            // Lock is released automatically by unique_lock destructor
            throw std::runtime_error("Failed node allocation in add: " + std::string(e.what()));
        }
        return true;
    }

    bool remove(const T &val) override
    {
        std::unique_lock<std::shared_mutex> lock(list_mutex);  // Write lock
        CoarseNode<T> *pred = find_node(val);
        CoarseNode<T> *curr = pred->next;

        if (curr != tail && curr->val == val)
        {
            pred->next = curr->next;
            // Lock is released automatically by unique_lock destructor
            // Deleting curr happens after lock release if needed, but here it's safe
            // as the node is unreachable. Consider epochs/GC in more complex scenarios.
            delete curr;
            return true;
        }
        return false;  // Value not found
    }
};

}  // namespace Set
}  // namespace DataStructure
