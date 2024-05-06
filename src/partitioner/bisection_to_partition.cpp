#include "partitioner/bisection_to_partition.hpp"

namespace osrm::partitioner
{

namespace
{
struct CellBisection
{
    std::uint32_t begin;
    std::uint32_t end;
    std::uint8_t bit;
    bool tabu; // we will not attempt to split this cell anymore
};
static constexpr std::size_t NUM_BISECTION_BITS = sizeof(BisectionID) * CHAR_BIT;

std::vector<std::uint32_t> getLargeCells(const std::size_t max_cell_size,
                                         const std::vector<CellBisection> &cells)
{
    std::vector<std::uint32_t> large_cells;

    for (auto index = 0u; index < cells.size(); ++index)
    {
        if (!cells[index].tabu && cells[index].end - cells[index].begin > max_cell_size)
            large_cells.push_back(index);
    }

    return large_cells;
}

Partition cellsToPartition(const std::vector<CellBisection> &cells,
                           const std::vector<std::uint32_t> &permutation)
{
    Partition partition(permutation.size(), INVALID_CELL_ID);
    CellID cell_id = 0;
    for (const auto &cell : cells)
    {
        std::for_each(permutation.begin() + cell.begin,
                      permutation.begin() + cell.end,
                      [&partition, cell_id](const auto node_id) { partition[node_id] = cell_id; });
        cell_id++;
    }
    BOOST_ASSERT(std::find(partition.begin(), partition.end(), INVALID_CELL_ID) == partition.end());

    return partition;
}

void partitionLevel(const std::vector<BisectionID> &node_to_bisection_id,
                    std::size_t max_cell_size,
                    std::vector<std::uint32_t> &permutation,
                    std::vector<CellBisection> &cells)
{
    for (auto large_cells = getLargeCells(max_cell_size, cells); large_cells.size() > 0;
         large_cells = getLargeCells(max_cell_size, cells))
    {
        for (const auto cell_index : large_cells)
        {
            auto &cell = cells[cell_index];
            BOOST_ASSERT(cell.bit < NUM_BISECTION_BITS);

            // Go over all nodes and sum up the bits to determine at which position the first one
            // bit is
            BisectionID sum =
                std::accumulate(permutation.begin() + cell.begin,
                                permutation.begin() + cell.end,
                                BisectionID{0},
                                [&node_to_bisection_id](const BisectionID lhs, const NodeID rhs)
                                { return lhs | node_to_bisection_id[rhs]; });
            // masks all bit strictly higher then cell.bit
            BOOST_ASSERT(sizeof(unsigned long long) * CHAR_BIT > sizeof(BisectionID) * CHAR_BIT);
            const BisectionID mask = (1ULL << (cell.bit + 1)) - 1;
            BOOST_ASSERT(mask == 0 || util::msb(mask) == cell.bit);
            const auto masked_sum = sum & mask;
            // we can't split the cell anymore, but it also doesn't conform to the max size
            // constraint
            // -> we need to remove it from the optimization
            if (masked_sum == 0)
            {
                cell.tabu = true;
                continue;
            }
            const auto bit = util::msb(masked_sum);
            // determines if an bisection ID is on the left side of the partition
            const BisectionID is_left_mask = 1ULL << bit;
            BOOST_ASSERT(util::msb(is_left_mask) == bit);

            std::uint32_t middle =
                std::partition(permutation.begin() + cell.begin,
                               permutation.begin() + cell.end,
                               [is_left_mask, &node_to_bisection_id](const auto node_id)
                               { return node_to_bisection_id[node_id] & is_left_mask; }) -
                permutation.begin();

            if (bit > 0)
                cell.bit = bit - 1;
            else
                cell.tabu = true;
            if (middle != cell.begin && middle != cell.end)
            {
                auto old_end = cell.end;
                cell.end = middle;
                cells.push_back(
                    CellBisection{middle, old_end, static_cast<std::uint8_t>(cell.bit), cell.tabu});
            }
        }
    }
}
} // namespace

// Implements a greedy algorithm that split cells using the bisection until a target cell size is
// reached
std::tuple<std::vector<Partition>, std::vector<std::uint32_t>>
bisectionToPartition(const std::vector<BisectionID> &node_to_bisection_id,
                     const std::vector<std::size_t> &max_cell_sizes)
{
    std::vector<std::uint32_t> permutation(node_to_bisection_id.size());
    std::iota(permutation.begin(), permutation.end(), 0);

    std::vector<CellBisection> cells;
    cells.push_back(CellBisection{
        0, static_cast<std::uint32_t>(node_to_bisection_id.size()), NUM_BISECTION_BITS - 1, false});

    std::vector<Partition> partitions(max_cell_sizes.size());
    std::vector<std::uint32_t> num_cells(max_cell_sizes.size());

    int level_idx = max_cell_sizes.size() - 1;
    for (auto max_cell_size : boost::adaptors::reverse(max_cell_sizes))
    {
        BOOST_ASSERT(level_idx >= 0);
        partitionLevel(node_to_bisection_id, max_cell_size, permutation, cells);

        partitions[level_idx] = cellsToPartition(cells, permutation);
        num_cells[level_idx] = cells.size();
        level_idx--;
    }

    return std::make_tuple(std::move(partitions), std::move(num_cells));
}
} // namespace osrm::partitioner
