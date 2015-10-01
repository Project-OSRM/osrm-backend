/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef SHORTEST_PATH_HPP
#define SHORTEST_PATH_HPP

#include <boost/assert.hpp>

#include "routing_base.hpp"
#include "../data_structures/search_engine_data.hpp"
#include "../util/integer_range.hpp"
#include "../typedefs.h"

template <class DataFacadeT>
class ShortestPathRouting final
    : public BasicRoutingInterface<DataFacadeT, ShortestPathRouting<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, ShortestPathRouting<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

  public:
    ShortestPathRouting(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    ~ShortestPathRouting() {}

    void operator()(const std::vector<PhantomNodes> &phantom_nodes_vector,
                    const std::vector<bool> &uturn_indicators,
                    InternalRouteResult &raw_route_data) const
    {
        int distance1 = 0;
        int distance2 = 0;
        bool search_from_1st_node = true;
        bool search_from_2nd_node = true;
        NodeID middle1 = SPECIAL_NODEID;
        NodeID middle2 = SPECIAL_NODEID;
        // path to the current forward node of the segment source
        std::vector<NodeID> packed_route1;
        // path to the current reverse node of the segment source
        std::vector<NodeID> packed_route2;
        std::vector<std::size_t> packed_leg_ends1;
        std::vector<std::size_t> packed_leg_ends2;

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());
        engine_working_data.InitializeOrClearSecondThreadLocalStorage(
            super::facade->GetNumberOfNodes());
        engine_working_data.InitializeOrClearThirdThreadLocalStorage(
            super::facade->GetNumberOfNodes());

        QueryHeap &forward_heap1 = *(engine_working_data.forward_heap_1);
        QueryHeap &reverse_heap1 = *(engine_working_data.reverse_heap_1);
        QueryHeap &forward_heap2 = *(engine_working_data.forward_heap_2);
        QueryHeap &reverse_heap2 = *(engine_working_data.reverse_heap_2);

        std::size_t current_leg = 0;
        // Get distance to next pair of target nodes.
        for (const PhantomNodes &phantom_node_pair : phantom_nodes_vector)
        {
            // heaps to search to the target forward node
            forward_heap1.Clear();
            forward_heap2.Clear();
            // heaps to search to the target reverse node
            reverse_heap1.Clear();
            reverse_heap2.Clear();
            int local_upper_bound1 = INVALID_EDGE_WEIGHT;
            int local_upper_bound2 = INVALID_EDGE_WEIGHT;

            middle1 = SPECIAL_NODEID;
            middle2 = SPECIAL_NODEID;

            const bool allow_u_turn = current_leg > 0 && uturn_indicators.size() > current_leg &&
                                      uturn_indicators[current_leg - 1];
            const EdgeWeight min_edge_offset =
                std::min(-phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                         -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset());

            // insert forward start node in both heaps
            if ((allow_u_turn || search_from_1st_node) &&
                phantom_node_pair.source_phantom.forward_node_id != SPECIAL_NODEID)
            {
                forward_heap1.Insert(
                    phantom_node_pair.source_phantom.forward_node_id,
                    -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                    phantom_node_pair.source_phantom.forward_node_id);
                forward_heap2.Insert(
                    phantom_node_pair.source_phantom.forward_node_id,
                    -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                    phantom_node_pair.source_phantom.forward_node_id);
            }

            // insert reverse start node in both heaps
            if ((allow_u_turn || search_from_2nd_node) &&
                phantom_node_pair.source_phantom.reverse_node_id != SPECIAL_NODEID)
            {
                forward_heap1.Insert(
                    phantom_node_pair.source_phantom.reverse_node_id,
                    -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset(),
                    phantom_node_pair.source_phantom.reverse_node_id);
                forward_heap2.Insert(
                    phantom_node_pair.source_phantom.reverse_node_id,
                    -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset(),
                    phantom_node_pair.source_phantom.reverse_node_id);
            }

            // insert forward node of target in reverse heap
            if (phantom_node_pair.target_phantom.forward_node_id != SPECIAL_NODEID)
            {
                reverse_heap1.Insert(phantom_node_pair.target_phantom.forward_node_id,
                                     phantom_node_pair.target_phantom.GetForwardWeightPlusOffset(),
                                     phantom_node_pair.target_phantom.forward_node_id);
            }

            // insert reverse node of target in reverse heap
            if (phantom_node_pair.target_phantom.reverse_node_id != SPECIAL_NODEID)
            {
                reverse_heap2.Insert(phantom_node_pair.target_phantom.reverse_node_id,
                                     phantom_node_pair.target_phantom.GetReverseWeightPlusOffset(),
                                     phantom_node_pair.target_phantom.reverse_node_id);
            }

            // run bi-directional Dijkstra routing step to forward target
            while (0 < (forward_heap1.Size() + reverse_heap1.Size()))
            {
                if (!forward_heap1.Empty())
                {
                    super::RoutingStep(forward_heap1, reverse_heap1, &middle1, &local_upper_bound1,
                                       min_edge_offset, true);
                }
                if (!reverse_heap1.Empty())
                {
                    super::RoutingStep(reverse_heap1, forward_heap1, &middle1, &local_upper_bound1,
                                       min_edge_offset, false);
                }
            }

            // run bi-directional Dijkstra routing step to reverse target
            if (!reverse_heap2.Empty())
            {
                while (0 < (forward_heap2.Size() + reverse_heap2.Size()))
                {
                    if (!forward_heap2.Empty())
                    {
                        super::RoutingStep(forward_heap2, reverse_heap2, &middle2,
                                           &local_upper_bound2, min_edge_offset, true);
                    }
                    if (!reverse_heap2.Empty())
                    {
                        super::RoutingStep(reverse_heap2, forward_heap2, &middle2,
                                           &local_upper_bound2, min_edge_offset, false);
                    }
                }
            }

            // No path found for both target nodes?
            if ((INVALID_EDGE_WEIGHT == local_upper_bound1) &&
                (INVALID_EDGE_WEIGHT == local_upper_bound2))
            {
                raw_route_data.shortest_path_length = INVALID_EDGE_WEIGHT;
                raw_route_data.alternative_path_length = INVALID_EDGE_WEIGHT;
                return;
            }

            // Was at most one of the two paths not found?
            BOOST_ASSERT_MSG((INVALID_EDGE_WEIGHT != local_upper_bound1 || INVALID_EDGE_WEIGHT != local_upper_bound2),
                             "no path found");

            // packed path from last forward/reverse via node to current forward node
            std::vector<NodeID> packed_sub_path1;
            // packed path from last forward/reverse via node to current reverse node
            std::vector<NodeID> packed_sub_path2;

            if (INVALID_EDGE_WEIGHT != local_upper_bound1)
            {
                super::RetrievePackedPathFromHeap(forward_heap1, reverse_heap1, middle1,
                                                  packed_sub_path1);
            }

            if (INVALID_EDGE_WEIGHT != local_upper_bound2)
            {
                super::RetrievePackedPathFromHeap(forward_heap2, reverse_heap2, middle2,
                                                  packed_sub_path2);
            }

            // if we allow uturns at this via it is optimal to select the shortest sub-path
            if (allow_u_turn)
            {
                if (local_upper_bound1 <= local_upper_bound2)
                {
                    local_upper_bound2 = local_upper_bound1;
                    packed_sub_path2 = packed_sub_path1;
                }
                else
                {
                    local_upper_bound1 = local_upper_bound2;
                    packed_sub_path1 = packed_sub_path2;
                }
            }

            // if we only use either one, don't do a full search on both
            search_from_1st_node = true;
            search_from_2nd_node = true;
            if (packed_sub_path1.empty())
            {
                packed_sub_path1 = packed_sub_path2;
                local_upper_bound1 = local_upper_bound2;
                search_from_1st_node = false;
            }
            if (packed_sub_path2.empty())
            {
                packed_sub_path2 = packed_sub_path1;
                local_upper_bound2 = local_upper_bound1;
                search_from_2nd_node = false;
            }

            // now we need to figure out if the reverse/forward node was chosen
            // there are two cases here
            // 1. both sub paths connect to the same previous path in which
            //    case we need to copy one
            // 2. both sub paths connect to different paths in which case
            //    we don't need to copy them
            if (!allow_u_turn && 0 < current_leg)
            {
                const NodeID previous_forward_id = packed_route1.back();
#ifndef NDEBUG
                const NodeID previous_reverse_id = packed_route2.back();
#endif

                BOOST_ASSERT(!packed_sub_path1.empty() && !packed_sub_path2.empty());

                const NodeID sub_path1_start = packed_sub_path1.front();
                const NodeID sub_path2_start = packed_sub_path2.front();

                // use the same previous path, we need to copy one
                if (sub_path1_start == sub_path2_start)
                {
                    if (sub_path1_start == previous_forward_id)
                    {
                        packed_route2 = packed_route1;
                        packed_leg_ends2 = packed_leg_ends1;
                        distance2 = distance1;
                    }
                    else
                    {
                        BOOST_ASSERT(sub_path1_start == previous_reverse_id);

                        packed_route1 = packed_route2;
                        packed_leg_ends1 = packed_leg_ends2;
                        distance1 = distance2;
                    }
                }
                else
                {
                    if (sub_path1_start != previous_forward_id)
                    {
                        BOOST_ASSERT(previous_forward_id == sub_path2_start && previous_reverse_id == sub_path1_start);
                        packed_route1.swap(packed_route2);
                        packed_leg_ends1.swap(packed_leg_ends1);
                        std::swap(distance1, distance2);
                    }
                }

                // this holds because we don't allow uturns
                BOOST_ASSERT(packed_route1.empty() || packed_sub_path1.front() == packed_route1.back());
                BOOST_ASSERT(packed_route2.empty() || packed_sub_path2.front() == packed_route2.back());
            }

            packed_route1.insert(packed_route1.end(), packed_sub_path1.begin(), packed_sub_path1.end());
            packed_leg_ends1.push_back(packed_route1.size());
            packed_route2.insert(packed_route2.end(), packed_sub_path2.begin(), packed_sub_path2.end());
            packed_leg_ends2.push_back(packed_route2.size());

            distance1 += local_upper_bound1;
            distance2 += local_upper_bound2;

            ++current_leg;
        }

        // chose the best overall route
        if (distance1 > distance2)
        {
            packed_route1.swap(packed_route2);
            packed_leg_ends1.swap(packed_leg_ends2);
            distance1 = distance2;
        }

        BOOST_ASSERT(!phantom_nodes_vector.empty());

        std::size_t start_index = 0;
        for (const std::size_t index : osrm::irange<std::size_t>(0, phantom_nodes_vector.size()))
        {
            std::size_t end_index = packed_leg_ends1[index];
            std::vector<NodeID> unpacked_leg;
            super::UnpackPath(packed_route1.begin() + start_index,
                              packed_route1.begin() + end_index,
                              unpacked_leg);

            auto source_traversed_in_reverse =
                packed_route1[start_index] != phantom_nodes_vector[index].source_phantom.forward_node_id;
            auto target_traversed_in_reverse =
                packed_route1[end_index-1] != phantom_nodes_vector[index].target_phantom.forward_node_id;

            PhantomNodes unpack_phantom_node_pair = phantom_nodes_vector[index];
            super::UncompressPath(unpacked_leg.begin(), unpacked_leg.end(),
                                  unpack_phantom_node_pair, source_traversed_in_reverse, target_traversed_in_reverse,
                                  raw_route_data.uncompressed_route);

            raw_route_data.segment_end_indices.push_back(raw_route_data.uncompressed_route.size());
            raw_route_data.source_traversed_in_reverse.push_back(source_traversed_in_reverse);
            raw_route_data.target_traversed_in_reverse.push_back(target_traversed_in_reverse);

            BOOST_ASSERT(end_index > 0);
            start_index = end_index;
        }
        raw_route_data.shortest_path_length = distance1;
    }
};

#endif /* SHORTEST_PATH_HPP */
