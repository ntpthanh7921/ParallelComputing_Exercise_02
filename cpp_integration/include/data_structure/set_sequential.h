#pragma once

#include "iset.h"
#include <limits>
#include <new>        // For std::bad_alloc
#include <stdexcept>  // For exceptions

namespace DataStructure
{
namespace Set
{

// --- Node Structure for Sequential Implementation ---
template <typename T>
struct SeqNode
{
    T val;
    SeqNode<T> *next;

    // Basic constructor
    SeqNode(T v, SeqNode<T> *n = nullptr) : val(v), next(n) { }
};

// --- Sequential Sorted Linked List Implementation ---
template <typename T>
class SortedLinkedList_Sequential : public ISet<T>
{
private:
    SeqNode<T> *head;
    SeqNode<T> *tail;

    // Helper: Finds node *before* potential position of val
    SeqNode<T> *find_node(const T &val) const
    {
        SeqNode<T> *curr = head;
        SeqNode<T> *next = curr->next;
        // Check next != nullptr is implicit if tail sentinel is always present
        while (next->val < val)
        {
            curr = next;
            next = curr->next;
        }
        return curr;
    }

public:
    // Constructor: Initializes sentinels using T's static methods
    SortedLinkedList_Sequential() : head(nullptr), tail(nullptr)
    {
        try
        {
            T min_val, max_val;
            // Check if numeric_limits is available for type T
            if constexpr (std::numeric_limits<T>::is_specialized)
            {
                min_val = std::numeric_limits<T>::lowest();  // Use lowest for min sentinel
                max_val = std::numeric_limits<T>::max();     // Use max for max sentinel
            }
            else
            {
                // Handle types without numeric_limits if necessary, e.g., throw
                throw std::logic_error(
                    "Type T requires std::numeric_limits specialization for sentinels.");
            }
            tail = new SeqNode<T>(max_val, nullptr);
            head = new SeqNode<T>(min_val, tail);
        }
        catch (...)
        {
            delete head;  // Safe if nullptr
            delete tail;  // Safe if nullptr
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

    // Interface methods implementation
    bool contains(const T &val) const override
    {
        SeqNode<T> *pred = find_node(val);
        SeqNode<T> *curr = pred->next;
        // Check if the node found is the target value and not the tail sentinel
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
            throw std::runtime_error("Failed to allocate node for add operation: "
                                     + std::string(e.what()));
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
};

}  // namespace Set
}  // namespace DataStructure
