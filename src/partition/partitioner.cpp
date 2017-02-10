#include "partition/partitioner.hpp"
#include "partition/annotated_partition.hpp"
#include "partition/bisection_graph.hpp"
#include "partition/compressed_node_based_graph_reader.hpp"
#include "partition/edge_based_graph_reader.hpp"
#include "partition/node_based_graph_to_edge_based_graph_mapping_reader.hpp"
#include "partition/recursive_bisection.hpp"

#include "util/coordinate.hpp"
#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"
#include "util/integer_range.hpp"
#include "util/json_container.hpp"
#include "util/log.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/assert.hpp>

#include "util/geojson_debug_logger.hpp"
#include "util/geojson_debug_policies.hpp"
#include "util/json_container.hpp"
#include "util/timing_util.hpp"

namespace osrm
{
namespace partition
{

void LogStatistics(const std::string &filename, std::vector<std::uint32_t> bisection_ids)
{
    auto compressed_node_based_graph = LoadCompressedNodeBasedGraph(filename);

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

    groupEdgesBySource(begin(compressed_node_based_graph.edges),
                       end(compressed_node_based_graph.edges));

    auto graph =
        makeBisectionGraph(compressed_node_based_graph.coordinates,
                           adaptToBisectionEdge(std::move(compressed_node_based_graph.edges)));

    TIMER_START(annotation);
    AnnotatedPartition partition(graph, bisection_ids);
    TIMER_STOP(annotation);
    std::cout << "Annotation took " << TIMER_SEC(annotation) << " seconds" << std::endl;
}

void LogGeojson(const std::string &filename, const std::vector<std::uint32_t> &bisection_ids)
{
    // reload graph, since we destroyed the old one
    auto compressed_node_based_graph = LoadCompressedNodeBasedGraph(filename);

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

    groupEdgesBySource(begin(compressed_node_based_graph.edges),
                       end(compressed_node_based_graph.edges));

    auto graph =
        makeBisectionGraph(compressed_node_based_graph.coordinates,
                           adaptToBisectionEdge(std::move(compressed_node_based_graph.edges)));

    const auto get_level = [](const std::uint32_t lhs, const std::uint32_t rhs) {
        auto xored = lhs ^ rhs;
        std::uint32_t level = log(xored) / log(2.0);
        return level;
    };

    std::vector<std::vector<util::Coordinate>> border_vertices(33);

    for (NodeID nid = 0; nid < graph.NumberOfNodes(); ++nid)
    {
        const auto source_id = bisection_ids[nid];
        for (const auto &edge : graph.Edges(nid))
        {
            const auto target_id = bisection_ids[edge.target];
            if (source_id != target_id)
            {
                auto level = get_level(source_id, target_id);
                border_vertices[level].push_back(graph.Node(nid).coordinate);
                border_vertices[level].push_back(graph.Node(edge.target).coordinate);
            }
        }
    }

    util::ScopedGeojsonLoggerGuard<util::CoordinateVectorToMultiPoint> guard(
        "border_vertices.geojson");
    std::size_t level = 0;
    for (auto &bv : border_vertices)
    {
        if (!bv.empty())
        {
            std::sort(bv.begin(), bv.end(), [](const auto lhs, const auto rhs) {
                return std::tie(lhs.lon, lhs.lat) < std::tie(rhs.lon, rhs.lat);
            });
            bv.erase(std::unique(bv.begin(), bv.end()), bv.end());

            util::json::Object jslevel;
            jslevel.values["level"] = util::json::Number(level++);
            guard.Write(bv, jslevel);
        }
    }
}

int Partitioner::Run(const PartitionConfig &config)
{
    auto compressed_node_based_graph =
        LoadCompressedNodeBasedGraph(config.compressed_node_based_graph_path.string());

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

    groupEdgesBySource(begin(compressed_node_based_graph.edges),
                       end(compressed_node_based_graph.edges));

    auto graph =
        makeBisectionGraph(compressed_node_based_graph.coordinates,
                           adaptToBisectionEdge(std::move(compressed_node_based_graph.edges)));

    util::Log() << " running partition: " << config.maximum_cell_size << " " << config.balance
                << " " << config.boundary_factor << " " << config.num_optimizing_cuts << " "
                << config.small_component_size
                << " # max_cell_size balance boundary cuts small_component_size";
    RecursiveBisection recursive_bisection(graph,
                                           config.maximum_cell_size,
                                           config.balance,
                                           config.boundary_factor,
                                           config.num_optimizing_cuts,
                                           config.small_component_size);

    LogStatistics(config.compressed_node_based_graph_path.string(),
                  recursive_bisection.BisectionIDs());

    // Up until now we worked on the compressed node based graph.
    // But what we actually need is a partition for the edge based graph to work on.
    // The following loads a mapping from node based graph to edge based graph.
    // Then loads the edge based graph tanslates the partition and modifies it.
    // For details see #3205

    auto mapping = LoadNodeBasedGraphToEdgeBasedGraphMapping(config.nbg_ebg_mapping_path.string());
    util::Log() << "Loaded node based graph to edge based graph mapping";

    auto edge_based_graph = LoadEdgeBasedGraph(config.edge_based_graph_path.string());
    util::Log() << "Loaded edge based graph for mapping partition ids: "
                << edge_based_graph->GetNumberOfEdges() << " edges, "
                << edge_based_graph->GetNumberOfNodes() << " nodes";

    // TODO: put translation into own function / file

    const auto &partition_ids = recursive_bisection.BisectionIDs();

    std::vector<NodeID> edge_based_border_nodes;

    // Extract edge based border nodes, based on node based partition and mapping.
    for (const auto node_id : util::irange(0u, edge_based_graph->GetNumberOfNodes()))
    {
        const auto node_based_nodes = mapping.Lookup(node_id);

        const auto u = node_based_nodes.u;
        const auto v = node_based_nodes.v;

        if (partition_ids[u] == partition_ids[v])
        {
            // Can use partition_ids[u/v] as partition for edge based graph `node_id`
        }
        else
        {
            // Border nodes u,v - need to be resolved.
            edge_based_border_nodes.push_back(node_id);
        }
    }

    util::Log() << "Fixing " << edge_based_border_nodes.size() << " edge based graph border nodes";

    std::vector<std::pair<NodeID, EdgeBasedGraphEdgeData>> incoming_edges;

    for (const auto border_node : edge_based_border_nodes)
    {
        for (const auto edge : edge_based_graph->GetAdjacentEdgeRange(border_node))
        {
            const auto &data = edge_based_graph->GetEdgeData(edge);

            if (data.backward)
            {
                incoming_edges.emplace_back(edge_based_graph->GetTarget(edge), data);
                edge_based_graph->DeleteEdge(border_node, edge);
            }
        }

        const auto artificial = edge_based_graph->InsertNode();

        EdgeBasedGraphEdgeData dummy{SPECIAL_EDGEID, /*is_boundary_arc=*/true, 0, 0, false, false};

        for (const auto edge : incoming_edges)
        {
            edge_based_graph->InsertEdge(edge.first, artificial, edge.second);
        }

        incoming_edges.clear();
    }

    return 0;
}

} // namespace partition
} // namespace osrm
