#include "extractor/node_data_container.hpp"

#include "customizer/cell_customizer.hpp"
#include "customizer/customizer.hpp"
#include "customizer/edge_based_graph.hpp"
#include "customizer/files.hpp"

#include "partitioner/cell_statistics.hpp"
#include "partitioner/cell_storage.hpp"
#include "partitioner/edge_based_graph_reader.hpp"
#include "partitioner/files.hpp"
#include "partitioner/multi_level_partition.hpp"

#include "storage/shared_memory_ownership.hpp"

#include "updater/updater.hpp"

#include "util/exclude_flag.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <boost/assert.hpp>

#include <tbb/task_scheduler_init.h>

namespace osrm
{
namespace customizer
{

namespace
{

template <typename Partition, typename CellStorage>
void printUnreachableStatistics(const Partition &partition,
                                const CellStorage &storage,
                                const CellMetric &metric)
{
    util::Log() << "Unreachable nodes statistics per level";

    for (std::size_t level = 1; level < partition.GetNumberOfLevels(); ++level)
    {
        auto num_cells = partition.GetNumberOfCells(level);
        std::size_t invalid_sources = 0;
        std::size_t invalid_destinations = 0;
        for (std::uint32_t cell_id = 0; cell_id < num_cells; ++cell_id)
        {
            const auto &cell = storage.GetCell(metric, level, cell_id);
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

        if (invalid_sources > 0 || invalid_destinations > 0)
        {
            util::Log(logWARNING) << "Level " << level << " unreachable boundary nodes per cell: "
                                  << (invalid_sources / (float)num_cells) << " sources, "
                                  << (invalid_destinations / (float)num_cells) << " destinations";
        }
    }
}

auto LoadAndUpdateEdgeExpandedGraph(const CustomizationConfig &config,
                                    const partitioner::MultiLevelPartition &mlp,
                                    std::vector<EdgeWeight> &node_weights,
                                    std::vector<EdgeDuration> &node_durations,
                                    std::vector<EdgeDistance> &node_distances,
                                    std::uint32_t &connectivity_checksum,
                                    updater::NodeSetPtr node_updated)
{
    updater::Updater updater(config.updater_config);

    std::vector<extractor::EdgeBasedEdge> edge_based_edge_list;
    EdgeID num_nodes = updater.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edge_list, node_weights, node_durations, connectivity_checksum, node_updated);

    extractor::files::readEdgeBasedNodeDistances(config.GetPath(".osrm.enw"), node_distances);

    auto directed = partitioner::splitBidirectionalEdges(edge_based_edge_list);

    auto tidied = partitioner::prepareEdgesForUsageInGraph<
        typename partitioner::MultiLevelEdgeBasedGraph::InputEdge>(std::move(directed));

    auto edge_based_graph =
        partitioner::MultiLevelEdgeBasedGraph(mlp, num_nodes, std::move(tidied));

    return edge_based_graph;
}

bool customizeFilteredMetrics(const partitioner::MultiLevelEdgeBasedGraph &graph,
                                                 const partitioner::CellStorage &storage,
                                                 const CellCustomizer &customizer,
                                                 const std::vector<std::vector<bool>> &node_filters,
                                                 const CellUpdateRecord &cell_update_record,
                                                 std::vector<CellMetric>& metrics,
                                                 bool incremental)
{
    if (metrics.size() != node_filters.size())
    {
        util::Log(logERROR) << "Under incremental mod, metrics array is compatible with nodefilter array.";
        util::Log(logERROR) << "metrics.size() = " << metrics.size() << " node_filters.size() = " << node_filters.size()<< std::endl;
        return false;
    }

    for (size_t i = 0; i < metrics.size(); ++i)
    {
        if (incremental)
        {
            customizer.Customize(graph, storage, node_filters[i], metrics[i], cell_update_record);
        }
        else
        {
            auto metric = storage.MakeMetric();
            customizer.Customize(graph, storage, node_filters[i], metric, cell_update_record);
            metrics[i] = std::move(metric);
            //metrics.push_back(std::move(metric));
        }
    }

    return true;
}

}

int Customizer::Run(const CustomizationConfig &config)
{
    tbb::task_scheduler_init init(config.requested_num_threads);
    BOOST_ASSERT(init.is_active());

    TIMER_START(loading_data);

    partitioner::MultiLevelPartition mlp;
    partitioner::files::readPartition(config.GetPath(".osrm.partition"), mlp);

    std::vector<EdgeWeight> node_weights;
    std::vector<EdgeDuration> node_durations; // TODO: remove when durations are optional
    std::vector<EdgeDistance> node_distances; // TODO: remove when distances are optional
    std::uint32_t connectivity_checksum = 0;

    updater::NodeSetPtr node_updated = config.updater_config.incremental ? std::make_shared<updater::NodeSet>() : nullptr;
    auto graph = LoadAndUpdateEdgeExpandedGraph(
        config, mlp, node_weights, node_durations, node_distances, connectivity_checksum, node_updated);
    BOOST_ASSERT(graph.GetNumberOfNodes() == node_weights.size());
    std::for_each(node_weights.begin(), node_weights.end(), [](auto &w) { w &= 0x7fffffff; });
    util::Log() << "Loaded edge based graph: " << graph.GetNumberOfEdges() << " edges, "
                << graph.GetNumberOfNodes() << " nodes";
    
    if (config.updater_config.incremental && node_updated)
    {
        double percentage = 0;
        if (0 != graph.GetNumberOfNodes())
        {
            percentage = (double)(node_updated->size() * 100) / graph.GetNumberOfNodes();
        }
        util::Log() << "Collect " << node_updated->size() << " updated nodes, "
                    << "about " << percentage << "% of total nodes in graph.";
    }

    partitioner::CellStorage storage;
    partitioner::files::readCells(config.GetPath(".osrm.cells"), storage);
    TIMER_STOP(loading_data);

    extractor::EdgeBasedNodeDataContainer node_data;
    extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);

    extractor::ProfileProperties properties;
    extractor::files::readProfileProperties(config.GetPath(".osrm.properties"), properties);

    util::Log() << "Loading partition data took " << TIMER_SEC(loading_data) << " seconds";

    std::vector<CellMetric> metrics;
    for (auto mask : properties.excludable_classes)
    {
        if (mask != extractor::INAVLID_CLASS_DATA)
        {
            metrics.push_back(CellMetric());
        }
    }

    if (config.updater_config.incremental)
    {
        TIMER_START(reading_mld_data);
        if (boost::filesystem::exists(config.GetPath(".osrm.cell_metrics")))
        {
            std::unordered_map<std::string, std::vector<CellMetric>> previous_metric_exclude_classes = {
            {properties.GetWeightName(), std::move(metrics)},
            };
            customizer::files::readCellMetrics(config.GetPath(".osrm.cell_metrics"), previous_metric_exclude_classes);
            metrics = std::move(previous_metric_exclude_classes[properties.GetWeightName()]);
        }
        else
        {
            util::Log() << "Incremental mode must have valid cell_metrics.";
            return -1;
        }
        TIMER_STOP(reading_mld_data);
        util::Log() << "MLD customization reading took " << TIMER_SEC(reading_mld_data) << " seconds";
    }

    TIMER_START(cell_customize);
    CellUpdateRecord cell_update_record(mlp, config.updater_config.incremental);
    {
        updater::NodeSetViewerPtr node_updated_viewer = std::move(node_updated); 
        cell_update_record.Collect(node_updated_viewer); 
    }
    util::Log() << cell_update_record.Statistic();

    auto filter = util::excludeFlagsToNodeFilter(graph.GetNumberOfNodes(), node_data, properties);
    auto metrics_succeed = customizeFilteredMetrics(graph, storage, CellCustomizer{mlp}, filter, cell_update_record, metrics, config.updater_config.incremental);
    if (!metrics_succeed) 
    {
        util::Log() << "Critical error in metrics table generation!" << std::endl;
        return -1;
    }
    TIMER_STOP(cell_customize);
    util::Log() << "Cells customization took " << TIMER_SEC(cell_customize) << " seconds";

    partitioner::printCellStatistics(mlp, storage);
    for (const auto &metric : metrics)
    {
        printUnreachableStatistics(mlp, storage, metric);
    }

    TIMER_START(writing_mld_data);
    std::unordered_map<std::string, std::vector<CellMetric>> metric_exclude_classes = {
        {properties.GetWeightName(), std::move(metrics)},
    };
    files::writeCellMetrics(config.GetPath(".osrm.cell_metrics"), metric_exclude_classes);
    TIMER_STOP(writing_mld_data);
    util::Log() << "MLD customization writing took " << TIMER_SEC(writing_mld_data) << " seconds";

    TIMER_START(writing_graph);
    MultiLevelEdgeBasedGraph shaved_graph{std::move(graph),
                                          std::move(node_weights),
                                          std::move(node_durations),
                                          std::move(node_distances)};
    customizer::files::writeGraph(
        config.GetPath(".osrm.mldgr"), shaved_graph, connectivity_checksum);
    TIMER_STOP(writing_graph);
    util::Log() << "Graph writing took " << TIMER_SEC(writing_graph) << " seconds";

    return 0;
}

} // namespace customizer$
} // namespace osrm
