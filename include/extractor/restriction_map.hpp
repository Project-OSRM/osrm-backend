#ifndef RESTRICTION_MAP_HPP
#define RESTRICTION_MAP_HPP

#include "extractor/edge_based_edge.hpp"
#include "extractor/restriction.hpp"
#include "util/std_hash.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace osrm
{
namespace extractor
{

struct RestrictionSource
{
    NodeID start_node;
    NodeID via_node;

    RestrictionSource(NodeID start, NodeID via) : start_node(start), via_node(via) {}

    friend inline bool operator==(const RestrictionSource &lhs, const RestrictionSource &rhs)
    {
        return (lhs.start_node == rhs.start_node && lhs.via_node == rhs.via_node);
    }
};

struct RestrictionTarget
{
    NodeID target_node;
    bool is_only;

    explicit RestrictionTarget(NodeID target, bool only) : target_node(target), is_only(only) {}

    friend inline bool operator==(const RestrictionTarget &lhs, const RestrictionTarget &rhs)
    {
        return (lhs.target_node == rhs.target_node && lhs.is_only == rhs.is_only);
    }
};
}
}

namespace std
{
template <> struct hash<osrm::extractor::RestrictionSource>
{
    size_t operator()(const osrm::extractor::RestrictionSource &r_source) const
    {
        return hash_val(r_source.start_node, r_source.via_node);
    }
};

template <> struct hash<osrm::extractor::RestrictionTarget>
{
    size_t operator()(const osrm::extractor::RestrictionTarget &r_target) const
    {
        return hash_val(r_target.target_node, r_target.is_only);
    }
};
}

namespace osrm
{
namespace extractor
{
/**
    \brief Efficent look up if an edge is the start + via node of a TurnRestriction
    EdgeBasedEdgeFactory decides by it if edges are inserted or geometry is compressed
*/
class RestrictionMap
{
  public:
    RestrictionMap() : m_count(0) {}
    RestrictionMap(const std::vector<TurnRestriction> &restriction_list);

    // Replace end v with w in each turn restriction containing u as via node
    template <class GraphT>
    void FixupArrivingTurnRestriction(const NodeID node_u,
                                      const NodeID node_v,
                                      const NodeID node_w,
                                      const GraphT &graph)
    {
        BOOST_ASSERT(node_u != SPECIAL_NODEID);
        BOOST_ASSERT(node_v != SPECIAL_NODEID);
        BOOST_ASSERT(node_w != SPECIAL_NODEID);

        if (!IsViaNode(node_u))
        {
            return;
        }

        // find all potential start edges. It is more efficent to get a (small) list
        // of potential start edges than iterating over all buckets
        std::vector<NodeID> predecessors;
        for (const EdgeID current_edge_id : graph.GetAdjacentEdgeRange(node_u))
        {
            const NodeID target = graph.GetTarget(current_edge_id);
            if (node_v != target)
            {
                predecessors.push_back(target);
            }
        }

        for (const NodeID node_x : predecessors)
        {
            const auto restriction_iterator = m_restriction_map.find({node_x, node_u});
            if (restriction_iterator == m_restriction_map.end())
            {
                continue;
            }

            const unsigned index = restriction_iterator->second;
            auto &bucket = m_restriction_bucket_list.at(index);

            for (RestrictionTarget &restriction_target : bucket)
            {
                if (node_v == restriction_target.target_node)
                {
                    restriction_target.target_node = node_w;
                }
            }
        }
    }

    bool IsViaNode(const NodeID node) const;

    // Replaces start edge (v, w) with (u, w). Only start node changes.
    void
    FixupStartingTurnRestriction(const NodeID node_u, const NodeID node_v, const NodeID node_w);

    // Check if edge (u, v) is the start of any turn restriction.
    // If so returns id of first target node.
    NodeID CheckForEmanatingIsOnlyTurn(const NodeID node_u, const NodeID node_v) const;
    // Checks if turn <u,v,w> is actually a turn restriction.
    bool
    CheckIfTurnIsRestricted(const NodeID node_u, const NodeID node_v, const NodeID node_w) const;

    std::size_t size() const { return m_count; }

  private:
    // check of node is the start of any restriction
    bool IsSourceNode(const NodeID node) const;

    using EmanatingRestrictionsVector = std::vector<RestrictionTarget>;

    std::size_t m_count;
    //! index -> list of (target, isOnly)
    std::vector<EmanatingRestrictionsVector> m_restriction_bucket_list;
    //! maps (start, via) -> bucket index
    std::unordered_map<RestrictionSource, unsigned> m_restriction_map;
    std::unordered_set<NodeID> m_restriction_start_nodes;
    std::unordered_set<NodeID> m_no_turn_via_node_set;
};
}
}

#endif // RESTRICTION_MAP_HPP
