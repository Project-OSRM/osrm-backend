#ifndef EXTRACTION_CONTAINERS_HPP
#define EXTRACTION_CONTAINERS_HPP

#include "extractor/internal_extractor_edge.hpp"
#include "extractor/nodes_of_way.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction.hpp"
#include "extractor/scripting_environment.hpp"

#include "storage/tar_fwd.hpp"
#include "traffic_lights.hpp"
#include "traffic_signals.hpp"

#include <unordered_map>
#include <unordered_set>

namespace osrm::extractor
{

/**
 * Uses  memory containers to store all the data that
 * is collected by the extractor callbacks.
 *
 * The data is the filtered, aggregated and finally written to disk.
 */
class ExtractionContainers
{
    using ReferencedWays = std::unordered_map<OSMWayID, NodesOfWay>;
    using ReferencedTrafficSignals =
        std::pair<std::unordered_set<OSMNodeID>, std::unordered_multimap<OSMNodeID, OSMNodeID>>;
    // The relationship between way and nodes is lost during node preparation.
    // We identify the ways and nodes relevant to restrictions/overrides/signals prior to
    // node processing so that they can be referenced in the preparation phase.
    ReferencedWays IdentifyRestrictionWays();
    ReferencedWays IdentifyManeuverOverrideWays();
    ReferencedTrafficSignals IdentifyTrafficSignals();

    void PrepareNodes();
    void PrepareManeuverOverrides(const ReferencedWays &maneuver_override_ways);
    void PrepareRestrictions(const ReferencedWays &restriction_ways);
    void PrepareTrafficSignals(const ReferencedTrafficSignals &referenced_traffic_signals);
    void PrepareEdges(ScriptingEnvironment &scripting_environment);

    void WriteCharData(const std::string &file_name);

  public:
    using NodeIDVector = std::vector<OSMNodeID>;
    using NodeVector = std::vector<QueryNode>;
    using EdgeVector = std::vector<InternalExtractorEdge>;
    using AnnotationDataVector = std::vector<NodeBasedEdgeAnnotation>;
    using NameCharData = std::vector<unsigned char>;
    using NameOffsets = std::vector<size_t>;
    using WayIDVector = std::vector<OSMWayID>;
    using WayNodeIDOffsets = std::vector<size_t>;
    using InputTrafficSignal = std::pair<OSMNodeID, TrafficLightClass::Direction>;

    std::vector<OSMNodeID> barrier_nodes;
    NodeIDVector used_node_id_list;
    NodeVector all_nodes_list;
    EdgeVector all_edges_list;
    AnnotationDataVector all_edges_annotation_data_list;
    NameCharData name_char_data;
    NameOffsets name_offsets;
    WayIDVector ways_list;
    // Offsets into used nodes for each way_list entry
    WayNodeIDOffsets way_node_id_offsets;

    unsigned max_internal_node_id;

    std::vector<InputTrafficSignal> external_traffic_signals;
    TrafficSignals internal_traffic_signals;

    std::vector<NodeBasedEdge> used_edges;

    // List of restrictions (conditional and unconditional) before we transform them into the
    // output types. Input containers reference OSMNodeIDs. We can only transform them to the
    // correct internal IDs after we've read everything. Without a multi-parse approach,
    // we have to remember the output restrictions before converting them to the internal formats
    std::vector<InputTurnRestriction> restrictions_list;
    std::vector<TurnRestriction> turn_restrictions;

    std::vector<InputManeuverOverride> external_maneuver_overrides_list;
    std::vector<UnresolvedManeuverOverride> internal_maneuver_overrides;
    std::unordered_set<NodeID> used_barrier_nodes;
    NodeVector used_nodes;

    ExtractionContainers();

    void PrepareData(ScriptingEnvironment &scripting_environment,
                     const std::string &names_data_path);
};
} // namespace osrm::extractor

#endif /* EXTRACTION_CONTAINERS_HPP */
