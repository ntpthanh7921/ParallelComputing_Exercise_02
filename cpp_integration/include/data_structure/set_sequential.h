#pragma once

#include "iset.h"
#include <cstddef>  // For size_t
#include <limits>
#include <new>        // For std::bad_alloc
#include <stdexcept>  // For exceptions

namespace DataStructure
{
namespace Set
{

// Node Structure for Sequential Implementation
template <typename T>
struct SeqNode
{
    T val;
    SeqNode<T> *next;

    SeqNode(T v, SeqNode<T> *n = nullptr) : val(v), next(n) { }
};

// Sequential Sorted Linked List Implementation
template <typename T>
class SortedLinkedList_Sequential : public ISet<T>
{
private:
    SeqNode<T> *head;
    SeqNode<T> *tail;

    // Helper: Finds node *before* potential position of val
    SeqNode<T> *find_node(const T &val) const
    {
        SeqNode<T> *pred = head;
        SeqNode<T> *curr = head->next;
        while (curr != tail && curr->val < val)  // Stop if we reach tail or pass val
        {
            pred = curr;
            curr = curr->next;
        }
        return pred;
    }

public:
    // Constructor: Initializes sentinels
    SortedLinkedList_Sequential() : head(nullptr), tail(nullptr)
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
            tail = new SeqNode<T>(max_val, nullptr);
            head = new SeqNode<T>(min_val, tail);
        }
        catch (...)
        {
            delete head;
            delete tail;
            throw std::runtime_error("SortedLinkedList_Sequential construction failed.");
        }
    }

    // Destructor: Cleans up all nodes
    ~SortedLinkedList_Sequential() override
    {
        SeqNode<T> *current = head;
        while (current != nullptr)
        {
            SeqNode<T> *next_node = current->next;
            delete current;
            current = next_node;
        }
    }

    // Disable copy semantics
    SortedLinkedList_Sequential(const SortedLinkedList_Sequential &) = delete;
    SortedLinkedList_Sequential &operator=(const SortedLinkedList_Sequential &) = delete;

    // Default move semantics
    SortedLinkedList_Sequential(SortedLinkedList_Sequential &&) = default;
    SortedLinkedList_Sequential &operator=(SortedLinkedList_Sequential &&) = default;

    // --- ISet Interface Methods ---

    bool contains(const T &val) const override
    {
        SeqNode<T> *pred = find_node(val);
        SeqNode<T> *curr = pred->next;
        return (curr != tail && curr->val == val);
    }

    bool add(const T &val) override
    {
        SeqNode<T> *pred = find_node(val);
        SeqNode<T> *curr = pred->next;

        if (curr != tail && curr->val == val)
        {
            return false;  // Value already exists
        }

        try
        {
            SeqNode<T> *new_node = new SeqNode<T>(val, curr);
            pred->next = new_node;
        }
        catch (const std::bad_alloc &e)
        {
            throw std::runtime_error("Failed node allocation in add: " + std::string(e.what()));
        }
        return true;
    }

    bool remove(const T &val) override
    {
        SeqNode<T> *pred = find_node(val);
        SeqNode<T> *curr = pred->next;

        if (curr != tail && curr->val == val)
        {
            pred->next = curr->next;
            delete curr;  // Deallocate the removed node
            return true;
        }
        return false;  // Value not found
    }

    // Returns the number of elements (excluding sentinels).
    size_t size() const override
    {
        size_t count = 0;
        SeqNode<T> *current = head->next;  // Start after head sentinel
        while (current != tail)            // Stop before tail sentinel
        {
            count++;
            current = current->next;
        }
        return count;
    }

    // Checks if the list is sorted and pointers are consistent.
    bool check_invariants() const override
    {
        SeqNode<T> *pred = head;
        SeqNode<T> *curr = head->next;

        if (!head || !tail || head->next == nullptr)
            return false;  // Basic structure check

        while (curr != tail)
        {
            if (!curr)
                return false;  // Should not encounter null before tail
            if (pred->val > curr->val)
                return false;  // Check sorted order
            pred = curr;
            curr = curr->next;
        }
        // Final check: last non-tail node should point to tail
        return (pred->next == tail);
    }
};

}  // namespace Set
}  // namespace DataStructure
