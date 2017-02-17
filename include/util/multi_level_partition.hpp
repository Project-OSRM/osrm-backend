#ifndef OSRM_UTIL_MULTI_LEVEL_PARTITION_HPP
#define OSRM_UTIL_MULTI_LEVEL_PARTITION_HPP

#include "util/exception.hpp"
#include "util/for_each_pair.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

#include <boost/range/adaptor/reversed.hpp>

namespace osrm
{
namespace util
{
namespace detail
{
// get the msb of an integer
// return 0 for integers without msb
template <typename T> std::size_t highestMSB(T value)
{
    static_assert(std::is_integral<T>::value, "Integer required.");
    std::size_t msb = 0;
    while (value > 0)
    {
        value >>= 1u;
        msb++;
    }
    return msb;
}

#if (defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)) && __x86_64__
inline std::size_t highestMSB(std::uint64_t v)
{
    BOOST_ASSERT(v > 0);
    return 63UL - __builtin_clzl(v);
}
#endif
}

using LevelID = std::uint8_t;
using CellID = std::uint32_t;

static constexpr CellID INVALID_CELL_ID = std::numeric_limits<CellID>::max();

// Mock interface, can be removed when we have an actual implementation
class MultiLevelPartition
{
  public:
    // Returns the cell id of `node` at `level`
    virtual CellID GetCell(LevelID level, NodeID node) const = 0;

    // Returns the lowest cell id (at `level - 1`) of all children `cell` at `level`
    virtual CellID BeginChildren(LevelID level, CellID cell) const = 0;

    // Returns the highest cell id (at `level - 1`) of all children `cell` at `level`
    virtual CellID EndChildren(LevelID level, CellID cell) const = 0;

    // Returns the highest level in which `first` and `second` are still in different cells
    virtual LevelID GetHighestDifferentLevel(NodeID first, NodeID second) const = 0;

    // Returns the level at which a `node` is relevant for a query from start to target
    virtual LevelID GetQueryLevel(NodeID start, NodeID target, NodeID node) const = 0;

    virtual std::uint8_t GetNumberOfLevels() const = 0;

    virtual std::uint32_t GetNumberOfCells(LevelID level) const = 0;
};

class PackedMultiLevelPartition final : public MultiLevelPartition
{
    using PartitionID = std::uint64_t;
    static const constexpr std::uint8_t NUM_PARTITION_BITS = sizeof(PartitionID) * CHAR_BIT;

  public:
    // cell_sizes is index by level (starting at 0, the base graph).
    // However level 0 always needs to have cell size 1, since it is the
    // basegraph.
    PackedMultiLevelPartition(const std::vector<std::vector<CellID>> &partitions,
                              const std::vector<std::uint32_t> &level_to_num_cells)
        : level_offsets(makeLevelOffsets(level_to_num_cells)), level_masks(makeLevelMasks()),
          bit_to_level(makeBitToLevel())
    {
        initializePartitionIDs(partitions);
    }

    // returns the index of the cell the vertex is contained at level l
    CellID GetCell(LevelID l, NodeID node) const final override
    {
        auto p = partition[node];
        auto lidx = LevelIDToIndex(l);
        auto masked = p & level_masks[lidx];
        return masked >> level_offsets[lidx];
    }

    LevelID GetQueryLevel(NodeID start, NodeID target, NodeID node) const final override
    {
        return std::min(GetHighestDifferentLevel(start, node),
                        GetHighestDifferentLevel(target, node));
    }

    LevelID GetHighestDifferentLevel(NodeID first, NodeID second) const final override
    {
        if (partition[first] == partition[second])
            return 0;

        auto msb = detail::highestMSB(partition[first] ^ partition[second]);
        return bit_to_level[msb];
    }

    std::uint8_t GetNumberOfLevels() const final override { return level_offsets.size(); }

    std::uint32_t GetNumberOfCells(LevelID level) const final override
    {
        return GetCell(level, GetSenitileNode());
    }

    // Returns the lowest cell id (at `level - 1`) of all children `cell` at `level`
    CellID BeginChildren(LevelID level, CellID cell) const final override
    {
        BOOST_ASSERT(level > 1);
        auto lidx = LevelIDToIndex(level);
        auto offset = level_to_children_offset[lidx];
        return cell_to_children[offset + cell];
    }

    // Returns the highest cell id (at `level - 1`) of all children `cell` at `level`
    CellID EndChildren(LevelID level, CellID cell) const final override
    {
        BOOST_ASSERT(level > 1);
        auto lidx = LevelIDToIndex(level);
        auto offset = level_to_children_offset[lidx];
        return cell_to_children[offset + cell + 1];
    }

  private:
    inline std::size_t LevelIDToIndex(LevelID l) const { return l - 1; }

    // We save the sentinel as last node in the partition information.
    // It has the highest cell id in each level so we can derived the range
    // of cell ids efficiently.
    inline NodeID GetSenitileNode() const { return partition.size() - 1; }

    void SetCellID(LevelID l, NodeID node, std::size_t cell_id)
    {
        auto lidx = LevelIDToIndex(l);

        auto shifted_id = cell_id << level_offsets[lidx];
        auto cleared_cell = partition[node] & ~level_masks[lidx];
        partition[node] = cleared_cell | shifted_id;
    }

    // If we have N cells per level we need log_2 bits for every cell ID
    std::vector<std::uint8_t>
    makeLevelOffsets(const std::vector<std::uint32_t> &level_to_num_cells) const
    {
        std::vector<std::uint8_t> offsets;
        offsets.reserve(level_to_num_cells.size());

        auto sum_bits = 0;
        for (auto num_cells : level_to_num_cells)
        {
            // bits needed to number all contained vertexes
            auto bits = static_cast<std::uint64_t>(std::ceil(std::log2(num_cells + 1)));
            offsets.push_back(sum_bits);
            sum_bits += bits;
            if (sum_bits > 64)
            {
                throw util::exception("Can't pack the partition information at level " +
                                      std::to_string(offsets.size()) +
                                      " into a 64bit integer. Would require " +
                                      std::to_string(sum_bits) + " bits.");
            }
        }
        // sentinel
        offsets.push_back(sum_bits);

        return offsets;
    }

    std::vector<PartitionID> makeLevelMasks() const
    {
        std::vector<PartitionID> masks;
        masks.reserve(level_offsets.size());

        util::for_each_pair(level_offsets.begin(),
                            level_offsets.end(),
                            [&](const auto offset, const auto next_offset) {
                                // create mask that has `bits` ones at its LSBs.
                                // 000011
                                BOOST_ASSERT(offset < NUM_PARTITION_BITS);
                                PartitionID mask = (1UL << offset) - 1UL;
                                // 001111
                                BOOST_ASSERT(next_offset < NUM_PARTITION_BITS);
                                PartitionID next_mask = (1UL << next_offset) - 1UL;
                                // 001100
                                masks.push_back(next_mask ^ mask);
                            });

        return masks;
    }

    std::array<LevelID, NUM_PARTITION_BITS> makeBitToLevel() const
    {
        std::array<LevelID, NUM_PARTITION_BITS> bit_to_level;

        LevelID l = 1;
        for (auto bits : level_offsets)
        {
            // set all bits to point to the correct level.
            for (auto idx = bits; idx < NUM_PARTITION_BITS; ++idx)
            {
                bit_to_level[idx] = l;
            }
            l++;
        }

        return bit_to_level;
    }

    void initializePartitionIDs(const std::vector<std::vector<CellID>> &partitions)
    {
        auto num_nodes = partitions.front().size();
        std::vector<NodeID> permutation(num_nodes);
        std::iota(permutation.begin(), permutation.end(), 0);
        // We include a sentinel element at the end of the partition
        partition.resize(num_nodes + 1, 0);
        NodeID sentinel = num_nodes;

        // Sort nodes bottum-up by cell id.
        // This ensures that we get a nice grouping from parent to child cells:
        //
        // intitial:
        // level 0: 0 1 2 3 4 5
        // level 1: 2 1 3 4 3 4
        // level 2: 2 2 0 1 0 1
        //
        // first round:
        // level 0: 1 0 2 4 3 5
        // level 1: 1 2 3 3 4 4 (< sorted)
        // level 2: 2 2 0 0 1 1
        //
        // second round:
        // level 0: 2 4 3 5 1 0
        // level 1: 3 3 4 4 1 2
        // level 2: 0 0 1 1 2 2 (< sorted)
        for (const auto &partition : partitions)
        {
            std::stable_sort(permutation.begin(),
                             permutation.end(),
                             [&partition](const auto lhs, const auto rhs) {
                                 return partition[lhs] < partition[rhs];
                             });
        }

        // top down assign new cell ids
        LevelID level = partitions.size();
        for (const auto &partition : boost::adaptors::reverse(partitions))
        {
            BOOST_ASSERT(permutation.size() > 0);
            CellID last_cell_id = partition[permutation.front()];
            CellID cell_id = 0;
            for (const auto node : permutation)
            {
                if (last_cell_id != partition[node])
                {
                    cell_id++;
                    last_cell_id = partition[node];
                }
                SetCellID(level, node, cell_id);
            }
            // Store the number of cells of the level in the sentinel
            SetCellID(level, sentinel, cell_id + 1);
            level--;
        }

        // level 1 does not have child cells
        level_to_children_offset.push_back(0);

        for (auto level_idx = 0UL; level_idx < partitions.size() - 1; ++level_idx)
        {
            const auto &parent_partition = partitions[level_idx + 1];

            level_to_children_offset.push_back(cell_to_children.size());

            CellID last_parent_id = parent_partition[permutation.front()];
            cell_to_children.push_back(GetCell(level_idx + 1, permutation.front()));
            for (const auto node : permutation)
            {
                if (last_parent_id != parent_partition[node])
                {
                    // Note: we use the new cell id here, not the ones contained
                    // in the input partition
                    cell_to_children.push_back(GetCell(level_idx + 1, node));
                    last_parent_id = parent_partition[node];
                }
            }
            // insert sentinel for the last cell
            cell_to_children.push_back(GetCell(level_idx + 1, permutation.back()) + 1);
        }
    }

    std::vector<PartitionID> partition;
    std::vector<std::uint8_t> level_offsets;
    std::vector<PartitionID> level_masks;
    std::vector<std::uint32_t> level_to_children_offset;
    std::vector<CellID> cell_to_children;
    std::array<LevelID, NUM_PARTITION_BITS> bit_to_level;
};
}
}

#endif
