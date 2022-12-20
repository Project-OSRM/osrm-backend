#ifndef OSRM_PARTITIONER_CELL_STATISTICS_HPP
#define OSRM_PARTITIONER_CELL_STATISTICS_HPP

#include "util/log.hpp"
#include "util/typedefs.hpp"

#include <unordered_set>

namespace osrm::partitioner
{
template <typename Partition, typename CellStorage>
void printCellStatistics(const Partition &partition, const CellStorage &storage)
{
    util::Log() << "Cells statistics per level";

    for (std::size_t level = 1; level < partition.GetNumberOfLevels(); ++level)
    {
        auto num_cells = partition.GetNumberOfCells(level);
        std::size_t source = 0, destination = 0;
        std::size_t boundary_nodes = 0;
        std::size_t entries = 0;
        for (std::uint32_t cell_id = 0; cell_id < num_cells; ++cell_id)
        {
            std::unordered_set<NodeID> boundary;
            const auto &cell = storage.GetUnfilledCell(level, cell_id);
            source += cell.GetSourceNodes().size();
            destination += cell.GetDestinationNodes().size();
            for (auto node : cell.GetSourceNodes())
            {
                boundary.insert(node);
            }
            for (auto node : cell.GetDestinationNodes())
            {
                boundary.insert(node);
            }
            boundary_nodes += boundary.size();
            entries += cell.GetSourceNodes().size() * cell.GetDestinationNodes().size();
        }

        source /= num_cells;
        destination /= num_cells;

        util::Log() << "Level " << level << " #cells " << num_cells << " #boundary nodes "
                    << boundary_nodes << ", sources: avg. " << source << ", destinations: avg. "
                    << destination << ", entries: " << entries << " ("
                    << (2 * entries * sizeof(EdgeWeight)) << " bytes)";
    }
}
} // namespace osrm::partitioner

#endif
