/*

Copyright (c) 2014, Project OSRM contributors
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

#ifndef GENERAL_TO_MANY_ROUTING_HPP
#define GENERAL_TO_MANY_ROUTING_HPP

#include "routing_base.hpp"
#include "../data_structures/search_engine_data.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

template <class DataFacadeT>
class general_many_to_many_routing final : public BasicRoutingInterface<DataFacadeT>
{
    using super = BasicRoutingInterface<DataFacadeT>;
    using many_to_many_heap = SearchEngineData::many_to_many_heap;
    SearchEngineData &engine_working_data;

    struct bucket_entry
    {
        unsigned target_id;
        EdgeWeight time;
        unsigned distance;
        explicit bucket_entry(const unsigned target_id, const EdgeWeight time, unsigned distance)
            : target_id(target_id), time(time), distance(distance)
        {
        }
    };
    using SearchSpaceWithBuckets = std::unordered_map<NodeID, std::vector<bucket_entry>>;

  public:
    struct distance_pair : public std::pair<EdgeWeight, unsigned>
    {
        distance_pair(EdgeWeight w, unsigned d) : std::pair<EdgeWeight, unsigned>(w, d) {}
        static distance_pair max()
        {
            return distance_pair{std::numeric_limits<EdgeWeight>::max(),
                                 std::numeric_limits<unsigned>::max()};
        }
    };

    general_many_to_many_routing(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    ~general_many_to_many_routing() {}

    std::shared_ptr<std::vector<distance_pair>>
    operator()(const PhantomNodeArray &phantom_source_nodes_array,
               const PhantomNodeArray &phantom_target_nodes_array) const
    {
        const auto number_of_sources = phantom_source_nodes_array.size();
        const auto number_of_targets = phantom_target_nodes_array.size();
        std::shared_ptr<std::vector<distance_pair>> result_table =
            std::make_shared<std::vector<distance_pair>>(number_of_sources * number_of_targets,
                                                         distance_pair::max());

        engine_working_data.initialize_general_m2m_heap(super::facade->GetNumberOfNodes());

        auto &query_heap = *(engine_working_data.general_m2m_heap);

        SearchSpaceWithBuckets search_space_with_buckets;

        unsigned target_id = 0;
        for (const auto &phantom_node_vector : phantom_target_nodes_array)
        {
            query_heap.Clear();
            // insert target(s) at distance 0

            for (const PhantomNode &phantom_node : phantom_node_vector)
            {
                if (SPECIAL_NODEID != phantom_node.forward_node_id)
                {
                    query_heap.Insert(phantom_node.forward_node_id,
                                      phantom_node.GetForwardWeightPlusOffset(),
                                      many_to_many_heapdata{phantom_node.forward_node_id, 0});
                }
                if (SPECIAL_NODEID != phantom_node.reverse_node_id)
                {
                    query_heap.Insert(phantom_node.reverse_node_id,
                                      phantom_node.GetReverseWeightPlusOffset(),
                                      many_to_many_heapdata{phantom_node.reverse_node_id, 0});
                }
            }

            // explore search space
            while (!query_heap.Empty())
            {
                reverse_routing_step(target_id, query_heap, search_space_with_buckets);
            }
            ++target_id;
        }

        // for each source do forward search
        unsigned source_id = 0;
        for (const std::vector<PhantomNode> &phantom_node_vector : phantom_source_nodes_array)
        {
            query_heap.Clear();
            for (const PhantomNode &phantom_node : phantom_node_vector)
            {
                // insert sources at distance 0
                if (SPECIAL_NODEID != phantom_node.forward_node_id)
                {
                    query_heap.Insert(phantom_node.forward_node_id,
                                      -phantom_node.GetForwardWeightPlusOffset(),
                                      many_to_many_heapdata{phantom_node.forward_node_id, 0});
                }
                if (SPECIAL_NODEID != phantom_node.reverse_node_id)
                {
                    query_heap.Insert(phantom_node.reverse_node_id,
                                      -phantom_node.GetReverseWeightPlusOffset(),
                                      many_to_many_heapdata{phantom_node.reverse_node_id, 0});
                }
            }

            // explore search space
            while (!query_heap.Empty())
            {
                forward_routing_step(source_id, number_of_targets, query_heap,
                                     search_space_with_buckets, result_table);
            }

            ++source_id;
        }
        BOOST_ASSERT(source_id == target_id);
        return result_table;
    }

    void forward_routing_step(const unsigned source_id,
                              const unsigned number_of_targets,
                              many_to_many_heap &query_heap,
                              const SearchSpaceWithBuckets &search_space_with_buckets,
                              std::shared_ptr<std::vector<distance_pair>> result_table) const
    {
        const NodeID node = query_heap.DeleteMin();
        const EdgeWeight source_time = query_heap.GetKey(node);
        const unsigned source_distance = query_heap.GetData(node).distance;

        // check if each encountered node has an entry
        const auto bucket_iterator = search_space_with_buckets.find(node);
        // iterate bucket if there exists one
        if (bucket_iterator != std::end(search_space_with_buckets))
        {
            const std::vector<bucket_entry> &bucket_list = bucket_iterator->second;
            for (const bucket_entry &current_bucket : bucket_list)
            {
                // get target id from bucket entry
                const unsigned target_id = current_bucket.target_id;
                const EdgeWeight target_time = current_bucket.time;
                const EdgeWeight current_time =
                    ((*result_table)[source_id * number_of_targets + target_id]).first;
                // check if new time is better
                const EdgeWeight new_time = source_time + target_time;
                if (new_time >= 0 && new_time < current_time)
                {
                    (*result_table)[source_id * number_of_targets + target_id] = {new_time,
                                                                                  source_distance};
                }
            }
        }
        if (stall_search<true>(node, source_time, query_heap))
        {
            return;
        }
        relax_edges<true>(node, source_time, source_distance, query_heap);
    }

    void reverse_routing_step(const unsigned target_id,
                              many_to_many_heap &query_heap,
                              SearchSpaceWithBuckets &search_space_with_buckets) const
    {
        const NodeID node = query_heap.DeleteMin();
        const EdgeWeight target_time = query_heap.GetKey(node);
        const unsigned target_distance = query_heap.GetData(node).distance;

        // store settled nodes in search space bucket
        search_space_with_buckets[node].emplace_back(target_id, target_time, target_distance);

        if (stall_search<false>(node, target_time, query_heap))
        {
            return;
        }

        relax_edges<false>(node, target_time, target_distance, query_heap);
    }

    template <bool forward_direction>
    void relax_edges(const NodeID node,
                     const EdgeWeight time,
                     unsigned target_distance,
                     many_to_many_heap &query_heap) const
    {
        for (const auto edge : super::facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = super::facade->GetEdgeData(edge);
            const bool direction_flag = (forward_direction ? data.forward : data.backward);
            if (direction_flag)
            {
                const NodeID to = super::facade->GetTarget(edge);
                const EdgeWeight edge_weight = data.distance;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const EdgeWeight to_time = time + edge_weight;
                const unsigned to_distance =
                    target_distance + get_unpacked_shortcut_euclidean_distance(node, to);

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!query_heap.WasInserted(to))
                {
                    query_heap.Insert(to, to_time, many_to_many_heapdata{node, to_distance});
                }
                // Found a shorter Path -> Update distance
                else if (to_time < query_heap.GetKey(to))
                {
                    // new parent
                    auto &data = query_heap.GetData(to);
                    data.parent = node;
                    data.distance = to_distance;

                    query_heap.DecreaseKey(to, to_time);
                }
            }
        }
    }

    // Stalling
    template <bool forward_direction>
    bool stall_search(const NodeID node, const EdgeWeight time, many_to_many_heap &query_heap) const
    {
        for (auto edge : super::facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = super::facade->GetEdgeData(edge);
            const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
            if (reverse_flag)
            {
                const NodeID to = super::facade->GetTarget(edge);
                const EdgeWeight edge_weight = data.distance;
                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                if (query_heap.WasInserted(to))
                {
                    if (query_heap.GetKey(to) + edge_weight < time)
                    {
                        return true;
                    }
                }
            }
        }
        return false;

        // NEW C++11 implementation. Needs testing
        // const auto edge_range = super::facade->GetAdjacentEdgeRange(node);
        // return std::any_of(std::begin(edge_range), std::end(edge_range), [&](const EdgeID edge){
        //     const auto &data = super::facade->GetEdgeData(edge);
        //     const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
        //     if (reverse_flag)
        //     {
        //         const NodeID to = super::facade->GetTarget(edge);
        //         const EdgeWeight edge_weight = data.distance;
        //         BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
        //         if (query_heap.WasInserted(to))
        //         {
        //             if (query_heap.GetKey(to) + edge_weight < distance)
        //             {
        //                 return true;
        //             }
        //         }
        //     }
        // });
    }

    unsigned get_unpacked_shortcut_euclidean_distance(const NodeID s, const NodeID t) const
    {

        unsigned euclidean_distance = 0;
        std::stack<std::pair<NodeID, NodeID>> recursion_stack;
        recursion_stack.emplace(s, t);

        std::pair<NodeID, NodeID> edge;
        while (!recursion_stack.empty())
        {
            edge = recursion_stack.top();
            recursion_stack.pop();

            EdgeID smaller_edge_id = SPECIAL_EDGEID;
            EdgeWeight edge_weight = std::numeric_limits<EdgeWeight>::max();
            for (const auto edge_id : super::facade->GetAdjacentEdgeRange(edge.first))
            {
                const EdgeWeight weight = super::facade->GetEdgeData(edge_id).distance;
                if ((super::facade->GetTarget(edge_id) == edge.second) && (weight < edge_weight) &&
                    super::facade->GetEdgeData(edge_id).forward)
                {
                    smaller_edge_id = edge_id;
                    edge_weight = weight;
                }
            }

            if (SPECIAL_EDGEID == smaller_edge_id)
            {
                for (const auto edge_id : super::facade->GetAdjacentEdgeRange(edge.second))
                {
                    const EdgeWeight weight = super::facade->GetEdgeData(edge_id).distance;
                    if ((super::facade->GetTarget(edge_id) == edge.first) &&
                        (weight < edge_weight) && super::facade->GetEdgeData(edge_id).backward)
                    {
                        smaller_edge_id = edge_id;
                        edge_weight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(edge_weight != std::numeric_limits<EdgeWeight>::max(),
                             "edge weight invalid");

            const auto &ed = super::facade->GetEdgeData(smaller_edge_id);
            if (ed.shortcut)
            { // unpack
                const NodeID middle_node_id = ed.id;
                // again, we need to this in reversed order
                recursion_stack.emplace(middle_node_id, edge.second);
                recursion_stack.emplace(edge.first, middle_node_id);
            }
            else
            {
                BOOST_ASSERT_MSG(!ed.shortcut, "edge must be shortcut");
                euclidean_distance += 1; // euclidean_distance(edge.first, edge.second);
            }
        }
        return euclidean_distance;
    }
};
#endif // GENERAL_TO_MANY_ROUTING_HPP
