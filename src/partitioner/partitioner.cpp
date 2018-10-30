#include "partitioner/partitioner.hpp"
#include "partitioner/bisection_graph.hpp"
#include "partitioner/bisection_to_partition.hpp"
#include "partitioner/cell_statistics.hpp"
#include "partitioner/cell_storage.hpp"
#include "partitioner/edge_based_graph_reader.hpp"
#include "partitioner/files.hpp"
#include "partitioner/multi_level_partition.hpp"
#include "partitioner/recursive_bisection.hpp"
#include "partitioner/remove_unconnected.hpp"
#include "partitioner/renumber.hpp"

#include "extractor/compressed_node_based_graph_edge.hpp"
#include "extractor/files.hpp"

#include "util/coordinate.hpp"
#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"
#include "util/integer_range.hpp"
#include "util/json_container.hpp"
#include "util/log.hpp"
#include "util/mmap_file.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>

#include <tbb/task_scheduler_init.h>

#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"
#include "util/json_container.hpp"
#include "util/timing_util.hpp"

namespace osrm
{
namespace partitioner
{
auto getGraphBisection(const PartitionerConfig &config)
{
    std::vector<extractor::CompressedNodeBasedGraphEdge> edges;
    extractor::files::readCompressedNodeBasedGraph(config.GetPath(".osrm.cnbg"), edges);
    groupEdgesBySource(begin(edges), end(edges));

    std::vector<util::Coordinate> coordinates;
    extractor::files::readNodeCoordinates(config.GetPath(".osrm.nbg_nodes"), coordinates);

    util::Log() << "Loaded compressed node based graph: " << edges.size() << " edges, "
                << coordinates.size() << " nodes";

    auto graph = makeBisectionGraph(coordinates, adaptToBisectionEdge(std::move(edges)));

    util::Log() << " running partition: " << config.max_cell_sizes.front() << " " << config.balance
                << " " << config.boundary_factor << " " << config.num_optimizing_cuts << " "
                << config.small_component_size
                << " # max_cell_size balance boundary cuts small_component_size";
    RecursiveBisection recursive_bisection(graph,
                                           config.max_cell_sizes.front(),
                                           config.balance,
                                           config.boundary_factor,
                                           config.num_optimizing_cuts,
                                           config.small_component_size);

    // Return bisection ids, keyed by node based graph nodes
    return recursive_bisection.BisectionIDs();
}

int Partitioner::Run(const PartitionerConfig &config)
{
    tbb::task_scheduler_init init(config.requested_num_threads);
    BOOST_ASSERT(init.is_active());

    const std::vector<BisectionID> &node_based_partition_ids = getGraphBisection(config);

    // Up until now we worked on the compressed node based graph.
    // But what we actually need is a partition for the edge based graph to work on.
    // The following loads a mapping from node based graph to edge based graph.
    // Then loads the edge based graph tanslates the partition and modifies it.
    // For details see #3205

    std::vector<extractor::NBGToEBG> mapping;
    extractor::files::readNBGMapping(config.GetPath(".osrm.cnbg_to_ebg").string(), mapping);
    util::Log() << "Loaded node based graph to edge based graph mapping";

    auto edge_based_graph = LoadEdgeBasedGraph(config.GetPath(".osrm.ebg").string());
    util::Log() << "Loaded edge based graph for mapping partition ids: "
                << edge_based_graph.GetNumberOfEdges() << " edges, "
                << edge_based_graph.GetNumberOfNodes() << " nodes";

    // Partition ids, keyed by edge based graph nodes
    std::vector<NodeID> edge_based_partition_ids(edge_based_graph.GetNumberOfNodes(),
                                                 SPECIAL_NODEID);

    // Only resolve all easy cases in the first pass
    for (const auto &entry : mapping)
    {
        const auto u = entry.u;
        const auto v = entry.v;
        const auto forward_node = entry.forward_ebg_node;
        const auto backward_node = entry.backward_ebg_node;

        // This heuristic strategy seems to work best, even beating chosing the minimum
        // border edge bisection ID
        edge_based_partition_ids[forward_node] = node_based_partition_ids[u];
        if (backward_node != SPECIAL_NODEID)
            edge_based_partition_ids[backward_node] = node_based_partition_ids[v];
    }

    std::vector<Partition> partitions;
    std::vector<std::uint32_t> level_to_num_cells;
    std::tie(partitions, level_to_num_cells) =
        bisectionToPartition(edge_based_partition_ids, config.max_cell_sizes);

    auto num_unconnected = removeUnconnectedBoundaryNodes(edge_based_graph, partitions);
    util::Log() << "Fixed " << num_unconnected << " unconnected nodes";

    util::Log() << "Edge-based-graph annotation:";
    for (std::size_t level = 0; level < level_to_num_cells.size(); ++level)
    {
        util::Log() << "  level " << level + 1 << " #cells " << level_to_num_cells[level]
                    << " bit size " << std::ceil(std::log2(level_to_num_cells[level] + 1));
    }

    TIMER_START(renumber);
    auto permutation = makePermutation(edge_based_graph, partitions);
    renumber(edge_based_graph, permutation);
    renumber(partitions, permutation);
    {
        renumber(mapping, permutation);
        extractor::files::writeNBGMapping(config.GetPath(".osrm.cnbg_to_ebg").string(), mapping);
    }
    {
        boost::iostreams::mapped_file segment_region;
        auto segments = util::mmapFile<extractor::EdgeBasedNodeSegment>(
            config.GetPath(".osrm.fileIndex"), segment_region);
        renumber(segments, permutation);
    }
    {
        extractor::EdgeBasedNodeDataContainer node_data;
        extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);
        renumber(node_data, permutation);
        extractor::files::writeNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);
    }
    {
        std::vector<EdgeWeight> node_weights;
        std::vector<EdgeDuration> node_durations;
        std::vector<EdgeDuration> node_distances;
        extractor::files::readEdgeBasedNodeWeightsDurations(
            config.GetPath(".osrm.enw"), node_weights, node_durations);
        extractor::files::readEdgeBasedNodeDistances(config.GetPath(".osrm.enw"), node_distances);
        util::inplacePermutation(node_weights.begin(), node_weights.end(), permutation);
        util::inplacePermutation(node_durations.begin(), node_durations.end(), permutation);
        util::inplacePermutation(node_distances.begin(), node_distances.end(), permutation);
        extractor::files::writeEdgeBasedNodeWeightsDurationsDistances(
            config.GetPath(".osrm.enw"), node_weights, node_durations, node_distances);
    }
    {
        const auto &filename = config.GetPath(".osrm.maneuver_overrides");
        std::vector<extractor::StorageManeuverOverride> maneuver_overrides;
        std::vector<NodeID> node_sequences;
        extractor::files::readManeuverOverrides(filename, maneuver_overrides, node_sequences);
        renumber(maneuver_overrides, permutation);
        renumber(node_sequences, permutation);
        extractor::files::writeManeuverOverrides(filename, maneuver_overrides, node_sequences);
    }
    if (boost::filesystem::exists(config.GetPath(".osrm.hsgr")))
    {
        util::Log(logWARNING) << "Found existing .osrm.hsgr file, removing. You need to re-run "
                                 "osrm-contract after osrm-partition.";
        boost::filesystem::remove(config.GetPath(".osrm.hsgr"));
    }
    TIMER_STOP(renumber);
    util::Log() << "Renumbered data in " << TIMER_SEC(renumber) << " seconds";

    TIMER_START(packed_mlp);
    MultiLevelPartition mlp{partitions, level_to_num_cells};
    TIMER_STOP(packed_mlp);
    util::Log() << "MultiLevelPartition constructed in " << TIMER_SEC(packed_mlp) << " seconds";

    TIMER_START(cell_storage);
    CellStorage storage(mlp, edge_based_graph);
    TIMER_STOP(cell_storage);
    util::Log() << "CellStorage constructed in " << TIMER_SEC(cell_storage) << " seconds";

    TIMER_START(writing_mld_data);
    files::writePartition(config.GetPath(".osrm.partition"), mlp);
    files::writeCells(config.GetPath(".osrm.cells"), storage);
    extractor::files::writeEdgeBasedGraph(config.GetPath(".osrm.ebg"),
                                          edge_based_graph.GetNumberOfNodes(),
                                          graphToEdges(edge_based_graph),
                                          edge_based_graph.connectivity_checksum);
    TIMER_STOP(writing_mld_data);
    util::Log() << "MLD data writing took " << TIMER_SEC(writing_mld_data) << " seconds";

    printCellStatistics(mlp, storage);

    return 0;
}

} // namespace partitioner
} // namespace osrm
