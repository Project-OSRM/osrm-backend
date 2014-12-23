/*

Copyright (c) 2014, Project OSRM, Dennis Luxen, others
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

#ifndef TINY_COMPONENTS_HPP
#define TINY_COMPONENTS_HPP

#include "../typedefs.h"
#include "../data_structures/deallocating_vector.hpp"
#include "../data_structures/import_edge.hpp"
#include "../data_structures/query_node.hpp"
#include "../data_structures/percent.hpp"
#include "../data_structures/restriction.hpp"
#include "../data_structures/turn_instructions.hpp"

#include "../Util/integer_range.hpp"
#include "../Util/OSRMException.h"
#include "../Util/simple_logger.hpp"
#include "../Util/std_hash.hpp"
#include "../Util/timing_util.hpp"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

#include <tbb/parallel_sort.h>


#include <cstdint>

#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <typename GraphT>
class TarjanSCC
{
    struct TarjanStackFrame
    {
        explicit TarjanStackFrame(NodeID v, NodeID parent) : v(v), parent(parent) {}
        NodeID v;
        NodeID parent;
    };

    struct TarjanNode
    {
        TarjanNode() : index(SPECIAL_NODEID), low_link(SPECIAL_NODEID), on_stack(false) {}
        unsigned index;
        unsigned low_link;
        bool on_stack;
    };

    using RestrictionSource = std::pair<NodeID, NodeID>;
    using RestrictionTarget = std::pair<NodeID, bool>;
    using EmanatingRestrictionsVector = std::vector<RestrictionTarget>;
    using RestrictionMap = std::unordered_map<RestrictionSource, unsigned>;

    std::vector<EmanatingRestrictionsVector> m_restriction_bucket_list;
    std::vector<unsigned> components_index;     
    std::vector<NodeID> component_size_vector;
    std::shared_ptr<GraphT> m_node_based_graph;
    std::unordered_set<NodeID> barrier_node_list;
    unsigned size_one_counter;
    RestrictionMap m_restriction_map;

  public:
    TarjanSCC(std::shared_ptr<GraphT> graph,
              std::vector<NodeID> &bn,
              std::vector<TurnRestriction> &irs)
        : components_index(graph->GetNumberOfNodes(), SPECIAL_NODEID),
          m_node_based_graph(graph), 
          size_one_counter(0)
    {

        TIMER_START(SCC_LOAD);
        for (const TurnRestriction &restriction : irs)
        {
            std::pair<NodeID, NodeID> restriction_source = {restriction.from.node,
                                                            restriction.via.node};
            unsigned index = 0;
            const auto restriction_iterator = m_restriction_map.find(restriction_source);
            if (restriction_iterator == m_restriction_map.end())
            {
                index = m_restriction_bucket_list.size();
                m_restriction_bucket_list.resize(index + 1);
                m_restriction_map.emplace(restriction_source, index);
            }
            else
            {
                index = restriction_iterator->second;
                // Map already contains an is_only_*-restriction
                if (m_restriction_bucket_list.at(index).begin()->second)
                {
                    continue;
                }
                else if (restriction.flags.is_only)
                {
                    // We are going to insert an is_only_*-restriction. There can be only one.
                    m_restriction_bucket_list.at(index).clear();
                }
            }

            m_restriction_bucket_list.at(index)
                .emplace_back(restriction.to.node, restriction.flags.is_only);
        }

        barrier_node_list.insert(bn.begin(), bn.end());

        TIMER_STOP(SCC_LOAD);
        SimpleLogger().Write() << "Loading data into SCC took " << TIMER_MSEC(SCC_LOAD)/1000. << "s";
    }

    void Run()
    {
        TIMER_START(SCC_RUN);
        // The following is a hack to distinguish between stuff that happens
        // before the recursive call and stuff that happens after
        std::stack<TarjanStackFrame> recursion_stack;
        // true = stuff before, false = stuff after call
        std::stack<NodeID> tarjan_stack;
        std::vector<TarjanNode> tarjan_node_list(m_node_based_graph->GetNumberOfNodes());
        unsigned component_index = 0, size_of_current_component = 0;
        int index = 0;
        const NodeID last_node = m_node_based_graph->GetNumberOfNodes();
        std::vector<bool> processing_node_before_recursion(m_node_based_graph->GetNumberOfNodes(), true);
        for(const NodeID node : osrm::irange(0u, last_node))
        {
            if (SPECIAL_NODEID == components_index[node])
            {
                recursion_stack.emplace(TarjanStackFrame(node, node));
            }

            while (!recursion_stack.empty())
            {
                TarjanStackFrame currentFrame = recursion_stack.top();
                const NodeID v = currentFrame.v;
                recursion_stack.pop();
                const bool before_recursion = processing_node_before_recursion[v];

                if (before_recursion && tarjan_node_list[v].index != UINT_MAX)
                {
                    continue;
                }

                if (before_recursion)
                {
                    // Mark frame to handle tail of recursion
                    recursion_stack.emplace(currentFrame);
                    processing_node_before_recursion[v] = false;

                    // Mark essential information for SCC
                    tarjan_node_list[v].index = index;
                    tarjan_node_list[v].low_link = index;
                    tarjan_stack.push(v);
                    tarjan_node_list[v].on_stack = true;
                    ++index;

                    // Traverse outgoing edges
                    for (const auto current_edge : m_node_based_graph->GetAdjacentEdgeRange(v))
                    {
                        const auto vprime = m_node_based_graph->GetTarget(current_edge);
                        if (SPECIAL_NODEID == tarjan_node_list[vprime].index)
                        {
                            recursion_stack.emplace(TarjanStackFrame(vprime, v));
                        }
                        else
                        {
                            if (tarjan_node_list[vprime].on_stack &&
                                tarjan_node_list[vprime].index < tarjan_node_list[v].low_link)
                            {
                                tarjan_node_list[v].low_link = tarjan_node_list[vprime].index;
                            }
                        }
                    }
                }
                else
                {
                    processing_node_before_recursion[v] = true;
                    tarjan_node_list[currentFrame.parent].low_link =
                        std::min(tarjan_node_list[currentFrame.parent].low_link,
                                 tarjan_node_list[v].low_link);
                    // after recursion, lets do cycle checking
                    // Check if we found a cycle. This is the bottom part of the recursion
                    if (tarjan_node_list[v].low_link == tarjan_node_list[v].index)
                    {
                        NodeID vprime;
                        do
                        {
                            vprime = tarjan_stack.top();
                            tarjan_stack.pop();
                            tarjan_node_list[vprime].on_stack = false;
                            components_index[vprime] = component_index;
                            ++size_of_current_component;
                        } while (v != vprime);

                        component_size_vector.emplace_back(size_of_current_component);

                        if (size_of_current_component > 1000)
                        {
                            SimpleLogger().Write() << "large component [" << component_index
                                                   << "]=" << size_of_current_component;
                        }

                        ++component_index;
                        size_of_current_component = 0;
                    }
                }
            }
        }

        TIMER_STOP(SCC_RUN);
        SimpleLogger().Write() << "SCC run took: " << TIMER_MSEC(SCC_RUN)/1000. << "s";
        SimpleLogger().Write() << "identified: " << component_size_vector.size() << " many components";


        size_one_counter = std::count_if(component_size_vector.begin(),
                                         component_size_vector.end(),
                                         [](unsigned value)
                                         {
            return 1 == value;
        });

        SimpleLogger().Write() << "identified " << size_one_counter << " SCCs of size 1";

    }

    unsigned get_component_size(const NodeID node) const
    {
        return component_size_vector[components_index[node]];
    }

  private:
    unsigned CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const
    {
        std::pair<NodeID, NodeID> restriction_source = {u, v};
        const auto restriction_iterator = m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end())
        {
            const unsigned index = restriction_iterator->second;
            for (const RestrictionSource &restriction_target : m_restriction_bucket_list.at(index))
            {
                if (restriction_target.second)
                {
                    return restriction_target.first;
                }
            }
        }
        return SPECIAL_NODEID;
    }

    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const
    {
        // only add an edge if turn is not a U-turn except it is the end of dead-end street.
        std::pair<NodeID, NodeID> restriction_source = {u, v};
        const auto restriction_iterator = m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end())
        {
            const unsigned index = restriction_iterator->second;
            for (const RestrictionTarget &restriction_target : m_restriction_bucket_list.at(index))
            {
                if (w == restriction_target.first)
                {
                    return true;
                }
            }
        }
        return false;
    }
};

#endif /* TINY_COMPONENTS_HPP */
