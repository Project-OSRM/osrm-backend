#ifndef OSRM_UTIL_MULTI_LEVEL_PARTITION_HPP
#define OSRM_UTIL_MULTI_LEVEL_PARTITION_HPP

#include "util/typedefs.hpp"
#include <cstdint>

namespace osrm
{
namespace util
{

using LevelID = std::uint8_t;
using CellID = std::uint32_t;

// Mock interface, can be removed when we have an actual implementation
class MultiLevelPartition
{
  public:
    // Returns the cell id of `node` at `level`
    virtual CellID GetCell(LevelID level, NodeID node) const = 0;

    // Returns the highest level in which `first` and `second` are still in different cells
    virtual LevelID GetHighestDifferentLevel(NodeID first, NodeID second) const = 0;

    virtual std::size_t GetNumberOfLevels() const = 0;

    virtual std::size_t GetNumberOfCells(LevelID level) const = 0;
};
}
}

#endif
