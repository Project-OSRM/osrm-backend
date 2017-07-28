#include "extractor/node_data_container.hpp"

#include "customizer/cell_customizer.hpp"
#include "customizer/customizer.hpp"
#include "customizer/edge_based_graph.hpp"
#include "customizer/files.hpp"

#include "partition/cell_storage.hpp"
#include "partition/edge_based_graph_reader.hpp"
#include "partition/files.hpp"
#include "partition/multi_level_partition.hpp"

#include "storage/shared_memory_ownership.hpp"

#include "updater/updater.hpp"

#include "util/log.hpp"
#include "util/timing_util.hpp"

namespace osrm
{
namespace customizer
{

template <typename Graph, typename Partition, typename CellStorage>
void CellStorageStatistics(const Graph &graph,
                           const Partition &partition,
                           const CellStorage &storage,
                           const CellMetric &metric)
{
    util::Log() << "Cells statistics per level";

    for (std::size_t level = 1; level < partition.GetNumberOfLevels(); ++level)
    {
        std::unordered_map<CellID, std::size_t> cell_nodes;
        for (auto node : util::irange(0u, graph.GetNumberOfNodes()))
        {
            ++cell_nodes[partition.GetCell(level, node)];
        }

        std::size_t source = 0, destination = 0, total = 0;
        std::size_t invalid_sources = 0, invalid_destinations = 0;
        for (std::uint32_t cell_id = 0; cell_id < partition.GetNumberOfCells(level); ++cell_id)
        {
            const auto &cell = storage.GetCell(metric, level, cell_id);
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
                    << ",   source nodes: average " << source << " (" << (100. * source / total)
                    << "%)"
                    << " invalid " << invalid_sources << " (" << (100. * invalid_sources / total)
                    << "%)"
                    << ",   destination nodes: average " << destination << " ("
                    << (100. * destination / total) << "%)"
                    << " invalid " << invalid_destinations << " ("
                    << (100. * invalid_destinations / total) << "%)";
    }
}

auto LoadAndUpdateEdgeExpandedGraph(const CustomizationConfig &config,
                                    const partition::MultiLevelPartition &mlp)
{
    updater::Updater updater(config.updater_config);

    EdgeID num_nodes;
    std::vector<extractor::EdgeBasedEdge> edge_based_edge_list;
    std::tie(num_nodes, edge_based_edge_list) = updater.LoadAndUpdateEdgeExpandedGraph();

    auto directed = partition::splitBidirectionalEdges(edge_based_edge_list);
    auto tidied =
        partition::prepareEdgesForUsageInGraph<StaticEdgeBasedGraphEdge>(std::move(directed));
    auto edge_based_graph = customizer::MultiLevelEdgeBasedGraph(mlp, num_nodes, std::move(tidied));

    return edge_based_graph;
}

std::vector<std::vector<bool>>
avoidFlagsToNodeFilter(const MultiLevelEdgeBasedGraph &graph,
                       const extractor::EdgeBasedNodeDataContainer &node_data,
                       const extractor::ProfileProperties &properties)
{
    std::vector<std::vector<bool>> filters;
    for (auto mask : properties.avoidable_classes)
    {
        if (mask != extractor::INAVLID_CLASS_DATA)
        {
            std::vector<bool> allowed_nodes(graph.GetNumberOfNodes(), true);
            for (const auto node : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
            {
                allowed_nodes[node] = (node_data.GetClassData(node) & mask) == 0;
            }
            filters.push_back(std::move(allowed_nodes));
        }
    }
    return filters;
}

std::vector<CellMetric> filterToMetrics(const MultiLevelEdgeBasedGraph &graph,
                                        const partition::CellStorage &storage,
                                        const partition::MultiLevelPartition &mlp,
                                        const std::vector<std::vector<bool>> &node_filters)
{
    CellCustomizer customizer(mlp);
    std::vector<CellMetric> metrics;

    for (auto filter : node_filters)
    {
        auto metric = storage.MakeMetric();
        customizer.Customize(graph, storage, filter, metric);
        metrics.push_back(std::move(metric));
    }

    return metrics;
}

int Customizer::Run(const CustomizationConfig &config)
{
    TIMER_START(loading_data);

    partition::MultiLevelPartition mlp;
    partition::files::readPartition(config.GetPath(".osrm.partition"), mlp);

    auto graph = LoadAndUpdateEdgeExpandedGraph(config, mlp);
    util::Log() << "Loaded edge based graph: " << graph.GetNumberOfEdges() << " edges, "
                << graph.GetNumberOfNodes() << " nodes";

    partition::CellStorage storage;
    partition::files::readCells(config.GetPath(".osrm.cells"), storage);
    TIMER_STOP(loading_data);

    extractor::EdgeBasedNodeDataContainer node_data;
    extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);

    extractor::ProfileProperties properties;
    extractor::files::readProfileProperties(config.GetPath(".osrm.properties"), properties);

    util::Log() << "Loading partition data took " << TIMER_SEC(loading_data) << " seconds";

    TIMER_START(cell_customize);
    auto filter = avoidFlagsToNodeFilter(graph, node_data, properties);
    auto metrics = filterToMetrics(graph, storage, mlp, filter);
    TIMER_STOP(cell_customize);
    util::Log() << "Cells customization took " << TIMER_SEC(cell_customize) << " seconds";

    TIMER_START(writing_mld_data);
    files::writeCellMetrics(config.GetPath(".osrm.cell_metrics"), metrics);
    TIMER_STOP(writing_mld_data);
    util::Log() << "MLD customization writing took " << TIMER_SEC(writing_mld_data) << " seconds";

    TIMER_START(writing_graph);
    partition::files::writeGraph(config.GetPath(".osrm.mldgr"), graph);
    TIMER_STOP(writing_graph);
    util::Log() << "Graph writing took " << TIMER_SEC(writing_graph) << " seconds";

    for (const auto &metric : metrics)
    {
        CellStorageStatistics(graph, mlp, storage, metric);
    }

    return 0;
}

} // namespace customizer$
} // namespace osrm
