#include "extractor/extractor.hpp"

#include "extractor/edge_based_edge.hpp"
#include "extractor/extraction_containers.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/extractor_callbacks.hpp"
#include "extractor/files.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/restriction_filter.hpp"
#include "extractor/restriction_parser.hpp"
#include "extractor/scripting_environment.hpp"

#include "storage/io.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/graph_loader.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/name_table.hpp"
#include "util/range_table.hpp"
#include "util/timing_util.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/restriction_map.hpp"
#include "extractor/way_restriction_map.hpp"
#include "util/static_graph.hpp"
#include "util/static_rtree.hpp"

// Keep debug include to make sure the debug header is in sync with types.
#include "util/debug.hpp"

#include "extractor/tarjan_scc.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/optional/optional.hpp>
#include <boost/scope_exit.hpp>

#include <osmium/io/any_input.hpp>

#include <tbb/pipeline.h>
#include <tbb/task_scheduler_init.h>

#include <cstdlib>

#include <algorithm>
#include <atomic>
#include <bitset>
#include <chrono>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace osrm
{
namespace extractor
{

namespace
{
// Converts the class name map into a fixed mapping of index to name
void SetClassNames(const ExtractorCallbacks::ClassesMap &classes_map,
                   ProfileProperties &profile_properties)
{
    for (const auto &pair : classes_map)
    {
        auto range = getClassIndexes(pair.second);
        BOOST_ASSERT(range.size() == 1);
        profile_properties.SetClassName(range.front(), pair.first);
    }
}
}

/**
 * TODO: Refactor this function into smaller functions for better readability.
 *
 * This function is the entry point for the whole extraction process. The goal of the extraction
 * step is to filter and convert the OSM geometry to something more fitting for routing.
 * That includes:
 *  - extracting turn restrictions
 *  - splitting ways into (directional!) edge segments
 *  - checking if nodes are barriers or traffic signal
 *  - discarding all tag information: All relevant type information for nodes/ways
 *    is extracted at this point.
 *
 * The result of this process are the following files:
 *  .names : Names of all streets, stored as long consecutive string with prefix sum based index
 *  .osrm  : Nodes and edges in a intermediate format that easy to digest for osrm-contract
 *  .restrictions : Turn restrictions that are used by osrm-contract to construct the edge-expanded
 * graph
 *
 */
int Extractor::run(ScriptingEnvironment &scripting_environment)
{
    util::LogPolicy::GetInstance().Unmute();

    const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();
    const auto number_of_threads = std::min(recommended_num_threads, config.requested_num_threads);
    tbb::task_scheduler_init init(number_of_threads ? number_of_threads
                                                    : tbb::task_scheduler_init::automatic);

    guidance::LaneDescriptionMap turn_lane_map;
    std::vector<TurnRestriction> turn_restrictions;
    std::tie(turn_lane_map, turn_restrictions) =
        ParseOSMData(scripting_environment, number_of_threads);

    // Transform the node-based graph that OSM is based on into an edge-based graph
    // that is better for routing.  Every edge becomes a node, and every valid
    // movement (e.g. turn from A->B, and B->A) becomes an edge
    util::Log() << "Generating edge-expanded graph representation";

    TIMER_START(expansion);

    EdgeBasedNodeDataContainer edge_based_nodes_container;
    std::vector<EdgeBasedNodeSegment> edge_based_node_segments;
    util::DeallocatingVector<EdgeBasedEdge> edge_based_edge_list;
    std::vector<bool> node_is_startpoint;
    std::vector<EdgeWeight> edge_based_node_weights;
    std::vector<util::Coordinate> coordinates;
    extractor::PackedOSMIDs osm_node_ids;

    auto graph_size = BuildEdgeExpandedGraph(scripting_environment,
                                             coordinates,
                                             osm_node_ids,
                                             edge_based_nodes_container,
                                             edge_based_node_segments,
                                             node_is_startpoint,
                                             edge_based_node_weights,
                                             edge_based_edge_list,
                                             config.GetPath(".osrm.icd").string(),
                                             turn_restrictions,
                                             turn_lane_map);

    auto number_of_node_based_nodes = graph_size.first;
    auto max_edge_id = graph_size.second - 1;

    TIMER_STOP(expansion);

    util::Log() << "Saving edge-based node weights to file.";
    TIMER_START(timer_write_node_weights);
    {
        storage::io::FileWriter writer(config.GetPath(".osrm.enw"),
                                       storage::io::FileWriter::GenerateFingerprint);
        storage::serialization::write(writer, edge_based_node_weights);
    }
    TIMER_STOP(timer_write_node_weights);
    util::Log() << "Done writing. (" << TIMER_SEC(timer_write_node_weights) << ")";

    util::Log() << "Computing strictly connected components ...";
    FindComponents(
        max_edge_id, edge_based_edge_list, edge_based_node_segments, edge_based_nodes_container);

    util::Log() << "Building r-tree ...";
    TIMER_START(rtree);
    BuildRTree(std::move(edge_based_node_segments), std::move(node_is_startpoint), coordinates);

    TIMER_STOP(rtree);

    util::Log() << "Writing nodes for nodes-based and edges-based graphs ...";
    files::writeNodes(config.GetPath(".osrm.nbg_nodes"), coordinates, osm_node_ids);
    files::writeNodeData(config.GetPath(".osrm.ebg_nodes"), edge_based_nodes_container);

    util::Log() << "Writing edge-based-graph edges       ... " << std::flush;
    TIMER_START(write_edges);
    files::writeEdgeBasedGraph(config.GetPath(".osrm.ebg"), max_edge_id, edge_based_edge_list);
    TIMER_STOP(write_edges);
    util::Log() << "ok, after " << TIMER_SEC(write_edges) << "s";

    util::Log() << "Processed " << edge_based_edge_list.size() << " edges";

    const auto nodes_per_second =
        static_cast<std::uint64_t>(number_of_node_based_nodes / TIMER_SEC(expansion));
    const auto edges_per_second =
        static_cast<std::uint64_t>((max_edge_id + 1) / TIMER_SEC(expansion));

    util::Log() << "Expansion: " << nodes_per_second << " nodes/sec and " << edges_per_second
                << " edges/sec";
    util::Log() << "To prepare the data for routing, run: "
                << "./osrm-contract " << config.GetPath(".osrm");

    return 0;
}

std::tuple<guidance::LaneDescriptionMap, std::vector<TurnRestriction>>
Extractor::ParseOSMData(ScriptingEnvironment &scripting_environment,
                        const unsigned number_of_threads)
{
    TIMER_START(extracting);

    util::Log() << "Input file: " << config.input_path.filename().string();
    if (!config.profile_path.empty())
    {
        util::Log() << "Profile: " << config.profile_path.filename().string();
    }
    util::Log() << "Threads: " << number_of_threads;

    const osmium::io::File input_file(config.input_path.string());

    osmium::io::Reader reader(
        input_file, (config.use_metadata ? osmium::io::read_meta::yes : osmium::io::read_meta::no));

    const osmium::io::Header header = reader.header();

    unsigned number_of_nodes = 0;
    unsigned number_of_ways = 0;
    unsigned number_of_relations = 0;

    util::Log() << "Parsing in progress..";
    TIMER_START(parsing);

    ExtractionContainers extraction_containers;
    ExtractorCallbacks::ClassesMap classes_map;
    guidance::LaneDescriptionMap turn_lane_map;
    auto extractor_callbacks =
        std::make_unique<ExtractorCallbacks>(extraction_containers,
                                             classes_map,
                                             turn_lane_map,
                                             scripting_environment.GetProfileProperties());

    std::string generator = header.get("generator");
    if (generator.empty())
    {
        generator = "unknown tool";
    }
    util::Log() << "input file generated by " << generator;

    // write .timestamp data file
    std::string timestamp = header.get("osmosis_replication_timestamp");
    if (timestamp.empty())
    {
        timestamp = "n/a";
    }
    util::Log() << "timestamp: " << timestamp;

    storage::io::FileWriter timestamp_file(config.GetPath(".osrm.timestamp"),
                                           storage::io::FileWriter::GenerateFingerprint);

    timestamp_file.WriteFrom(timestamp.c_str(), timestamp.length());

    std::vector<std::string> restrictions = scripting_environment.GetRestrictions();
    // setup restriction parser
    const RestrictionParser restriction_parser(
        scripting_environment.GetProfileProperties().use_turn_restrictions,
        config.parse_conditionals,
        restrictions);

    std::mutex process_mutex;

    using SharedBuffer = std::shared_ptr<const osmium::memory::Buffer>;
    struct ParsedBuffer
    {
        SharedBuffer buffer;
        std::vector<std::pair<const osmium::Node &, ExtractionNode>> resulting_nodes;
        std::vector<std::pair<const osmium::Way &, ExtractionWay>> resulting_ways;
        std::vector<InputConditionalTurnRestriction> resulting_restrictions;
    };

    tbb::filter_t<void, SharedBuffer> buffer_reader(
        tbb::filter::serial_in_order, [&](tbb::flow_control &fc) {
            if (auto buffer = reader.read())
            {
                return std::make_shared<const osmium::memory::Buffer>(std::move(buffer));
            }
            else
            {
                fc.stop();
                return SharedBuffer{};
            }
        });
    tbb::filter_t<SharedBuffer, std::shared_ptr<ParsedBuffer>> buffer_transform(
        tbb::filter::parallel, [&](const SharedBuffer buffer) {
            if (!buffer)
                return std::shared_ptr<ParsedBuffer>{};

            auto parsed_buffer = std::make_shared<ParsedBuffer>();
            parsed_buffer->buffer = buffer;
            scripting_environment.ProcessElements(*buffer,
                                                  restriction_parser,
                                                  parsed_buffer->resulting_nodes,
                                                  parsed_buffer->resulting_ways,
                                                  parsed_buffer->resulting_restrictions);
            return parsed_buffer;
        });
    tbb::filter_t<std::shared_ptr<ParsedBuffer>, void> buffer_storage(
        tbb::filter::serial_in_order, [&](const std::shared_ptr<ParsedBuffer> parsed_buffer) {
            if (!parsed_buffer)
                return;

            number_of_nodes += parsed_buffer->resulting_nodes.size();
            // put parsed objects thru extractor callbacks
            for (const auto &result : parsed_buffer->resulting_nodes)
            {
                extractor_callbacks->ProcessNode(result.first, result.second);
            }
            number_of_ways += parsed_buffer->resulting_ways.size();
            for (const auto &result : parsed_buffer->resulting_ways)
            {
                extractor_callbacks->ProcessWay(result.first, result.second);
            }
            number_of_relations += parsed_buffer->resulting_restrictions.size();
            for (const auto &result : parsed_buffer->resulting_restrictions)
            {
                extractor_callbacks->ProcessRestriction(result);
            }
        });

    // Number of pipeline tokens that yielded the best speedup was about 1.5 * num_cores
    tbb::parallel_pipeline(tbb::task_scheduler_init::default_num_threads() * 1.5,
                           buffer_reader & buffer_transform & buffer_storage);

    TIMER_STOP(parsing);
    util::Log() << "Parsing finished after " << TIMER_SEC(parsing) << " seconds";

    util::Log() << "Raw input contains " << number_of_nodes << " nodes, " << number_of_ways
                << " ways, and " << number_of_relations << " relations";

    extractor_callbacks.reset();

    if (extraction_containers.all_edges_list.empty())
    {
        throw util::exception(std::string("There are no edges remaining after parsing.") +
                              SOURCE_REF);
    }

    extraction_containers.PrepareData(scripting_environment,
                                      config.GetPath(".osrm").string(),
                                      config.GetPath(".osrm.restrictions").string(),
                                      config.GetPath(".osrm.names").string());

    auto profile_properties = scripting_environment.GetProfileProperties();
    SetClassNames(classes_map, profile_properties);
    files::writeProfileProperties(config.GetPath(".osrm.properties").string(), profile_properties);

    TIMER_STOP(extracting);
    util::Log() << "extraction finished after " << TIMER_SEC(extracting) << "s";

    return std::make_tuple(std::move(turn_lane_map),
                           std::move(extraction_containers.unconditional_turn_restrictions));
}

void Extractor::FindComponents(unsigned max_edge_id,
                               const util::DeallocatingVector<EdgeBasedEdge> &input_edge_list,
                               const std::vector<EdgeBasedNodeSegment> &input_node_segments,
                               EdgeBasedNodeDataContainer &nodes_container) const
{
    using InputEdge = util::static_graph_details::SortableEdgeWithData<void>;
    using UncontractedGraph = util::StaticGraph<void>;
    std::vector<InputEdge> edges;
    edges.reserve(input_edge_list.size() * 2);

    for (const auto &edge : input_edge_list)
    {
        BOOST_ASSERT_MSG(static_cast<unsigned int>(std::max(edge.data.weight, 1)) > 0,
                         "edge distance < 1");
        BOOST_ASSERT(edge.source <= max_edge_id);
        BOOST_ASSERT(edge.target <= max_edge_id);
        if (edge.data.forward)
        {
            edges.push_back({edge.source, edge.target});
        }

        if (edge.data.backward)
        {
            edges.push_back({edge.target, edge.source});
        }
    }

    // Connect forward and backward nodes of each edge to enforce
    // forward and backward edge-based nodes be in one strongly-connected component
    for (const auto &segment : input_node_segments)
    {
        if (segment.reverse_segment_id.enabled)
        {
            BOOST_ASSERT(segment.forward_segment_id.id <= max_edge_id);
            BOOST_ASSERT(segment.reverse_segment_id.id <= max_edge_id);
            edges.push_back({segment.forward_segment_id.id, segment.reverse_segment_id.id});
            edges.push_back({segment.reverse_segment_id.id, segment.forward_segment_id.id});
        }
    }

    tbb::parallel_sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    auto uncontracted_graph = UncontractedGraph(max_edge_id + 1, edges);

    TarjanSCC<UncontractedGraph> component_search(uncontracted_graph);
    component_search.Run();

    for (NodeID node_id = 0; node_id <= max_edge_id; ++node_id)
    {
        const auto forward_component = component_search.GetComponentID(node_id);
        const auto component_size = component_search.GetComponentSize(forward_component);
        const auto is_tiny = component_size < config.small_component_size;
        nodes_container.SetComponentID(node_id, {1 + forward_component, is_tiny});
    }
}

/**
  \brief Load node based graph from .osrm file
  */
std::shared_ptr<util::NodeBasedDynamicGraph>
Extractor::LoadNodeBasedGraph(std::unordered_set<NodeID> &barriers,
                              std::unordered_set<NodeID> &traffic_signals,
                              std::vector<util::Coordinate> &coordiantes,
                              extractor::PackedOSMIDs &osm_node_ids)
{
    storage::io::FileReader file_reader(config.GetPath(".osrm"),
                                        storage::io::FileReader::VerifyFingerprint);

    auto barriers_iter = inserter(barriers, end(barriers));
    auto traffic_signals_iter = inserter(traffic_signals, end(traffic_signals));

    NodeID number_of_node_based_nodes = util::loadNodesFromFile(
        file_reader, barriers_iter, traffic_signals_iter, coordiantes, osm_node_ids);

    util::Log() << " - " << barriers.size() << " bollard nodes, " << traffic_signals.size()
                << " traffic lights";

    std::vector<NodeBasedEdge> edge_list;
    util::loadEdgesFromFile(file_reader, edge_list);

    if (edge_list.empty())
    {
        throw util::exception("Node-based-graph (" + config.GetPath(".osrm").string() +
                              ") contains no edges." + SOURCE_REF);
    }

    return util::NodeBasedDynamicGraphFromEdges(number_of_node_based_nodes, edge_list);
}

/**
 \brief Building an edge-expanded graph from node-based input and turn restrictions
*/
std::pair<std::size_t, EdgeID>
Extractor::BuildEdgeExpandedGraph(ScriptingEnvironment &scripting_environment,
                                  std::vector<util::Coordinate> &coordinates,
                                  extractor::PackedOSMIDs &osm_node_ids,
                                  EdgeBasedNodeDataContainer &edge_based_nodes_container,
                                  std::vector<EdgeBasedNodeSegment> &edge_based_node_segments,
                                  std::vector<bool> &node_is_startpoint,
                                  std::vector<EdgeWeight> &edge_based_node_weights,
                                  util::DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list,
                                  const std::string &intersection_class_output_file,
                                  std::vector<TurnRestriction> &turn_restrictions,
                                  guidance::LaneDescriptionMap &turn_lane_map)
{
    std::unordered_set<NodeID> barrier_nodes;
    std::unordered_set<NodeID> traffic_signals;

    auto node_based_graph =
        LoadNodeBasedGraph(barrier_nodes, traffic_signals, coordinates, osm_node_ids);

    CompressedEdgeContainer compressed_edge_container;
    GraphCompressor graph_compressor;
    graph_compressor.Compress(barrier_nodes,
                              traffic_signals,
                              scripting_environment,
                              turn_restrictions,
                              *node_based_graph,
                              compressed_edge_container);

    turn_restrictions = removeInvalidRestrictions(std::move(turn_restrictions), *node_based_graph);

    util::NameTable name_table(config.GetPath(".osrm.names").string());

    EdgeBasedGraphFactory edge_based_graph_factory(node_based_graph,
                                                   compressed_edge_container,
                                                   barrier_nodes,
                                                   traffic_signals,
                                                   coordinates,
                                                   osm_node_ids,
                                                   scripting_environment.GetProfileProperties(),
                                                   name_table,
                                                   turn_lane_map);

    const auto create_edge_based_edges = [&]() {
        // scoped to relase intermediate datastructures right after the call
        RestrictionMap via_node_restriction_map(turn_restrictions);
        WayRestrictionMap via_way_restriction_map(turn_restrictions);
        turn_restrictions.clear();
        turn_restrictions.shrink_to_fit();

        edge_based_graph_factory.Run(scripting_environment,
                                     config.GetPath(".osrm.edges").string(),
                                     config.GetPath(".osrm.tld").string(),
                                     config.GetPath(".osrm.turn_weight_penalties").string(),
                                     config.GetPath(".osrm.turn_duration_penalties").string(),
                                     config.GetPath(".osrm.turn_penalties_index").string(),
                                     config.GetPath(".osrm.cnbg_to_ebg").string(),
                                     via_node_restriction_map,
                                     via_way_restriction_map);
        return edge_based_graph_factory.GetNumberOfEdgeBasedNodes();
    };

    const auto number_of_edge_based_nodes = create_edge_based_edges();

    compressed_edge_container.PrintStatistics();

    // The osrm-partition tool requires the compressed node based graph with an embedding.
    //
    // The `Run` function above re-numbers non-reverse compressed node based graph edges
    // to a continuous range so that the nodes in the edge based graph are continuous.
    //
    // Luckily node based node ids still coincide with the coordinate array.
    // That's the reason we can only here write out the final compressed node based graph.

    // Dumps to file asynchronously and makes sure we wait for its completion.
    std::future<void> compressed_node_based_graph_writing;

    BOOST_SCOPE_EXIT_ALL(&)
    {
        if (compressed_node_based_graph_writing.valid())
            compressed_node_based_graph_writing.wait();
    };

    compressed_node_based_graph_writing = std::async(std::launch::async, [&] {
        WriteCompressedNodeBasedGraph(
            config.GetPath(".osrm.cnbg").string(), *node_based_graph, coordinates);
    });

    {
        std::vector<std::uint32_t> turn_lane_offsets;
        std::vector<guidance::TurnLaneType::Mask> turn_lane_masks;
        std::tie(turn_lane_offsets, turn_lane_masks) =
            guidance::transformTurnLaneMapIntoArrays(turn_lane_map);
        files::writeTurnLaneDescriptions(
            config.GetPath(".osrm.tls"), turn_lane_offsets, turn_lane_masks);
    }
    files::writeSegmentData(config.GetPath(".osrm.geometry"),
                            *compressed_edge_container.ToSegmentData());

    edge_based_graph_factory.GetEdgeBasedEdges(edge_based_edge_list);
    edge_based_graph_factory.GetEdgeBasedNodes(edge_based_nodes_container);
    edge_based_graph_factory.GetEdgeBasedNodeSegments(edge_based_node_segments);
    edge_based_graph_factory.GetStartPointMarkers(node_is_startpoint);
    edge_based_graph_factory.GetEdgeBasedNodeWeights(edge_based_node_weights);

    const std::size_t number_of_node_based_nodes = node_based_graph->GetNumberOfNodes();

    util::Log() << "Writing Intersection Classification Data";
    TIMER_START(write_intersections);
    files::writeIntersections(
        intersection_class_output_file,
        IntersectionBearingsContainer{edge_based_graph_factory.GetBearingClassIds(),
                                      edge_based_graph_factory.GetBearingClasses()},
        edge_based_graph_factory.GetEntryClasses());
    TIMER_STOP(write_intersections);
    util::Log() << "ok, after " << TIMER_SEC(write_intersections) << "s";

    return std::make_pair(number_of_node_based_nodes, number_of_edge_based_nodes);
}

/**
    \brief Building rtree-based nearest-neighbor data structure

    Saves tree into '.ramIndex' and leaves into '.fileIndex'.
 */
void Extractor::BuildRTree(std::vector<EdgeBasedNodeSegment> edge_based_node_segments,
                           std::vector<bool> node_is_startpoint,
                           const std::vector<util::Coordinate> &coordinates)
{
    util::Log() << "Constructing r-tree of " << edge_based_node_segments.size()
                << " segments build on-top of " << coordinates.size() << " coordinates";

    BOOST_ASSERT(node_is_startpoint.size() == edge_based_node_segments.size());

    // Filter node based edges based on startpoint
    auto out_iter = edge_based_node_segments.begin();
    auto in_iter = edge_based_node_segments.begin();
    for (auto index : util::irange<std::size_t>(0UL, node_is_startpoint.size()))
    {
        BOOST_ASSERT(in_iter != edge_based_node_segments.end());
        if (node_is_startpoint[index])
        {
            *out_iter = *in_iter;
            out_iter++;
        }
        in_iter++;
    }
    auto new_size = out_iter - edge_based_node_segments.begin();
    if (new_size == 0)
    {
        throw util::exception("There are no snappable edges left after processing.  Are you "
                              "setting travel modes correctly in the profile?  Cannot continue." +
                              SOURCE_REF);
    }
    edge_based_node_segments.resize(new_size);

    TIMER_START(construction);
    util::StaticRTree<EdgeBasedNodeSegment> rtree(edge_based_node_segments,
                                                  config.GetPath(".osrm.ramIndex").string(),
                                                  config.GetPath(".osrm.fileIndex").string(),
                                                  coordinates);

    TIMER_STOP(construction);
    util::Log() << "finished r-tree construction in " << TIMER_SEC(construction) << " seconds";
}

void Extractor::WriteCompressedNodeBasedGraph(const std::string &path,
                                              const util::NodeBasedDynamicGraph &graph,
                                              const std::vector<util::Coordinate> &coordinates)
{
    const auto fingerprint = storage::io::FileWriter::GenerateFingerprint;

    storage::io::FileWriter writer{path, fingerprint};

    // Writes:  | Fingerprint | #e | #n | edges | coordinates |
    // - uint64: number of edges (from, to) pairs
    // - uint64: number of nodes and therefore also coordinates
    // - (uint32_t, uint32_t): num_edges * edges
    // - (int32_t, int32_t: num_nodes * coordinates (lon, lat)

    const auto num_edges = graph.GetNumberOfEdges();
    const auto num_nodes = graph.GetNumberOfNodes();

    BOOST_ASSERT_MSG(num_nodes == coordinates.size(), "graph and embedding out of sync");

    writer.WriteElementCount64(num_edges);
    writer.WriteElementCount64(num_nodes);

    // For all nodes iterate over its edges and dump (from, to) pairs
    for (const NodeID from_node : util::irange(0u, num_nodes))
    {
        for (const EdgeID edge : graph.GetAdjacentEdgeRange(from_node))
        {
            const auto to_node = graph.GetTarget(edge);

            writer.WriteOne(from_node);
            writer.WriteOne(to_node);
        }
    }

    // FIXME this is unneccesary: We have this data
    for (const auto &qnode : coordinates)
    {
        writer.WriteOne(qnode.lon);
        writer.WriteOne(qnode.lat);
    }
}

} // namespace extractor
} // namespace osrm
