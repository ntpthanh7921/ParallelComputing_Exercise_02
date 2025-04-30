#pragma once

#include <cstddef>  // For size_t
#include <optional>

namespace DataStructure
{
namespace PriorityQueue
{

/**
 * @brief Interface for a Priority Queue.
 *
 * Provides basic priority queue operations. Implementations define
 * concurrency guarantees. Highest priority corresponds to the largest value.
 * Equal priority elements follow FIFO order.
 *
 * @tparam T Element type. Must support comparison via operator<.
 */
template <typename T>
class IPriorityQueue
{
public:
    virtual ~IPriorityQueue() = default;

    /**
     * @brief Inserts an element, maintaining sorted order and FIFO for equal priorities.
     * @param val The value to insert.
     * @throws std::bad_alloc if node allocation fails.
     */
    virtual void push(const T &val) = 0;

    /**
     * @brief Removes and returns the highest priority element.
     * @return An std::optional containing the highest priority element if the queue
     * is not empty, otherwise std::nullopt. Value is moved if possible.
     */
    virtual std::optional<T> pop() = 0;

    /**
     * @brief Checks if the priority queue contains no elements.
     * @return true if the queue is empty, false otherwise.
     */
    virtual bool empty() const = 0;

    /**
     * @brief Returns the number of elements in the priority queue.
     * @return The number of elements.
     */
    virtual size_t size() const = 0;

    /**
     * @brief Checks internal invariants (e.g., sorted order). Debug only.
     * @return true if invariants hold, false otherwise.
     * @note Assumes exclusive access or quiescent state. Not thread-safe
     * to call concurrently with modifications.
     */
    virtual bool check_invariants() const = 0;
};

}  // namespace PriorityQueue
}  // namespace DataStructure
