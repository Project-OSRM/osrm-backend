/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "RestrictionMap.h"
#include "NodeBasedGraph.h"

#include "../Util/SimpleLogger.h"

bool RestrictionMap::IsNodeAViaNode(const NodeID node) const
{
    return m_no_turn_via_node_set.find(node) != m_no_turn_via_node_set.end();
}

RestrictionMap::RestrictionMap(const std::shared_ptr<NodeBasedDynamicGraph> &graph,
                               const std::vector<TurnRestriction> &input_restrictions_list)
    : m_count(0), m_graph(graph)
{
    // decompose restirction consisting of a start, via and end note into a start-edge
    // and all end-nodes
    for (auto &restriction : input_restrictions_list)
    {

        m_no_turn_via_node_set.insert(restriction.viaNode);

        std::pair<NodeID, NodeID> restriction_source =
            std::make_pair(restriction.fromNode, restriction.viaNode);

        unsigned index;
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
            if (m_restriction_bucket_list.at(index).begin()->second)
            {
                continue;
            }
            else if (restriction.flags.isOnly)
            {
                // We are going to insert an is_only_*-restriction. There can be only one.
                m_count -= m_restriction_bucket_list.at(index).size();
                m_restriction_bucket_list.at(index).clear();
            }
        }
        ++m_count;
        m_restriction_bucket_list.at(index)
            .push_back(std::make_pair(restriction.toNode, restriction.flags.isOnly));
    }
}

/**
 * Replace end v with w in each turn restriction containing u as via node
 *
 * Note: We need access to node based graph.
 */
void RestrictionMap::FixupArrivingTurnRestriction(const NodeID u, const NodeID v, const NodeID w)
{
    BOOST_ASSERT(u != std::numeric_limits<unsigned>::max());
    BOOST_ASSERT(v != std::numeric_limits<unsigned>::max());
    BOOST_ASSERT(w != std::numeric_limits<unsigned>::max());

    // find all possible start edges
    // it is more efficent to get a (small) list of potential start edges
    // than iterating over all buckets
    std::vector<NodeID> predecessors;
    for (EdgeID current_edge_id : m_graph->GetAdjacentEdgeRange(u))
    {
        const EdgeData &edge_data = m_graph->GetEdgeData(current_edge_id);
        const NodeID target = m_graph->GetTarget(current_edge_id);
        if (edge_data.backward && (v != target))
        {
            predecessors.push_back(target);
        }
    }

    for (const NodeID x : predecessors)
    {
        const std::pair<NodeID, NodeID> restr_start = std::make_pair(x, u);
        auto restriction_iterator = m_restriction_map.find(restr_start);
        if (restriction_iterator == m_restriction_map.end())
        {
            continue;
        }

        const unsigned index = restriction_iterator->second;
        auto &bucket = m_restriction_bucket_list.at(index);
        for (RestrictionTarget &restriction_target : bucket)
        {
            if (v == restriction_target.first)
            {
                restriction_target.first = w;
            }
        }
    }
}

/**
 * Replaces the start edge (v, w) with (u, w), only start node changes.
 */
void RestrictionMap::FixupStartingTurnRestriction(const NodeID u, const NodeID v, const NodeID w)
{
    BOOST_ASSERT(u != std::numeric_limits<unsigned>::max());
    BOOST_ASSERT(v != std::numeric_limits<unsigned>::max());
    BOOST_ASSERT(w != std::numeric_limits<unsigned>::max());

    const std::pair<NodeID, NodeID> old_start = std::make_pair(v, w);

    auto restriction_iterator = m_restriction_map.find(old_start);
    if (restriction_iterator != m_restriction_map.end())
    {
        const unsigned index = restriction_iterator->second;
        // remove old restriction start (v,w)
        m_restriction_map.erase(restriction_iterator);

        // insert new restriction start (u,w) (point to index)
        const std::pair<NodeID, NodeID> new_start = std::make_pair(u, w);
        m_restriction_map.insert(std::make_pair(new_start, index));
    }
}

/*
 * Check if the edge (u, v) is contained in any turn restriction.
 * If so returns id of first target node.
 */
NodeID RestrictionMap::CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const
{
    BOOST_ASSERT(u != std::numeric_limits<unsigned>::max());
    BOOST_ASSERT(v != std::numeric_limits<unsigned>::max());

    const std::pair<NodeID, NodeID> restriction_source = std::make_pair(u, v);
    auto restriction_iter = m_restriction_map.find(restriction_source);

    if (restriction_iter != m_restriction_map.end())
    {
        const unsigned index = restriction_iter->second;
        auto &bucket = m_restriction_bucket_list.at(index);
        for (const RestrictionSource &restriction_target : bucket)
        {
            if (restriction_target.second)
            {
                return restriction_target.first;
            }
        }
    }

    return std::numeric_limits<unsigned>::max();
}

/**
 * Checks if the turn described by start u, via v and targed w is covert by any turn restriction.
 */
bool RestrictionMap::CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const
{
    // SimpleLogger().Write(logDEBUG) << "checking turn <" << u << "," << v << "," << w << ">";
    BOOST_ASSERT(u != std::numeric_limits<unsigned>::max());
    BOOST_ASSERT(v != std::numeric_limits<unsigned>::max());
    BOOST_ASSERT(w != std::numeric_limits<unsigned>::max());

    const std::pair<NodeID, NodeID> restriction_source = std::make_pair(u, v);
    auto restriction_iter = m_restriction_map.find(restriction_source);

    if (restriction_iter != m_restriction_map.end())
    {
        const unsigned index = restriction_iter->second;
        auto &bucket = m_restriction_bucket_list.at(index);
        for (const RestrictionTarget &restriction_target : bucket)
        {
            if ((w == restriction_target.first) && // target found
                (!restriction_target.second)       // and not an only_-restr.
                )
            {
                return true;
            }
        }
    }
    return false;
}
