#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/extractor_config.hpp"
#include "extractor/extractor_callbacks.hpp"
#include "extractor/extraction_containers.hpp"
#include "extractor/graph_compressor.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/restriction_graph.hpp"
#include "extractor/scripting_environment_lua.hpp"

#include "guidance/guidance_processing.hpp"
#include "guidance/turn_data_container.hpp"

#include "merger_config.hpp"

#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_lanes.hpp"

#include "util/typedefs.hpp"

namespace osrm
{
namespace merger
{
class Merger
{
  public:
    Merger(MergerConfig merger_config) : config(std::move(merger_config)) {}
    int run();
  private:
    using MapKey = std::tuple<std::string, std::string, std::string, std::string, std::string>;
    using MapVal = unsigned;
    using StringMap = std::unordered_map<MapKey, MapVal>;
    MergerConfig config;

    void parseOSMFiles(
        StringMap &string_map,
        extractor::ExtractionContainers &extraction_containers,
        extractor::ExtractorCallbacks::ClassesMap &classes_map,
        extractor::LaneDescriptionMap &turn_lane_map,
        extractor::ScriptingEnvironment &scripting_environment,
        const boost::filesystem::path profile_path,
        const unsigned number_of_threads,
        std::vector<std::string> &class_names,
        std::set<std::set<std::string>> &excludeable_classes_set);

    void parseOSMData(
        StringMap &string_map,
        extractor::ExtractionContainers &extraction_containers,
        extractor::ExtractorCallbacks::ClassesMap &classes_map,
        extractor::LaneDescriptionMap &turn_lane_map,
        extractor::ScriptingEnvironment &scripting_environment,
        const boost::filesystem::path input_path,
        const boost::filesystem::path profile_path,
        const unsigned number_of_threads);

    void writeTimestamp();

    void writeOSMData(
        extractor::ExtractionContainers &extraction_containers,
        extractor::ExtractorCallbacks::ClassesMap &classes_map,
        std::vector<std::string> &class_names,
        std::vector<std::vector<std::string>> &excludable_classes,
        extractor::ScriptingEnvironment &scripting_environment);

    EdgeID BuildEdgeExpandedGraph(
        // input data
        const util::NodeBasedDynamicGraph &node_based_graph,
        const std::vector<util::Coordinate> &coordinates,
        const extractor::CompressedEdgeContainer &compressed_edge_container,
        const std::unordered_set<NodeID> &barrier_nodes,
        const std::unordered_set<NodeID> &traffic_lights,
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
        std::uint32_t &connectivity_checksum);

    void FindComponents(
        unsigned max_edge_id,
        const util::DeallocatingVector<extractor::EdgeBasedEdge> &input_edge_list,
        const std::vector<extractor::EdgeBasedNodeSegment> &input_node_segments,
        extractor::EdgeBasedNodeDataContainer &nodes_container) const;
    
    void BuildRTree(
        std::vector<extractor::EdgeBasedNodeSegment> edge_based_node_segments,
        const std::vector<util::Coordinate> &coordinates);

    void ProcessGuidanceTurns(
        const util::NodeBasedDynamicGraph &node_based_graph,
        const extractor::EdgeBasedNodeDataContainer &edge_based_node_container,
        const std::vector<util::Coordinate> &node_coordinates,
        const extractor::CompressedEdgeContainer &compressed_edge_container,
        const std::unordered_set<NodeID> &barrier_nodes,
        const extractor::RestrictionGraph &restriction_graph,
        const extractor::NameTable &name_table,
        extractor::LaneDescriptionMap lane_description_map,
        extractor::ScriptingEnvironment &scripting_environment);
};
}
}
