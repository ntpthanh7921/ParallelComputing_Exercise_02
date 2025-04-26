#pragma once

namespace DataStructure
{
namespace Set
{

// --- Common Interface ---
template <typename T>
class ISet
{
public:
    virtual ~ISet() = default;  // Virtual destructor is crucial for interfaces
    virtual bool add(const T &val) = 0;
    virtual bool remove(const T &val) = 0;
    virtual bool contains(const T &val) const = 0;
};

}  // namespace Set
}  // namespace DataStructure
