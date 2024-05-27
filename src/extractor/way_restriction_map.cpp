#include "extractor/way_restriction_map.hpp"

#include <tuple>
#include <utility>

namespace osrm::extractor
{

WayRestrictionMap::WayRestrictionMap(const RestrictionGraph &restriction_graph)
    : restriction_graph(restriction_graph)
{
}

std::size_t WayRestrictionMap::NumberOfDuplicatedNodes() const
{
    return restriction_graph.num_via_nodes;
}

bool WayRestrictionMap::IsViaWayEdge(const NodeID from, const NodeID to) const
{
    return restriction_graph.via_edge_to_node.contains({from, to});
}

std::vector<DuplicatedNodeID> WayRestrictionMap::DuplicatedNodeIDs(const NodeID from,
                                                                   const NodeID to) const
{
    const auto restriction_ways = restriction_graph.via_edge_to_node.equal_range({from, to});
    std::vector<DuplicatedNodeID> result;
    std::transform(restriction_ways.first,
                   restriction_ways.second,
                   std::back_inserter(result),
                   [](const auto &range_val) { return DuplicatedNodeID(range_val.second); });
    return result;
}

bool WayRestrictionMap::IsRestricted(DuplicatedNodeID duplicated_node, const NodeID to) const
{
    // Checks if a turn to 'to' is restricted
    BOOST_ASSERT(duplicated_node < restriction_graph.num_via_nodes);
    const auto &restrictions = restriction_graph.GetRestrictions(duplicated_node);
    return std::any_of(restrictions.begin(),
                       restrictions.end(),
                       [&to](const auto &restriction)
                       { return restriction->IsTurnRestricted(to); });
}

std::vector<const TurnRestriction *>
WayRestrictionMap::GetRestrictions(DuplicatedNodeID duplicated_node, const NodeID to) const
{
    std::vector<const TurnRestriction *> result;
    // Fetch all restrictions that will restrict a turn to 'to'.
    BOOST_ASSERT(duplicated_node < restriction_graph.num_via_nodes);
    const auto &restrictions = restriction_graph.GetRestrictions(duplicated_node);
    std::copy_if(restrictions.begin(),
                 restrictions.end(),
                 std::back_inserter(result),
                 [&to](const auto &restriction) { return restriction->IsTurnRestricted(to); });
    if (result.empty())
    {
        throw(
            "Asking for the restriction of an unrestricted turn. Check with `IsRestricted` before "
            "calling GetRestriction");
    }
    return result;
}

std::vector<WayRestrictionMap::ViaEdge> WayRestrictionMap::DuplicatedViaEdges() const
{
    std::vector<ViaEdge> result;
    result.resize(NumberOfDuplicatedNodes());

    // We use the node id from the restriction graph to enumerate all
    // duplicate nodes, and map them to their node based edge representation.
    // This means an node based edge (from,to) can have many duplicate nodes.
    for (auto entry : restriction_graph.via_edge_to_node)
    {
        result[entry.second] = {entry.first.first, entry.first.second};
    }
    return result;
}

NodeID WayRestrictionMap::RemapIfRestrictionStart(const NodeID edge_based_node,
                                                  const NodeID node_based_from,
                                                  const NodeID node_based_via,
                                                  const NodeID node_based_to,
                                                  const NodeID number_of_edge_based_nodes) const
{

    auto restriction_it =
        restriction_graph.start_edge_to_node.find({node_based_from, node_based_via});
    if (restriction_it != restriction_graph.start_edge_to_node.end())
    {
        for (const auto &edge : restriction_graph.GetEdges(restriction_it->second))
        {
            if (edge.node_based_to == node_based_to)
            {
                return number_of_edge_based_nodes - NumberOfDuplicatedNodes() + edge.target;
            }
        }
    }
    return edge_based_node;
}

NodeID WayRestrictionMap::RemapIfRestrictionVia(const NodeID edge_based_target_node,
                                                const NodeID edge_based_via_node,
                                                const NodeID node_based_to,
                                                const NodeID number_of_edge_based_nodes) const
{

    auto duplicated_node_id =
        edge_based_via_node + NumberOfDuplicatedNodes() - number_of_edge_based_nodes;
    BOOST_ASSERT(duplicated_node_id < restriction_graph.num_via_nodes);
    for (const auto &edge : restriction_graph.GetEdges(duplicated_node_id))
    {
        if (edge.node_based_to == node_based_to)
        {
            return number_of_edge_based_nodes - NumberOfDuplicatedNodes() + edge.target;
        }
    }
    return edge_based_target_node;
}

} // namespace osrm::extractor
