#ifndef OSRM_EXTRACTOR_NODE_RESTRICTION_MAP_HPP_
#define OSRM_EXTRACTOR_NODE_RESTRICTION_MAP_HPP_

#include "extractor/restriction.hpp"
#include "restriction_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/range/adaptor/filtered.hpp>

#include <unordered_map>
#include <utility>
#include <vector>

namespace osrm::extractor
{

// Allows easy check for whether a node restriction is present at a given intersection
template <typename RestrictionFilter> class NodeRestrictionMap
{
  public:
    NodeRestrictionMap(const RestrictionGraph &restriction_graph)
        : restriction_graph(restriction_graph), index_filter(RestrictionFilter{})
    {
    }

    // Find all restrictions applicable to (from,via,*) turns
    auto Restrictions(NodeID from, NodeID via) const
    {
        return getRange(from, via) | boost::adaptors::filtered(index_filter);
    };

    // Find all restrictions applicable to (from,via,to) turns
    auto Restrictions(NodeID from, NodeID via, NodeID to) const
    {
        const auto turnFilter = [this, to](const auto &restriction)
        { return index_filter(restriction) && restriction->IsTurnRestricted(to); };
        return getRange(from, via) | boost::adaptors::filtered(turnFilter);
    };

  private:
    RestrictionGraph::RestrictionRange getRange(NodeID from, NodeID via) const
    {
        const auto start_node_it = restriction_graph.start_edge_to_node.find({from, via});
        if (start_node_it != restriction_graph.start_edge_to_node.end())
        {
            return restriction_graph.GetRestrictions(start_node_it->second);
        }
        return {};
    }
    const RestrictionGraph &restriction_graph;
    const RestrictionFilter index_filter;
};

// Restriction filters to use as index defaults
struct ConditionalOnly
{
    bool operator()(const TurnRestriction *restriction) const
    {
        return !restriction->IsUnconditional();
    };
};

struct UnconditionalOnly
{
    bool operator()(const TurnRestriction *restriction) const
    {
        return restriction->IsUnconditional();
    };
};

using RestrictionMap = NodeRestrictionMap<UnconditionalOnly>;
using ConditionalRestrictionMap = NodeRestrictionMap<ConditionalOnly>;

} // namespace osrm::extractor

#endif // OSRM_EXTRACTOR_NODE_RESTRICTION_MAP_HPP_
