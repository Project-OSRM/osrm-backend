#include "customizer/customizer.hpp"
#include "customizer/cell_customizer.hpp"
#include "partition/edge_based_graph_reader.hpp"
#include "partition/io.hpp"

#include "partition/cell_storage.hpp"
#include "partition/io.hpp"
#include "partition/multi_level_partition.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"

namespace osrm
{
namespace customize
{

template <typename Graph, typename Partition, typename CellStorage>
void CellStorageStatistics(const Graph &graph,
                           const Partition &partition,
                           const CellStorage &storage)
{
    util::Log() << "Cells statistics per level";

    for (std::size_t level = 1; level < partition.GetNumberOfLevels(); ++level)
    {
        std::unordered_map<partition::CellID, std::size_t> cell_nodes;
        for (auto node : util::irange(0u, graph.GetNumberOfNodes()))
        {
            ++cell_nodes[partition.GetCell(level, node)];
        }

        std::size_t source = 0, destination = 0, total = 0;
        std::size_t invalid_sources = 0, invalid_destinations = 0;
        for (std::uint32_t cell_id = 0; cell_id < partition.GetNumberOfCells(level); ++cell_id)
        {
            const auto &cell = storage.GetCell(level, cell_id);
            source += cell.GetSourceNodes().size();
            destination += cell.GetDestinationNodes().size();
            total += cell_nodes[cell_id];
            for (auto node : cell.GetSourceNodes())
            {
                const auto &weights = cell.GetOutWeight(node);
                invalid_sources += std::all_of(weights.begin(), weights.end(), [](auto weight) {
                    return weight == INVALID_EDGE_WEIGHT;
                });
            }
            for (auto node : cell.GetDestinationNodes())
            {
                const auto &weights = cell.GetInWeight(node);
                invalid_destinations +=
                    std::all_of(weights.begin(), weights.end(), [](auto weight) {
                        return weight == INVALID_EDGE_WEIGHT;
                    });
            }
        }

        util::Log() << "Level " << level << " #cells " << cell_nodes.size() << " #nodes " << total
                    << ",   source nodes: average " << source
                    << " (" << (100. * source / total) << "%)"
                    << " invalid " << invalid_sources
                    << " (" << (100. * invalid_sources / total) << "%)"
                    << ",   destination nodes: average " << destination
                    << " (" << (100. * destination / total) << "%)"
                    << " invalid " << invalid_destinations
                    << " (" << (100. * invalid_destinations / total) << "%)";
    }
}

int Customizer::Run(const CustomizationConfig &config)
{
    TIMER_START(loading_data);
    auto edge_based_graph = partition::LoadEdgeBasedGraph(config.edge_based_graph_path.string());
    util::Log() << "Loaded edge based graph for mapping partition ids: "
                << edge_based_graph->GetNumberOfEdges() << " edges, "
                << edge_based_graph->GetNumberOfNodes() << " nodes";

    osrm::partition::MultiLevelPartition mlp;
    partition::io::read(config.mld_partition_path, mlp);

    partition::CellStorage storage(mlp, *edge_based_graph);
    TIMER_STOP(loading_data);
    util::Log() << "Loading partition data took " << TIMER_SEC(loading_data) << " seconds";

    TIMER_START(cell_customize);
    CellCustomizer customizer(mlp);
    customizer.Customize(*edge_based_graph, storage);
    TIMER_STOP(cell_customize);
    util::Log() << "Cells customization took " << TIMER_SEC(cell_customize) << " seconds";

    TIMER_START(writing_mld_data);
    partition::io::write(config.mld_storage_path, storage);
    TIMER_STOP(writing_mld_data);
    util::Log() << "MLD customization writing took " << TIMER_SEC(writing_mld_data) << " seconds";

    CellStorageStatistics(*edge_based_graph, mlp, storage);

    return 0;
}

} // namespace customize
} // namespace osrm
