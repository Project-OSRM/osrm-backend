#ifndef OSRM_EXTRACTOR_NODE_BASED_GRAPH_FACTORY_HPP_
#define OSRM_EXTRACTOR_NODE_BASED_GRAPH_FACTORY_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/node_based_edge.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/scripting_environment.hpp"

#include "traffic_signals.hpp"
#include "util/coordinate.hpp"
#include "util/node_based_graph.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace osrm::extractor
{

// Turn the output of the extraction process into a graph that represents junctions as nodes and
// ways as edges between these nodes. The graph forms the further input for OSRMs creation of the
// edge-based graph and is also the basic concept for annotating paths. All information from ways
// that is transferred into the API response is connected to the edges of the node-based graph.
//
// The input to the graph creation is the set of edges, traffic signals, barriers, meta-data,...
// which is generated during extraction and stored within the extraction containers.
class NodeBasedGraphFactory
{
  public:
    // The node-based graph factory transforms the graph data into the
    // node-based graph to represent the OSM network. This includes geometry compression, annotation
    // data optimisation and many other aspects. After this step, the edge-based graph factory can
    // turn the graph into the routing graph to be used with the navigation algorithms.
    NodeBasedGraphFactory(ScriptingEnvironment &scripting_environment,
                          std::vector<TurnRestriction> &turn_restrictions,
                          std::vector<UnresolvedManeuverOverride> &maneuver_overrides,
                          TrafficSignals &traffic_signals,
                          std::unordered_set<NodeID> &&barriers,
                          std::vector<util::Coordinate> &&coordinates,
                          extractor::PackedOSMIDs &&osm_node_ids,
                          const std::vector<NodeBasedEdge> &edge_list,
                          std::vector<NodeBasedEdgeAnnotation> &&annotation_data);

    auto const &GetGraph() const { return compressed_output_graph; }
    auto const &GetBarriers() const { return barriers; }
    auto const &GetCompressedEdges() const { return compressed_edge_container; }
    auto const &GetCoordinates() const { return coordinates; }
    auto const &GetAnnotationData() const { return annotation_data; }
    auto const &GetOsmNodes() const { return osm_node_ids; }
    auto &GetCompressedEdges() { return compressed_edge_container; }
    auto &GetCoordinates() { return coordinates; }
    auto &GetAnnotationData() { return annotation_data; }
    auto &GetOsmNodes() { return osm_node_ids; }

    // to reduce the memory footprint, the node-based graph factory allows releasing memory after it
    // might have been used for the last time:
    void ReleaseOsmNodes();

  private:
    // Build and validate compressed output graph
    void BuildCompressedOutputGraph(const std::vector<NodeBasedEdge> &edge_list);

    // Compress the node-based graph into a compact representation of itself. This removes storing a
    // single edge for every part of the geometry and might also combine meta-data for multiple
    // edges into a single representative form
    void Compress(ScriptingEnvironment &scripting_environment,
                  std::vector<TurnRestriction> &turn_restrictions,
                  std::vector<UnresolvedManeuverOverride> &maneuver_overrides,
                  TrafficSignals &traffic_signals);

    // Most ways are bidirectional, making the geometry in forward and backward direction the same,
    // except for reversal. We make use of this fact by keeping only one representation of the
    // geometry around
    void CompressGeometry();

    // After graph compression, some of the annotation entries might not be referenced anymore. We
    // compress the annotation data by relabeling the node-based graph references and removing all
    // unreferenced entries
    void CompressAnnotationData();

    // After produce, this will contain a compressed version of the node-based graph
    util::NodeBasedDynamicGraph compressed_output_graph;
    // To store the meta-data for the graph that is purely annotative / not used for the navigation
    // itself. Since the edges of a node-based graph form the nodes of the edge based graphs, we
    // transform this data into the EdgeBasedNodeDataContainer as output storage.
    std::vector<NodeBasedEdgeAnnotation> annotation_data;

    // General Information about the graph, not used outside of extractor
    std::unordered_set<NodeID> barriers;

    std::vector<util::Coordinate> coordinates;

    // data to keep in sync with the node-based graph
    extractor::PackedOSMIDs osm_node_ids;

    // for the compressed geometry
    extractor::CompressedEdgeContainer compressed_edge_container;
};

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_NODE_BASED_GRAPH_FACTORY_HPP_
