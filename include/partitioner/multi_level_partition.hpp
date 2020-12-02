#ifndef OSRM_PARTITIONER_MULTI_LEVEL_PARTITION_HPP
#define OSRM_PARTITIONER_MULTI_LEVEL_PARTITION_HPP

#include "util/exception.hpp"
#include "util/for_each_pair.hpp"
#include "util/msb.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"

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
namespace partitioner
{
namespace detail
{
template <storage::Ownership Ownership> class MultiLevelPartitionImpl;
}
using MultiLevelPartition = detail::MultiLevelPartitionImpl<storage::Ownership::Container>;
using MultiLevelPartitionView = detail::MultiLevelPartitionImpl<storage::Ownership::View>;

namespace serialization
{
template <storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          detail::MultiLevelPartitionImpl<Ownership> &mlp);
template <storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const detail::MultiLevelPartitionImpl<Ownership> &mlp);
} // namespace serialization

namespace detail
{

template <storage::Ownership Ownership> class MultiLevelPartitionImpl final
{
    // we will support at most 16 levels
    static const constexpr std::uint8_t MAX_NUM_LEVEL = 16;
    static const constexpr std::uint8_t NUM_PARTITION_BITS = sizeof(PartitionID) * CHAR_BIT;

    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

  public:
    // Contains all data necessary to describe the level hierarchy
    struct LevelData
    {

        std::uint32_t num_level;
        std::array<std::uint8_t, MAX_NUM_LEVEL - 1> lidx_to_offset;
        std::array<PartitionID, MAX_NUM_LEVEL - 1> lidx_to_mask;
        std::array<LevelID, NUM_PARTITION_BITS> bit_to_level;
        std::array<std::uint32_t, MAX_NUM_LEVEL - 1> lidx_to_children_offsets;
    };
    using LevelDataPtr = typename std::conditional<Ownership == storage::Ownership::View,
                                                   LevelData *,
                                                   std::unique_ptr<LevelData>>::type;

    MultiLevelPartitionImpl();

    // cell_sizes is index by level (starting at 0, the base graph).
    // However level 0 always needs to have cell size 1, since it is the
    // basegraph.
    template <typename = typename std::enable_if<Ownership == storage::Ownership::Container>>
    MultiLevelPartitionImpl(const std::vector<std::vector<CellID>> &partitions,
                            const std::vector<std::uint32_t> &lidx_to_num_cells)
        : level_data(MakeLevelData(lidx_to_num_cells))
    {
        InitializePartitionIDs(partitions);
    }

    template <typename = typename std::enable_if<Ownership == storage::Ownership::View>>
    MultiLevelPartitionImpl(LevelDataPtr level_data,
                            Vector<PartitionID> partition_,
                            Vector<CellID> cell_to_children_)
        : level_data(std::move(level_data)), partition(std::move(partition_)),
          cell_to_children(std::move(cell_to_children_))
    {
    }

    // returns the index of the cell the vertex is contained at level l
    CellID GetCell(LevelID l, NodeID node) const
    {
        auto p = partition[node];
        auto lidx = LevelIDToIndex(l);
        auto masked = p & level_data->lidx_to_mask[lidx];
        return masked >> level_data->lidx_to_offset[lidx];
    }

    LevelID GetQueryLevel(NodeID start, NodeID target, NodeID node) const
    {
        return std::min(GetHighestDifferentLevel(start, node),
                        GetHighestDifferentLevel(target, node));
    }

    LevelID GetHighestDifferentLevel(NodeID first, NodeID second) const
    {
        if (partition[first] == partition[second])
            return 0;

        auto msb = util::msb(partition[first] ^ partition[second]);
        return level_data->bit_to_level[msb];
    }

    std::uint8_t GetNumberOfLevels() const { return level_data->num_level; }

    std::uint32_t GetNumberOfCells(LevelID level) const
    {
        return GetCell(level, GetSentinelNode());
    }

    // Returns the lowest cell id (at `level - 1`) of all children `cell` at `level`
    CellID BeginChildren(LevelID level, CellID cell) const
    {
        BOOST_ASSERT(level > 1);
        auto lidx = LevelIDToIndex(level);
        auto offset = level_data->lidx_to_children_offsets[lidx];
        return cell_to_children[offset + cell];
    }

    // Returns the highest cell id (at `level - 1`) of all children `cell` at `level`
    CellID EndChildren(LevelID level, CellID cell) const
    {
        BOOST_ASSERT(level > 1);
        auto lidx = LevelIDToIndex(level);
        auto offset = level_data->lidx_to_children_offsets[lidx];
        return cell_to_children[offset + cell + 1];
    }

    friend void serialization::read<Ownership>(storage::tar::FileReader &reader,
                                               const std::string &name,
                                               MultiLevelPartitionImpl &mlp);
    friend void serialization::write<Ownership>(storage::tar::FileWriter &writer,
                                                const std::string &name,
                                                const MultiLevelPartitionImpl &mlp);

  private:
    auto MakeLevelData(const std::vector<std::uint32_t> &lidx_to_num_cells)
    {
        std::uint32_t num_level = lidx_to_num_cells.size() + 1;
        auto offsets = MakeLevelOffsets(lidx_to_num_cells);
        auto masks = MakeLevelMasks(offsets, num_level);
        auto bits = MakeBitToLevel(offsets, num_level);
        return std::make_unique<LevelData>(LevelData{num_level, offsets, masks, bits, {{0}}});
    }

    inline std::size_t LevelIDToIndex(LevelID l) const { return l - 1; }

    // We save the sentinel as last node in the partition information.
    // It has the highest cell id in each level so we can derived the range
    // of cell ids efficiently.
    inline NodeID GetSentinelNode() const { return partition.size() - 1; }

    void SetCellID(LevelID l, NodeID node, CellID cell_id)
    {
        auto lidx = LevelIDToIndex(l);

        auto shifted_id = static_cast<std::uint64_t>(cell_id) << level_data->lidx_to_offset[lidx];
        auto cleared_cell = partition[node] & ~level_data->lidx_to_mask[lidx];
        partition[node] = cleared_cell | shifted_id;
    }

    // If we have N cells per level we need log_2 bits for every cell ID
    auto MakeLevelOffsets(const std::vector<std::uint32_t> &lidx_to_num_cells) const
    {
        std::array<std::uint8_t, MAX_NUM_LEVEL - 1> offsets;

        auto lidx = 0UL;
        auto sum_bits = 0;
        for (auto num_cells : lidx_to_num_cells)
        {
            // bits needed to number all contained vertexes
            auto bits = static_cast<std::uint64_t>(std::ceil(std::log2(num_cells + 1)));
            offsets[lidx++] = sum_bits;
            sum_bits += bits;
            if (sum_bits > 64)
            {
                throw util::exception(
                    "Can't pack the partition information at level " + std::to_string(lidx) +
                    " into a 64bit integer. Would require " + std::to_string(sum_bits) + " bits.");
            }
        }
        // sentinel
        offsets[lidx++] = sum_bits;
        BOOST_ASSERT(lidx < MAX_NUM_LEVEL);

        return offsets;
    }

    auto MakeLevelMasks(const std::array<std::uint8_t, MAX_NUM_LEVEL - 1> &level_offsets,
                        std::uint32_t num_level) const
    {
        std::array<PartitionID, MAX_NUM_LEVEL - 1> masks;

        auto lidx = 0UL;
        util::for_each_pair(level_offsets.begin(),
                            level_offsets.begin() + num_level,
                            [&](const auto offset, const auto next_offset) {
                                // create mask that has `bits` ones at its LSBs.
                                // 000011
                                BOOST_ASSERT(offset < NUM_PARTITION_BITS);
                                PartitionID mask = (1ULL << offset) - 1ULL;
                                // 001111
                                BOOST_ASSERT(next_offset < NUM_PARTITION_BITS);
                                PartitionID next_mask = (1ULL << next_offset) - 1ULL;
                                // 001100
                                masks[lidx++] = next_mask ^ mask;
                            });

        return masks;
    }

    auto MakeBitToLevel(const std::array<std::uint8_t, MAX_NUM_LEVEL - 1> &level_offsets,
                        std::uint32_t num_level) const
    {
        std::array<LevelID, NUM_PARTITION_BITS> bit_to_level;

        for (auto l = 1u; l < num_level; ++l)
        {
            auto bits = level_offsets[l - 1];
            // set all bits to point to the correct level.
            for (auto idx = bits; idx < NUM_PARTITION_BITS; ++idx)
            {
                bit_to_level[idx] = l;
            }
        }

        return bit_to_level;
    }

    void InitializePartitionIDs(const std::vector<std::vector<CellID>> &partitions)
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

        level_data->lidx_to_children_offsets[0] = 0;

        for (auto level_idx = 0UL; level_idx < partitions.size() - 1; ++level_idx)
        {
            const auto &parent_partition = partitions[level_idx + 1];

            level_data->lidx_to_children_offsets[level_idx + 1] = cell_to_children.size();

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

    LevelDataPtr level_data = {};
    Vector<PartitionID> partition;
    Vector<CellID> cell_to_children;
};

template <>
inline MultiLevelPartitionImpl<storage::Ownership::Container>::MultiLevelPartitionImpl()
    : level_data(std::make_unique<LevelData>())
{
}

template <>
inline MultiLevelPartitionImpl<storage::Ownership::View>::MultiLevelPartitionImpl()
    : level_data(nullptr)
{
}
} // namespace detail
} // namespace partitioner
} // namespace osrm

#endif
