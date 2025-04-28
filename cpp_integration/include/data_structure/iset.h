#pragma once

#include <cstddef>  // Include for size_t

namespace DataStructure
{
namespace Set
{

// Common Interface for Set Data Structures
template <typename T>
class ISet
{
public:
    // Virtual destructor is crucial for interfaces
    virtual ~ISet() = default;

    // Adds a value to the set. Returns true if added, false if already present.
    virtual bool add(const T &val) = 0;

    // Removes a value from the set. Returns true if removed, false if not found.
    virtual bool remove(const T &val) = 0;

    // Checks if a value is present in the set.
    virtual bool contains(const T &val) const = 0;

    // Returns the number of elements currently in the set (excluding sentinels).
    virtual size_t size() const = 0;

    // Checks internal invariants (e.g., sorted order). Primarily for testing.
    // Note: Implementations must define thread-safety guarantees for this method.
    virtual bool check_invariants() const = 0;
};

}  // namespace Set
}  // namespace DataStructure
