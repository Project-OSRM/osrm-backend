#ifndef OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_
#define OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_

#include <utility>
#include <vector>

// to access the turn restrictions
#include <boost/unordered_map.hpp>

#include "extractor/restriction.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

// Given the compressed representation of via-way turn restrictions, we provide a fast access into
// the restrictions to indicate which turns may be restricted due to a way in between
namespace osrm
{
namespace extractor
{

class WayRestrictionMap
{
  public:
    struct ViaWay
    {
        std::size_t id;
        NodeID from;
        NodeID to;
    };
    WayRestrictionMap(const std::vector<TurnRestriction> &turn_restrictions);

    // check if an edge between two nodes is a restricted turn. The check needs to be performed
    bool IsViaWay(const NodeID from, const NodeID to) const;

    // number of duplicated nodes
    std::size_t NumberOfDuplicatedNodes() const;

    // returns a representative for the duplicated way, consisting of the representative ID (first
    // ID of the nodes restrictions) and the from/to vertices of the via-way
    // This is used to construct edge based nodes that act as intermediate nodes.
    std::vector<ViaWay> DuplicatedNodeRepresentatives() const;

    // Access all duplicated NodeIDs for a set of nodes indicating a via way
    util::range<std::size_t> DuplicatedNodeIDs(const NodeID from, const NodeID to) const;

    // check whether a turn onto a given node is restricted, when coming from a duplicated node
    bool IsRestricted(std::size_t duplicated_node, const NodeID to) const;

    TurnRestriction const &GetRestriction(std::size_t) const;

    // changes edge_based_node to the correct duplicated_node_id in case node_based_from,
    // node_based_via, node_based_to can be identified with a restriction group
    NodeID RemapIfRestricted(const NodeID edge_based_node,
                             const NodeID node_based_from,
                             const NodeID node_based_via,
                             const NodeID node_based_to,
                             const NodeID number_of_edge_based_nodes) const;

  private:
    std::size_t AsDuplicatedNodeID(const std::size_t restriction_id) const;

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
    std::vector<std::size_t> duplicated_node_groups;

    boost::unordered_multimap<std::pair<NodeID, NodeID>, std::size_t> restriction_starts;
    boost::unordered_multimap<std::pair<NodeID, NodeID>, std::size_t> restriction_ends;

    std::vector<TurnRestriction> restriction_data;
};

} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_WAY_RESTRICTION_MAP_HPP_
