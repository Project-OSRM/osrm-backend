#include "extractor/restriction_map.hpp"

namespace osrm
{
namespace extractor
{

RestrictionMap::RestrictionMap(const std::vector<TurnRestriction> &restriction_list) : m_count(0)
{
    // decompose restriction consisting of a start, via and end node into a
    // a pair of starting edge and a list of all end nodes
    for (auto &restriction : restriction_list)
    {
        // This downcasting is OK because when this is called, the node IDs have been
        // renumbered into internal values, which should be well under 2^32
        // This will be a problem if we have more than 2^32 actual restrictions
        BOOST_ASSERT(restriction.from.node < std::numeric_limits<NodeID>::max());
        BOOST_ASSERT(restriction.via.node < std::numeric_limits<NodeID>::max());
        m_restriction_start_nodes.insert(restriction.from.node);
        m_no_turn_via_node_set.insert(restriction.via.node);

        // This explicit downcasting is also OK for the same reason.
        RestrictionSource restriction_source = {static_cast<NodeID>(restriction.from.node),
                                                static_cast<NodeID>(restriction.via.node)};

        std::size_t index;
        auto restriction_iter = m_restriction_map.find(restriction_source);
        if (restriction_iter == m_restriction_map.end())
        {
            index = m_restriction_bucket_list.size();
            m_restriction_bucket_list.resize(index + 1);
            m_restriction_map.emplace(restriction_source, index);
        }
        else
        {
            index = restriction_iter->second;
            // Map already contains an is_only_*-restriction
            if (m_restriction_bucket_list.at(index).begin()->is_only)
            {
                continue;
            }
            else if (restriction.flags.is_only)
            {
                // We are going to insert an is_only_*-restriction. There can be only one.
                m_count -= m_restriction_bucket_list.at(index).size();
                m_restriction_bucket_list.at(index).clear();
            }
        }
        ++m_count;
        BOOST_ASSERT(restriction.to.node < std::numeric_limits<NodeID>::max());
        m_restriction_bucket_list.at(index).emplace_back(restriction.to.node,
                                                         restriction.flags.is_only);
    }
}

bool RestrictionMap::IsViaNode(const NodeID node) const
{
    return m_no_turn_via_node_set.find(node) != m_no_turn_via_node_set.end();
}

// Replaces start edge (v, w) with (u, w). Only start node changes.
void RestrictionMap::FixupStartingTurnRestriction(const NodeID node_u,
                                                  const NodeID node_v,
                                                  const NodeID node_w)
{
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);
    BOOST_ASSERT(node_w != SPECIAL_NODEID);

    if (!IsSourceNode(node_v))
    {
        return;
    }

    const auto restriction_iterator = m_restriction_map.find({node_v, node_w});
    if (restriction_iterator != m_restriction_map.end())
    {
        const unsigned index = restriction_iterator->second;
        // remove old restriction start (v,w)
        m_restriction_map.erase(restriction_iterator);
        m_restriction_start_nodes.emplace(node_u);
        // insert new restriction start (u,w) (pointing to index)
        RestrictionSource new_source = {node_u, node_w};
        m_restriction_map.emplace(new_source, index);
    }
}

// Check if edge (u, v) is the start of any turn restriction.
// If so returns id of first target node.
NodeID RestrictionMap::CheckForEmanatingIsOnlyTurn(const NodeID node_u, const NodeID node_v) const
{
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);

    if (!IsSourceNode(node_u))
    {
        return SPECIAL_NODEID;
    }

    const auto restriction_iter = m_restriction_map.find({node_u, node_v});
    if (restriction_iter != m_restriction_map.end())
    {
        const unsigned index = restriction_iter->second;
        const auto &bucket = m_restriction_bucket_list.at(index);
        for (const RestrictionTarget &restriction_target : bucket)
        {
            if (restriction_target.is_only)
            {
                return restriction_target.target_node;
            }
        }
    }
    return SPECIAL_NODEID;
}

// Checks if turn <u,v,w> is actually a turn restriction.
bool RestrictionMap::CheckIfTurnIsRestricted(const NodeID node_u,
                                             const NodeID node_v,
                                             const NodeID node_w) const
{
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);
    BOOST_ASSERT(node_w != SPECIAL_NODEID);

    if (!IsSourceNode(node_u))
    {
        return false;
    }

    const auto restriction_iter = m_restriction_map.find({node_u, node_v});
    if (restriction_iter == m_restriction_map.end())
    {
        return false;
    }

    const unsigned index = restriction_iter->second;
    const auto &bucket = m_restriction_bucket_list.at(index);

    for (const RestrictionTarget &restriction_target : bucket)
    {
        if (node_w == restriction_target.target_node && // target found
            !restriction_target.is_only)                // and not an only_-restr.
        {
            return true;
        }
        // We could be tempted to check for `only` restrictions here as well. However, that check is
        // actually perfomed in intersection generation where we can also verify if the only
        // restriction is valid at all.
    }
    return false;
}

// check of node is the start of any restriction
bool RestrictionMap::IsSourceNode(const NodeID node) const
{
    return m_restriction_start_nodes.find(node) != m_restriction_start_nodes.end();
}
}
}
