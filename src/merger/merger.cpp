#include "extractor/class_data.hpp"
#include "extractor/compressed_node_based_graph_edge.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "extractor/extractor.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_relation.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/files.hpp"
#include "extractor/intersection_bearings_container.hpp"
#include "extractor/maneuver_override_relation_parser.hpp"
#include "extractor/node_based_graph_factory.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/restriction.hpp"
#include "extractor/restriction_filter.hpp"
#include "extractor/restriction_parser.hpp"
#include "extractor/tarjan_scc.hpp"

#include "guidance/files.hpp"
#include "guidance/segregated_intersection_classification.hpp"

#include "merger/merger.hpp"

#include "util/log.hpp"
#include "util/timing_util.hpp"

#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>

#if TBB_VERSION_MAJOR == 2020
#include <tbb/global_control.h>
#else
#include <tbb/task_scheduler_init.h>
#endif
#include <tbb/pipeline.h>

namespace osrm
{
namespace merger
{

namespace
{
/*
    Adds the class names to an accumulator if not present there. This function is used after each map is preprocessed
    in order to determine the union of classes from all the maps.
*/
void classNamesUnion(std::vector<std::string> &accumulator,
                     const std::vector<std::string> &class_names)
{
    for (const auto &name : class_names)
    {
        auto iter = std::find(accumulator.begin(), accumulator.end(), name);
        if (iter == accumulator.end())
        {
            accumulator.push_back(name);
        }
    }
}

/**
    Adds the excludable classes to an accumulator if not present there. This function is used after each map is preprocessed
    in order to determine the union of excludable classes from all the maps.
*/
void excludableClassesUnion(std::set<std::set<std::string>> &accumulator,
                             const std::vector<std::vector<std::string>> &excludable_classes)
{
    for (const auto &combination : excludable_classes)
    {
        std::set<std::string> combination_set(combination.begin(), combination.end());
        auto iter = accumulator.find(combination_set);
        if (iter == accumulator.end())
        {
            accumulator.insert(combination_set);
        }
    }
}

// Converts the class name map into a fixed mapping of index to name and update the profile properties.
void SetClassNames(const std::vector<std::string> &class_names,
                   extractor::ExtractorCallbacks::ClassesMap &classes_map,
                   extractor::ProfileProperties &profile_properties)
{
    // if we get a list of class names we can validate if we set invalid classes
    // and add classes that were never reference
    if (!class_names.empty())
    {
        // add class names that were never used explicitly on a way
        // this makes sure we can correctly validate unkown class names later
        for (const auto &name : class_names)
        {
            if (!extractor::isValidClassName(name))
            {
                throw util::exception("Invalid class name " + name + " only [a-Z0-9] allowed.");
            }

            auto iter = classes_map.find(name);
            if (iter == classes_map.end())
            {
                auto index = classes_map.size();
                // there is a hard limit on the number of classes
                if (index > extractor::MAX_CLASS_INDEX)
                {
                    throw util::exception("Maximum number of classes is " +
                                          std::to_string(extractor::MAX_CLASS_INDEX + 1));
                }

                classes_map[name] = extractor::getClassData(index);
            }
        }

        // check if class names are only from the list supplied by the user
        for (const auto &pair : classes_map)
        {
            auto iter = std::find(class_names.begin(), class_names.end(), pair.first);
            if (iter == class_names.end())
            {
                throw util::exception("Profile used unknown class name: " + pair.first);
            }
        }
    }

    // update the profile properties
    for (const auto &pair : classes_map)
    {
        auto range = extractor::getClassIndexes(pair.second);
        BOOST_ASSERT(range.size() == 1);
        profile_properties.SetClassName(range.front(), pair.first);
    }
}

// Converts the class name list to a mask list and update the profile properties.
void SetExcludableClasses(const extractor::ExtractorCallbacks::ClassesMap &classes_map,
                          const std::vector<std::vector<std::string>> &excludable_classes,
                          extractor::ProfileProperties &profile_properties)
{
    if (excludable_classes.size() > extractor::MAX_EXCLUDABLE_CLASSES)
    {
        throw util::exception("Only " + std::to_string(extractor::MAX_EXCLUDABLE_CLASSES) +
                              " excludable combinations allowed.");
    }

    // The exclude index 0 is reserve for not excludeing anything
    profile_properties.SetExcludableClasses(0, 0);

    std::size_t combination_index = 1;
    for (const auto &combination : excludable_classes)
    {
        extractor::ClassData mask = 0;
        for (const auto &name : combination)
        {
            auto iter = classes_map.find(name);
            if (iter == classes_map.end())
            {
                util::Log(logWARNING)
                    << "Unknown class name " + name + " in excludable combination. Ignoring.";
            }
            else
            {
                mask |= iter->second;
            }
        }

        if (mask > 0)
        {
            // update the profile properties with excludable classes
            profile_properties.SetExcludableClasses(combination_index++, mask);
        }
    }
}

// Convert a node-based graph to an edge list.
std::vector<extractor::CompressedNodeBasedGraphEdge> toEdgeList(const util::NodeBasedDynamicGraph &graph)
{
    std::vector<extractor::CompressedNodeBasedGraphEdge> edges;
    edges.reserve(graph.GetNumberOfEdges());

    // For all nodes iterate over its edges and dump (from, to) pairs
    for (const NodeID from_node : util::irange(0u, graph.GetNumberOfNodes()))
    {
        for (const EdgeID edge : graph.GetAdjacentEdgeRange(from_node))
        {
            const auto to_node = graph.GetTarget(edge);

            edges.push_back({from_node, to_node});
        }
    }

    return edges;
}
} // namespace

/**
 * This function is the entry point for the whole merge process. The goal of the merge
 * step is to filter and convert the OSM geometry of multiple maps with potentially different
 * profiles to something more fitting for routing.
 * That includes:
 *  - extracting turn restrictions
 *  - splitting ways into (directional!) edge segments
 *  - checking if nodes are barriers or traffic signal
 *  - discarding all tag information: All relevant type information for nodes/ways
 *    is extracted at this point.
 *
 * The result of this process are the following files:
 *  .names        : Names of all streets, stored as long consecutive string with prefix sum based index
 *  .osrm         : Nodes and edges in a intermediate format that easy to digest for osrm-contract
 *  .properties   : The profile properties
 *  .restrictions : Turn restrictions that are used by osrm-contract to construct the edge-expanded
 * graph
 *  .cnbg         : Compressed node-based graph edges
 *  .ebg          : Edge-based graph edge data with turns, distances, durations and weights
 *  .ebg_nodes    : Edge-based graph node data with node ids and annotation ids
 *  .turn*        : Contains turn duration and weight penalties
 *  etc
 */
int Merger::run()
{
    TIMER_START(extracting);

    const unsigned recommended_num_threads = std::thread::hardware_concurrency();
    const auto number_of_threads = std::min(recommended_num_threads, config.requested_num_threads);

#if TBB_VERSION_MAJOR == 2020
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism,
                           config.requested_num_threads);
#else
    tbb::task_scheduler_init init(config.requested_num_threads);
    BOOST_ASSERT(init.is_active());
#endif

    // Accumulators that will contain the merged graph data: nodes, edges, classes, street names, etc
    StringMap string_map;
    extractor::ExtractionContainers extraction_containers;
    extractor::ExtractorCallbacks::ClassesMap classes_map;
    extractor::LaneDescriptionMap turn_lane_map;
    std::vector<std::string> class_names;
    std::set<std::set<std::string>> excludable_classes_set;

    std::map<boost::filesystem::path, std::vector<boost::filesystem::path>>::iterator it = config.profile_to_input.begin();
    // scripting_environment_first will be used below after the merging as it contains the common profile information for all the graphs
    extractor::Sol2ScriptingEnvironment scripting_environment_first(it->first.string());
    parseOSMFiles(
        string_map,
        extraction_containers,
        classes_map,
        turn_lane_map,
        scripting_environment_first,
        it->first,
        number_of_threads,
        class_names,
        excludable_classes_set);
    it++;
    
    while (it != config.profile_to_input.end())
    {
        extractor::Sol2ScriptingEnvironment scripting_environment(it->first.string());
        parseOSMFiles(
            string_map,
            extraction_containers,
            classes_map,
            turn_lane_map,
            scripting_environment,
            it->first,
            number_of_threads,
            class_names,
            excludable_classes_set);
        it++;
    }

    // Union of the excludable classes
    std::vector<std::vector<std::string>> excludable_classes;
    for (const auto &combination_set : excludable_classes_set)
    {
        excludable_classes.push_back(std::vector<std::string>(combination_set.begin(), combination_set.end()));
    }

    // From this point on, the data is merged between the maps.

    writeTimestamp();
    // Use scripting_environment_first as profile accumulator
    writeOSMData(
        extraction_containers,
        classes_map,
        class_names,
        excludable_classes,
        scripting_environment_first);

    TIMER_STOP(extracting);
    util::Log() << "extraction finished after " << TIMER_SEC(extracting) << "s";

    std::vector<extractor::TurnRestriction> turn_restrictions = std::move(extraction_containers.turn_restrictions);
    std::vector<extractor::UnresolvedManeuverOverride> unresolved_maneuver_overrides = std::move(extraction_containers.internal_maneuver_overrides);

    // Transform the node-based graph that OSM is based on into an edge-based graph
    // that is better for routing.  Every edge becomes a node, and every valid
    // movement (e.g. turn from A->B, and B->A) becomes an edge
    util::Log() << "Generating edge-expanded graph representation";

    TIMER_START(expansion);

    // Containers for the edge-based graph
    extractor::EdgeBasedNodeDataContainer edge_based_nodes_container;
    std::vector<extractor::EdgeBasedNodeSegment> edge_based_node_segments;
    util::DeallocatingVector<extractor::EdgeBasedEdge> edge_based_edge_list;
    std::vector<EdgeWeight> edge_based_node_weights;
    std::vector<EdgeDuration> edge_based_node_durations;
    std::vector<EdgeDistance> edge_based_node_distances;
    std::uint32_t ebg_connectivity_checksum = 0;

    // Create a node-based graph from the OSRM file
    extractor::NodeBasedGraphFactory node_based_graph_factory(
        config.GetPath(".osrm"),
        scripting_environment_first,
        turn_restrictions,
        unresolved_maneuver_overrides);

    // Names referenced by annotations
    extractor::NameTable name_table;
    extractor::files::readNames(config.GetPath(".osrm.names"), name_table);

    util::Log() << "Find segregated edges in node-based graph ..." << std::flush;
    TIMER_START(segregated);

    auto segregated_edges = guidance::findSegregatedNodes(node_based_graph_factory, name_table);

    TIMER_STOP(segregated);
    util::Log() << "ok, after " << TIMER_SEC(segregated) << "s";
    util::Log() << "Segregated edges count = " << segregated_edges.size();

    util::Log() << "Writing nodes for nodes-based and edges-based graphs ...";
    const auto &coordinates = node_based_graph_factory.GetCoordinates();
    extractor::files::writeNodes(
        config.GetPath(".osrm.nbg_nodes"), coordinates, node_based_graph_factory.GetOsmNodes());
    node_based_graph_factory.ReleaseOsmNodes();

    const auto &node_based_graph = node_based_graph_factory.GetGraph();

    // The osrm-partition tool requires the compressed node based graph with an embedding.
    //
    // The `Run` function above re-numbers non-reverse compressed node based graph edges
    // to a continuous range so that the nodes in the edge based graph are continuous.
    //
    // Luckily node based node ids still coincide with the coordinate array.
    // That's the reason we can only here write out the final compressed node based graph.
    extractor::files::writeCompressedNodeBasedGraph(
        config.GetPath(".osrm.cnbg").string(),
        toEdgeList(node_based_graph));

    node_based_graph_factory.GetCompressedEdges().PrintStatistics();

    const auto &barrier_nodes = node_based_graph_factory.GetBarriers();
    const auto &traffic_signals = node_based_graph_factory.GetTrafficSignals();
    // stealing the annotation data from the node-based graph
    edge_based_nodes_container =
        extractor::EdgeBasedNodeDataContainer({}, std::move(node_based_graph_factory.GetAnnotationData()));

    turn_restrictions = extractor::removeInvalidRestrictions(std::move(turn_restrictions), node_based_graph);
    auto restriction_graph = extractor::constructRestrictionGraph(turn_restrictions);

    const auto number_of_node_based_nodes = node_based_graph.GetNumberOfNodes();

    const auto number_of_edge_based_nodes = BuildEdgeExpandedGraph(
            node_based_graph,
            coordinates,
            node_based_graph_factory.GetCompressedEdges(),
            barrier_nodes,
            traffic_signals,
            restriction_graph,
            segregated_edges,
            name_table,
            unresolved_maneuver_overrides,
            turn_lane_map,
            scripting_environment_first,
            edge_based_nodes_container,
            edge_based_node_segments,
            edge_based_node_weights,
            edge_based_node_durations,
            edge_based_node_distances,
            edge_based_edge_list,
            ebg_connectivity_checksum);

    ProcessGuidanceTurns(
        node_based_graph,
        edge_based_nodes_container,
        coordinates,
        node_based_graph_factory.GetCompressedEdges(),
        barrier_nodes,
        restriction_graph,
        name_table,
        std::move(turn_lane_map),
        scripting_environment_first);

    TIMER_STOP(expansion);

    // output the geometry of the node-based graph, needs to be done after the last usage, since it
    // destroys internal containers
    extractor::files::writeSegmentData(config.GetPath(".osrm.geometry"),
                            *node_based_graph_factory.GetCompressedEdges().ToSegmentData());

    util::Log() << "Saving edge-based node weights to file.";
    TIMER_START(timer_write_node_weights);
    extractor::files::writeEdgeBasedNodeWeightsDurationsDistances(config.GetPath(".osrm.enw"),
                                                                  edge_based_node_weights,
                                                                  edge_based_node_durations,
                                                                  edge_based_node_distances);
    TIMER_STOP(timer_write_node_weights);
    util::Log() << "Done writing. (" << TIMER_SEC(timer_write_node_weights) << ")";

    util::Log() << "Computing strictly connected components ...";
    FindComponents(
        number_of_edge_based_nodes,
        edge_based_edge_list,
        edge_based_node_segments,
        edge_based_nodes_container);

    util::Log() << "Building r-tree ...";
    TIMER_START(rtree);
    BuildRTree(std::move(edge_based_node_segments), coordinates);

    TIMER_STOP(rtree);

    extractor::files::writeNodeData(config.GetPath(".osrm.ebg_nodes"), edge_based_nodes_container);

    util::Log() << "Writing edge-based-graph edges       ... " << std::flush;
    TIMER_START(write_edges);
    extractor::files::writeEdgeBasedGraph(config.GetPath(".osrm.ebg"),
                               number_of_edge_based_nodes,
                               edge_based_edge_list,
                               ebg_connectivity_checksum);
    TIMER_STOP(write_edges);
    util::Log() << "ok, after " << TIMER_SEC(write_edges) << "s";

    util::Log() << "Processed " << edge_based_edge_list.size() << " edges";

    const auto nodes_per_second =
        static_cast<std::uint64_t>(number_of_node_based_nodes / TIMER_SEC(expansion));
    const auto edges_per_second =
        static_cast<std::uint64_t>((number_of_edge_based_nodes) / TIMER_SEC(expansion));

    util::Log() << "Expansion: " << nodes_per_second << " nodes/sec and " << edges_per_second
                << " edges/sec";
    util::Log() << "To prepare the data for routing, run: "
                << "./osrm-partition " << config.GetPath(".osrm");

    util::Log(logINFO) << "Merge is done!";
    return 0;
}

/**
    For a given profile and a list of OSM input files extract, filter and convert the
    OSM geometry for each input file to a format more fitting for routing. The preprocessed
    data is added to containers provided as function arguments that accumulate data from
    all the input files provided to osrm-merge.
*/
void Merger::parseOSMFiles(
    StringMap &string_map,
    extractor::ExtractionContainers &extraction_containers,
    extractor::ExtractorCallbacks::ClassesMap &classes_map,
    extractor::LaneDescriptionMap &turn_lane_map,
    extractor::ScriptingEnvironment &scripting_environment,
    const boost::filesystem::path profile_path,
    const unsigned number_of_threads,
    std::vector<std::string> &class_names,
    std::set<std::set<std::string>> &excludable_classes_set)
{
    for (const auto &input_file : config.profile_to_input[profile_path])
    {
        parseOSMData(
            string_map,
            extraction_containers,
            classes_map,
            turn_lane_map,
            scripting_environment,
            input_file,
            profile_path,
            number_of_threads);
    }

    // scripting_environment contains the extracted class names and excludable classes
    classNamesUnion(class_names, scripting_environment.GetClassNames());
    excludableClassesUnion(excludable_classes_set, scripting_environment.GetExcludableClasses());
}

/**
    For a given profile and an OSM input file extract, filter and convert the OSM geometry
    to a format more fitting for routing. The preprocessed data is added to containers
    provided as function arguments that accumulate data from all the input files provided
    to osrm-merge.
*/
void Merger::parseOSMData(
    StringMap &string_map,
    extractor::ExtractionContainers &extraction_containers,
    extractor::ExtractorCallbacks::ClassesMap &classes_map,
    extractor::LaneDescriptionMap &turn_lane_map,
    extractor::ScriptingEnvironment &scripting_environment,
    const boost::filesystem::path input_path,
    const boost::filesystem::path profile_path,
    const unsigned number_of_threads)
{
    util::Log() << "Input file: " << input_path.filename().string();
    if (!profile_path.empty())
    {
        util::Log() << "Profile: " << profile_path.filename().string();
    }
    util::Log() << "Threads: " << number_of_threads;

    const osmium::io::File input_file(input_path.string());
    osmium::thread::Pool pool(number_of_threads);

    util::Log() << "Parsing in progress..";
    TIMER_START(parsing);

    { // Parse OSM header
        osmium::io::Reader reader(input_file, pool, osmium::osm_entity_bits::nothing);
        osmium::io::Header header = reader.header();

        std::string generator = header.get("generator");
        if (generator.empty())
        {
            generator = "unknown tool";
        }
        util::Log() << "input file generated by " << generator;
    }

    // Extraction containers and restriction parser
    auto extractor_callbacks =
        std::make_unique<extractor::ExtractorCallbacks>(string_map,
                                            extraction_containers,
                                            classes_map,
                                            turn_lane_map,
                                            scripting_environment.GetProfileProperties());

    // Get list of supported relation types
    auto relation_types = scripting_environment.GetRelations();
    std::sort(relation_types.begin(), relation_types.end());

    std::vector<std::string> restrictions = scripting_environment.GetRestrictions();
    // Setup restriction parser
    const extractor::RestrictionParser restriction_parser(
        scripting_environment.GetProfileProperties().use_turn_restrictions,
        config.parse_conditionals,
        restrictions);

    const extractor::ManeuverOverrideRelationParser maneuver_override_parser;

    // OSM data reader
    using SharedBuffer = std::shared_ptr<osmium::memory::Buffer>;
    struct ParsedBuffer
    {
        SharedBuffer buffer;
        std::vector<std::pair<const osmium::Node &, extractor::ExtractionNode>> resulting_nodes;
        std::vector<std::pair<const osmium::Way &, extractor::ExtractionWay>> resulting_ways;
        std::vector<std::pair<const osmium::Relation &, extractor::ExtractionRelation>> resulting_relations;
        std::vector<extractor::InputTurnRestriction> resulting_restrictions;
        std::vector<extractor::InputManeuverOverride> resulting_maneuver_overrides;
    };

    extractor::ExtractionRelationContainer relations;

    const auto buffer_reader = [](osmium::io::Reader &reader) {
        return tbb::filter_t<void, SharedBuffer>(
            tbb::filter::serial_in_order, [&reader](tbb::flow_control &fc) {
                if (auto buffer = reader.read())
                {
                    return std::make_shared<osmium::memory::Buffer>(std::move(buffer));
                }
                else
                {
                    fc.stop();
                    return SharedBuffer{};
                }
            });
    };

    // Node locations cache (assumes nodes are placed before ways)
    using osmium_index_type =
        osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
    using osmium_location_handler_type = osmium::handler::NodeLocationsForWays<osmium_index_type>;

    osmium_index_type location_cache;
    osmium_location_handler_type location_handler(location_cache);

    tbb::filter_t<SharedBuffer, SharedBuffer> location_cacher(
        tbb::filter::serial_in_order, [&location_handler](SharedBuffer buffer) {
            osmium::apply(buffer->begin(), buffer->end(), location_handler);
            return buffer;
        });

    // OSM elements Lua parser
    tbb::filter_t<SharedBuffer, ParsedBuffer> buffer_transformer(
        tbb::filter::parallel, [&](const SharedBuffer buffer) {
            ParsedBuffer parsed_buffer;
            parsed_buffer.buffer = buffer;
            scripting_environment.ProcessElements(*buffer,
                                                  restriction_parser,
                                                  maneuver_override_parser,
                                                  relations,
                                                  parsed_buffer.resulting_nodes,
                                                  parsed_buffer.resulting_ways,
                                                  parsed_buffer.resulting_restrictions,
                                                  parsed_buffer.resulting_maneuver_overrides);
            return parsed_buffer;
        });

    // Parsed nodes and ways handler
    unsigned number_of_nodes = 0;
    unsigned number_of_ways = 0;
    unsigned number_of_restrictions = 0;
    unsigned number_of_maneuver_overrides = 0;
    tbb::filter_t<ParsedBuffer, void> buffer_storage(
        tbb::filter::serial_in_order, [&](const ParsedBuffer &parsed_buffer) {
            number_of_nodes += parsed_buffer.resulting_nodes.size();
            // put parsed objects thru extractor callbacks
            for (const auto &result : parsed_buffer.resulting_nodes)
            {
                extractor_callbacks->ProcessNode(result.first, result.second);
            }
            number_of_ways += parsed_buffer.resulting_ways.size();
            for (const auto &result : parsed_buffer.resulting_ways)
            {
                extractor_callbacks->ProcessWay(result.first, result.second);
            }

            number_of_restrictions += parsed_buffer.resulting_restrictions.size();
            for (const auto &result : parsed_buffer.resulting_restrictions)
            {
                extractor_callbacks->ProcessRestriction(result);
            }

            number_of_maneuver_overrides = parsed_buffer.resulting_maneuver_overrides.size();
            for (const auto &result : parsed_buffer.resulting_maneuver_overrides)
            {
                extractor_callbacks->ProcessManeuverOverride(result);
            }
        });

    tbb::filter_t<SharedBuffer, std::shared_ptr<extractor::ExtractionRelationContainer>> buffer_relation_cache(
        tbb::filter::parallel, [&](const SharedBuffer buffer) {
            if (!buffer)
                return std::shared_ptr<extractor::ExtractionRelationContainer>{};

            auto relations = std::make_shared<extractor::ExtractionRelationContainer>();
            for (auto entity = buffer->cbegin(), end = buffer->cend(); entity != end; ++entity)
            {
                if (entity->type() != osmium::item_type::relation)
                    continue;

                const auto &rel = static_cast<const osmium::Relation &>(*entity);

                const char *rel_type = rel.get_value_by_key("type");
                if (!rel_type || !std::binary_search(relation_types.begin(),
                                                     relation_types.end(),
                                                     std::string(rel_type)))
                    continue;

                extractor::ExtractionRelation extracted_rel({rel.id(), osmium::item_type::relation});
                for (const auto &t : rel.tags())
                    extracted_rel.attributes.emplace_back(std::make_pair(t.key(), t.value()));

                for (const auto &m : rel.members())
                {
                    extractor::ExtractionRelation::OsmIDTyped const mid(m.ref(), m.type());
                    extracted_rel.AddMember(mid, m.role());
                    relations->AddRelationMember(extracted_rel.id, mid);
                }

                relations->AddRelation(std::move(extracted_rel));
            };
            return relations;
        });

    unsigned number_of_relations = 0;
    tbb::filter_t<std::shared_ptr<extractor::ExtractionRelationContainer>, void> buffer_storage_relation(
        tbb::filter::serial_in_order,
        [&](const std::shared_ptr<extractor::ExtractionRelationContainer> parsed_relations) {
            number_of_relations += parsed_relations->GetRelationsNum();
            relations.Merge(std::move(*parsed_relations));
        });

    // Parse OSM elements with parallel transformer
    // Number of pipeline tokens that yielded the best speedup was about 1.5 * num_cores
    const auto num_threads = std::thread::hardware_concurrency() * 1.5;
    const auto read_meta =
        config.use_metadata ? osmium::io::read_meta::yes : osmium::io::read_meta::no;

    { // Relations reading pipeline
        util::Log() << "Parse relations ...";
        osmium::io::Reader reader(input_file, pool, osmium::osm_entity_bits::relation, read_meta);
        tbb::parallel_pipeline(
            num_threads, buffer_reader(reader) & buffer_relation_cache & buffer_storage_relation);
    }

    { // Nodes and ways reading pipeline
        util::Log() << "Parse ways and nodes ...";
        osmium::io::Reader reader(input_file,
                                  pool,
                                  osmium::osm_entity_bits::node | osmium::osm_entity_bits::way |
                                      osmium::osm_entity_bits::relation,
                                  read_meta);

        const auto pipeline =
            scripting_environment.HasLocationDependentData() && config.use_locations_cache
                ? buffer_reader(reader) & location_cacher & buffer_transformer & buffer_storage
                : buffer_reader(reader) & buffer_transformer & buffer_storage;
        tbb::parallel_pipeline(num_threads, pipeline);
    }

    TIMER_STOP(parsing);
    util::Log() << "Parsing finished after " << TIMER_SEC(parsing) << " seconds";

    util::Log() << "Raw input contains " << number_of_nodes << " nodes, " << number_of_ways
                << " ways, and " << number_of_relations << " relations, " << number_of_restrictions
                << " restrictions";

    extractor_callbacks.reset();

    if (extraction_containers.all_edges_list.empty())
    {
        throw util::exception(std::string("There are no edges remaining after parsing.") +
                              SOURCE_REF);
    }
}

// Write the timestamp data file
void Merger::writeTimestamp()
{
    std::string timestamp = config.data_version;

    extractor::files::writeTimestamp(config.GetPath(".osrm.timestamp").string(), timestamp);
    if (timestamp.empty())
    {
        timestamp = "n/a";
    }
    util::Log() << "timestamp: " << timestamp;
}

/**
    Write the initial data extracted from the OSM input files. This data contains:
    - graph data: nodes, edges, barriers, traffic lights and annotations
    - names that are referenced by annotations
    - profile properties
*/
void Merger::writeOSMData(
    extractor::ExtractionContainers &extraction_containers,
    extractor::ExtractorCallbacks::ClassesMap &classes_map,
    std::vector<std::string> &class_names,
    std::vector<std::vector<std::string>> &excludable_classes,
    extractor::ScriptingEnvironment &scripting_environment)
{
    extraction_containers.PrepareData(scripting_environment,
                                      config.GetPath(".osrm").string(),
                                      config.GetPath(".osrm.names").string());

    auto profile_properties = scripting_environment.GetProfileProperties();
    SetClassNames(class_names, classes_map, profile_properties);
    SetExcludableClasses(classes_map, excludable_classes, profile_properties);
    extractor::files::writeProfileProperties(config.GetPath(".osrm.properties").string(), profile_properties);
}

void Merger::FindComponents(unsigned number_of_edge_based_nodes,
                            const util::DeallocatingVector<extractor::EdgeBasedEdge> &input_edge_list,
                            const std::vector<extractor::EdgeBasedNodeSegment> &input_node_segments,
                            extractor::EdgeBasedNodeDataContainer &nodes_container) const
{
    using InputEdge = util::static_graph_details::SortableEdgeWithData<void>;
    using UncontractedGraph = util::StaticGraph<void>;
    std::vector<InputEdge> edges;
    edges.reserve(input_edge_list.size() * 2);

    for (const auto &edge : input_edge_list)
    {
        BOOST_ASSERT_MSG(static_cast<unsigned int>(std::max(edge.data.weight, 1)) > 0,
                         "edge distance < 1");
        BOOST_ASSERT(edge.source < number_of_edge_based_nodes);
        BOOST_ASSERT(edge.target < number_of_edge_based_nodes);
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
            BOOST_ASSERT(segment.forward_segment_id.id < number_of_edge_based_nodes);
            BOOST_ASSERT(segment.reverse_segment_id.id < number_of_edge_based_nodes);
            edges.push_back({segment.forward_segment_id.id, segment.reverse_segment_id.id});
            edges.push_back({segment.reverse_segment_id.id, segment.forward_segment_id.id});
        }
    }

    tbb::parallel_sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    auto uncontracted_graph = UncontractedGraph(number_of_edge_based_nodes, edges);

    extractor::TarjanSCC<UncontractedGraph> component_search(uncontracted_graph);
    component_search.Run();

    for (NodeID node_id = 0; node_id < number_of_edge_based_nodes; ++node_id)
    {
        const auto forward_component = component_search.GetComponentID(node_id);
        const auto component_size = component_search.GetComponentSize(forward_component);
        const auto is_tiny = component_size < config.small_component_size;
        BOOST_ASSERT(node_id < nodes_container.NumberOfNodes());
        nodes_container.nodes[node_id].component_id = {1 + forward_component, is_tiny};
    }
}

/**
 \brief Building an edge-expanded graph from node-based input and turn restrictions
*/
EdgeID Merger::BuildEdgeExpandedGraph(
    // input data
    const util::NodeBasedDynamicGraph &node_based_graph,
    const std::vector<util::Coordinate> &coordinates,
    const extractor::CompressedEdgeContainer &compressed_edge_container,
    const std::unordered_set<NodeID> &barrier_nodes,
    const std::unordered_set<NodeID> &traffic_signals,
    const extractor::RestrictionGraph &restriction_graph,
    const std::unordered_set<EdgeID> &segregated_edges,
    const extractor::NameTable &name_table,
    const std::vector<extractor::UnresolvedManeuverOverride> &maneuver_overrides,
    const extractor::LaneDescriptionMap &turn_lane_map,
    // for calculating turn penalties
    extractor::ScriptingEnvironment &scripting_environment,
    // output data
    extractor::EdgeBasedNodeDataContainer &edge_based_nodes_container,
    std::vector<extractor::EdgeBasedNodeSegment> &edge_based_node_segments,
    std::vector<EdgeWeight> &edge_based_node_weights,
    std::vector<EdgeDuration> &edge_based_node_durations,
    std::vector<EdgeDistance> &edge_based_node_distances,
    util::DeallocatingVector<extractor::EdgeBasedEdge> &edge_based_edge_list,
    std::uint32_t &connectivity_checksum)
{
    extractor::EdgeBasedGraphFactory edge_based_graph_factory(
        node_based_graph,
        edge_based_nodes_container,
        compressed_edge_container,
        barrier_nodes,
        traffic_signals,
        coordinates,
        name_table,
        segregated_edges,
        turn_lane_map);

    const auto create_edge_based_edges = [&]() {
        // scoped to release intermediate data structures right after the call
        extractor::RestrictionMap unconditional_node_restriction_map(restriction_graph);
        extractor::ConditionalRestrictionMap conditional_node_restriction_map(restriction_graph);
        extractor::WayRestrictionMap via_way_restriction_map(restriction_graph);
        edge_based_graph_factory.Run(scripting_environment,
                                     config.GetPath(".osrm.turn_weight_penalties").string(),
                                     config.GetPath(".osrm.turn_duration_penalties").string(),
                                     config.GetPath(".osrm.turn_penalties_index").string(),
                                     config.GetPath(".osrm.cnbg_to_ebg").string(),
                                     config.GetPath(".osrm.restrictions").string(),
                                     config.GetPath(".osrm.maneuver_overrides").string(),
                                     unconditional_node_restriction_map,
                                     conditional_node_restriction_map,
                                     via_way_restriction_map,
                                     maneuver_overrides);
        return edge_based_graph_factory.GetNumberOfEdgeBasedNodes();
    };

    const auto number_of_edge_based_nodes = create_edge_based_edges();

    edge_based_graph_factory.GetEdgeBasedEdges(edge_based_edge_list);
    edge_based_graph_factory.GetEdgeBasedNodeSegments(edge_based_node_segments);
    edge_based_graph_factory.GetEdgeBasedNodeWeights(edge_based_node_weights);
    edge_based_graph_factory.GetEdgeBasedNodeDurations(edge_based_node_durations);
    edge_based_graph_factory.GetEdgeBasedNodeDistances(edge_based_node_distances);
    connectivity_checksum = edge_based_graph_factory.GetConnectivityChecksum();

    return number_of_edge_based_nodes;
}

/**
    \brief Building rtree-based nearest-neighbor data structure

    Saves tree into '.ramIndex' and leaves into '.fileIndex'.
*/
void Merger::BuildRTree(std::vector<extractor::EdgeBasedNodeSegment> edge_based_node_segments,
                        const std::vector<util::Coordinate> &coordinates)
{
    util::Log() << "Constructing r-tree of " << edge_based_node_segments.size()
                << " segments build on-top of " << coordinates.size() << " coordinates";

    // Filter node based edges based on startpoint
    auto start_point_count = std::accumulate(edge_based_node_segments.begin(),
                                             edge_based_node_segments.end(),
                                             0,
                                             [](const size_t so_far, const auto &segment) {
                                                 return so_far + (segment.is_startpoint ? 1 : 0);
                                             });
    if (start_point_count == 0)
    {
        throw util::exception("There are no snappable edges left after processing.  Are you "
                              "setting travel modes correctly in the profile?  Cannot continue." +
                              SOURCE_REF);
    }

    TIMER_START(construction);
    util::StaticRTree<extractor::EdgeBasedNodeSegment> rtree(
        edge_based_node_segments, coordinates, config.GetPath(".osrm.fileIndex"));

    extractor::files::writeRamIndex(config.GetPath(".osrm.ramIndex"), rtree);

    TIMER_STOP(construction);
    util::Log() << "finished r-tree construction in " << TIMER_SEC(construction) << " seconds";
}

template <typename Map> auto convertIDMapToVector(const Map &map)
{
    std::vector<typename Map::key_type> result(map.size());
    for (const auto &pair : map)
    {
        BOOST_ASSERT(pair.second < map.size());
        result[pair.second] = pair.first;
    }
    return result;
}

void Merger::ProcessGuidanceTurns(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const extractor::EdgeBasedNodeDataContainer &edge_based_node_container,
    const std::vector<util::Coordinate> &node_coordinates,
    const extractor::CompressedEdgeContainer &compressed_edge_container,
    const std::unordered_set<NodeID> &barrier_nodes,
    const extractor::RestrictionGraph &restriction_graph,
    const extractor::NameTable &name_table,
    extractor::LaneDescriptionMap lane_description_map,
    extractor::ScriptingEnvironment &scripting_environment)
{
    // Output data
    osrm::guidance::TurnDataExternalContainer turn_data_container;
    util::guidance::LaneDataIdMap lane_data_map;
    osrm::guidance::BearingClassesVector bearing_class_by_node_based_node;
    osrm::guidance::BearingClassesMap bearing_class_hash;
    osrm::guidance::EntryClassesMap entry_class_hash;
    std::uint32_t connectivity_checksum = 0;

    TIMER_START(turn_annotations);

    {
        extractor::SuffixTable street_name_suffix_table(scripting_environment);
        const auto &turn_lanes_data = transformTurnLaneMapIntoArrays(lane_description_map);

        extractor::RestrictionMap unconditional_node_restriction_map(restriction_graph);
        extractor::WayRestrictionMap way_restriction_map(restriction_graph);

        osrm::guidance::annotateTurns(node_based_graph,
                                      edge_based_node_container,
                                      node_coordinates,
                                      compressed_edge_container,
                                      barrier_nodes,
                                      unconditional_node_restriction_map,
                                      way_restriction_map,
                                      name_table,
                                      street_name_suffix_table,
                                      turn_lanes_data,
                                      lane_description_map,
                                      lane_data_map,
                                      turn_data_container,
                                      bearing_class_by_node_based_node,
                                      bearing_class_hash,
                                      entry_class_hash,
                                      connectivity_checksum);
    }

    TIMER_STOP(turn_annotations);
    util::Log() << "Guidance turn annotations took " << TIMER_SEC(turn_annotations) << "s";

    util::Log() << "Writing Intersection Classification Data";
    TIMER_START(write_intersections);
    extractor::files::writeIntersections(
        config.GetPath(".osrm.icd").string(),
        extractor::IntersectionBearingsContainer{bearing_class_by_node_based_node,
                                      convertIDMapToVector(bearing_class_hash.data)},
        convertIDMapToVector(entry_class_hash.data));
    TIMER_STOP(write_intersections);
    util::Log() << "ok, after " << TIMER_SEC(write_intersections) << "s";

    util::Log() << "Writing Turns and Lane Data...";
    TIMER_START(write_guidance_data);

    {
        auto turn_lane_data = convertIDMapToVector(lane_data_map.data);
        extractor::files::writeTurnLaneData(config.GetPath(".osrm.tld"), turn_lane_data);
    }

    { // Turn lanes handler modifies lane_description_map, so another transformation is needed
        std::vector<std::uint32_t> turn_lane_offsets;
        std::vector<extractor::TurnLaneType::Mask> turn_lane_masks;
        std::tie(turn_lane_offsets, turn_lane_masks) =
            extractor::transformTurnLaneMapIntoArrays(lane_description_map);
        extractor::files::writeTurnLaneDescriptions(
            config.GetPath(".osrm.tls"), turn_lane_offsets, turn_lane_masks);
    }

    osrm::guidance::files::writeTurnData(
        config.GetPath(".osrm.edges").string(), turn_data_container, connectivity_checksum);
    TIMER_STOP(write_guidance_data);
    util::Log() << "ok, after " << TIMER_SEC(write_guidance_data) << "s";
}

} // namespace merger
} // namespace osrm
