#ifndef OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_
#define OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_

#include "extractor/restriction.hpp"
#include "extractor/restriction_graph.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

// to access the turn restrictions
#include <unordered_map>
#include <utility>
#include <vector>

namespace osrm::extractor
{

// Given the compressed representation of via-way turn restrictions, we provide a fast access into
// the restrictions to indicate which turns may be restricted due to a way in between.
class WayRestrictionMap
{
  public:
    struct ViaEdge
    {
        NodeID from;
        NodeID to;
    };
    WayRestrictionMap(const RestrictionGraph &restriction_graph);

    // Check if an edge between two nodes is part of a via-way. The check needs to be performed to
    // find duplicated nodes during the creation of edge-based-edges
    bool IsViaWayEdge(NodeID from, NodeID to) const;

    // There is a bijection between restriction graph via-nodes and edge-based duplicated nodes.
    // This counts the number of duplicated nodes contributed by the way restrictions.
    std::size_t NumberOfDuplicatedNodes() const;

    // For each restriction graph via-node, we return its node-based edge representation (from,to).
    // This is used to create the duplicate node in the edge based graph.
    std::vector<ViaEdge> DuplicatedViaEdges() const;

    // Access all duplicated NodeIDs from the restriction graph via-node represented by (from,to)
    std::vector<DuplicatedNodeID> DuplicatedNodeIDs(NodeID from, NodeID to) const;

    // Check whether a turn onto a given node is restricted, when coming from a duplicated node
    bool IsRestricted(DuplicatedNodeID duplicated_node, NodeID to) const;

    // Get the restrictions resulting in ^ IsRestricted. Requires IsRestricted to evaluate to true
    std::vector<const TurnRestriction *> GetRestrictions(DuplicatedNodeID duplicated_node,
                                                         NodeID to) const;

    // Changes edge_based_node to the correct duplicated_node_id in case node_based_from,
    // node_based_via, node_based_to can be identified as the start of a way restriction.
    NodeID RemapIfRestrictionStart(NodeID edge_based_node,
                                   NodeID node_based_from,
                                   NodeID node_based_via,
                                   NodeID node_based_to,
                                   NodeID number_of_edge_based_nodes) const;

    // Changes edge_based_node to the correct duplicated_node_id in case node_based_from,
    // node_based_via, node_based_to can be identified as successive via steps of a way restriction.
    NodeID RemapIfRestrictionVia(NodeID edge_based_target_node,
                                 NodeID edge_based_via_node,
                                 NodeID node_based_to,
                                 NodeID number_of_edge_based_nodes) const;

  private:
    const RestrictionGraph &restriction_graph;
};

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_
