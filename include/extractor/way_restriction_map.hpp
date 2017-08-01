#ifndef OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_
#define OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_

#include <utility>
#include <vector>

// to access the turn restrictions
#include <boost/unordered_map.hpp>

#include "extractor/restriction.hpp"
#include "extractor/restriction_index.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

// Given the compressed representation of via-way turn restrictions, we provide a fast access into
// the restrictions to indicate which turns may be restricted due to a way in between
namespace osrm
{
namespace extractor
{

// The WayRestrictionMap uses ConditionalTurnRestrictions in general. Most restrictions will have
// empty conditions, though.
class WayRestrictionMap
{
  public:
    struct ViaWay
    {
        NodeID from;
        NodeID to;
    };
    WayRestrictionMap(const std::vector<ConditionalTurnRestriction> &conditional_restrictions);

    // Check if an edge between two nodes is a restricted turn. The check needs to be performed to
    // find duplicated nodes during the creation of edge-based-edges
    bool IsViaWay(const NodeID from, const NodeID to) const;

    // Every via-way results in a duplicated node that is required in the edge-based-graph. This
    // count is essentially the same as the number of valid via-way restrictions (except for
    // non-only restrictions that share the same in/via combination)
    std::size_t NumberOfDuplicatedNodes() const;

    // Returns a representative for each duplicated node, consisting of the representative ID (first
    // ID of the nodes restrictions) and the from/to vertices of the via-way
    // This is used to construct edge based nodes that act as intermediate nodes.
    std::vector<ViaWay> DuplicatedNodeRepresentatives() const;

    // Access all duplicated NodeIDs for a set of nodes indicating a via way
    std::vector<DuplicatedNodeID> DuplicatedNodeIDs(const NodeID from, const NodeID to) const;

    // check whether a turn onto a given node is restricted, when coming from a duplicated node
    bool IsRestricted(DuplicatedNodeID duplicated_node, const NodeID to) const;
    // Get the restriction resulting in ^ IsRestricted. Requires IsRestricted to evaluate to true
    const ConditionalTurnRestriction &GetRestriction(DuplicatedNodeID duplicated_node,
                                                     const NodeID to) const;

    // changes edge_based_node to the correct duplicated_node_id in case node_based_from,
    // node_based_via, node_based_to can be identified with a restriction group
    NodeID RemapIfRestricted(const NodeID edge_based_node,
                             const NodeID node_based_from,
                             const NodeID node_based_via,
                             const NodeID node_based_to,
                             const NodeID number_of_edge_based_nodes) const;

  private:
    DuplicatedNodeID AsDuplicatedNodeID(const RestrictionID restriction_id) const;

    // access all restrictions that have the same starting way and via way. Any duplicated node
    // represents the same in-way + via-way combination. This vector contains data about all
    // restrictions and their assigned duplicated nodes. It indicates the minimum restriciton ID
    // that is represented by the next node. The ID of a node is defined as the position of the
    // lower bound of the restrictions ID within this array
    //
    // a - b
    //     |
    // y - c - x
    //
    // restriction nodes | restriction id
    // a - b - c - x : 5
    // a - b - c - y : 6
    //
    //                    EBN: 0 . | 2 | 3 | 4 ...
    // duplicated node groups: ... | 5 | 7 | ...
    std::vector<DuplicatedNodeID> duplicated_node_groups;
    std::vector<ConditionalTurnRestriction> restriction_data;
    RestrictionIndex<ConditionalTurnRestriction> restriction_starts;
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_
