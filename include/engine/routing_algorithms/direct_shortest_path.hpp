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

#ifndef DIRECT_SHORTEST_PATH_HPP
#define DIRECT_SHORTEST_PATH_HPP

#include <boost/assert.hpp>
#include <iterator>

#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"
#include "util/integer_range.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

/// This is a striped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrainted
/// by the previous route.
/// This variation is only an optimazation for graphs with slow queries, for example
/// not fully contracted graphs.
template <class DataFacadeT>
class DirectShortestPathRouting final
    : public BasicRoutingInterface<DataFacadeT, DirectShortestPathRouting<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, DirectShortestPathRouting<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

  public:
    DirectShortestPathRouting(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    ~DirectShortestPathRouting() {}

    void operator()(const std::vector<PhantomNodes> &phantom_nodes_vector,
                    const std::vector<bool> &uturn_indicators,
                    InternalRouteResult &raw_route_data) const
    {
        (void)uturn_indicators; // unused

        // Get distance to next pair of target nodes.
        BOOST_ASSERT_MSG(1 == phantom_nodes_vector.size(),
                                         "Direct Shortest Path Query only accepts a single source and target pair. Multiple ones have been specified.");
        const auto& phantom_node_pair = phantom_nodes_vector.front();
        const auto& source_phantom = phantom_node_pair.source_phantom;
        const auto& target_phantom = phantom_node_pair.target_phantom;

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());
        QueryHeap &forward_heap = *(engine_working_data.forward_heap_1);
        QueryHeap &reverse_heap = *(engine_working_data.reverse_heap_1);
        forward_heap.Clear();
        reverse_heap.Clear();

        BOOST_ASSERT(source_phantom.is_valid());
        BOOST_ASSERT(target_phantom.is_valid());

        if (source_phantom.forward_node_id != SPECIAL_NODEID)
        {
            forward_heap.Insert(source_phantom.forward_node_id,
                                -source_phantom.GetForwardWeightPlusOffset(),
                                source_phantom.forward_node_id);
        }
        if (source_phantom.reverse_node_id != SPECIAL_NODEID)
        {
            forward_heap.Insert(source_phantom.reverse_node_id,
                                -source_phantom.GetReverseWeightPlusOffset(),
                                source_phantom.reverse_node_id);
        }

        if (target_phantom.forward_node_id != SPECIAL_NODEID)
        {
            reverse_heap.Insert(target_phantom.forward_node_id,
                                target_phantom.GetForwardWeightPlusOffset(),
                                target_phantom.forward_node_id);
        }

        if (target_phantom.reverse_node_id != SPECIAL_NODEID)
        {
            reverse_heap.Insert(target_phantom.reverse_node_id,
                                target_phantom.GetReverseWeightPlusOffset(),
                                target_phantom.reverse_node_id);
        }

        int distance = INVALID_EDGE_WEIGHT;
        std::vector<NodeID> packed_leg;

        if (super::facade->GetCoreSize() > 0)
        {
            engine_working_data.InitializeOrClearSecondThreadLocalStorage(
                super::facade->GetNumberOfNodes());
            QueryHeap &forward_core_heap = *(engine_working_data.forward_heap_2);
            QueryHeap &reverse_core_heap = *(engine_working_data.reverse_heap_2);
            forward_core_heap.Clear();
            reverse_core_heap.Clear();


            super::SearchWithCore(forward_heap, reverse_heap, forward_core_heap, reverse_core_heap,
                                  distance, packed_leg);
        }
        else
        {
            super::Search(forward_heap, reverse_heap, distance, packed_leg);
        }

        // No path found for both target nodes?
        if (INVALID_EDGE_WEIGHT == distance)
        {
            raw_route_data.shortest_path_length = INVALID_EDGE_WEIGHT;
            raw_route_data.alternative_path_length = INVALID_EDGE_WEIGHT;
            return;
        }

        BOOST_ASSERT_MSG(!packed_leg.empty(), "packed path empty");

        raw_route_data.shortest_path_length = distance;
        raw_route_data.unpacked_path_segments.resize(1);
        raw_route_data.source_traversed_in_reverse.push_back(
            (packed_leg.front() != phantom_node_pair.source_phantom.forward_node_id));
        raw_route_data.target_traversed_in_reverse.push_back(
            (packed_leg.back() != phantom_node_pair.target_phantom.forward_node_id));

        super::UnpackPath(packed_leg.begin(), packed_leg.end(), phantom_node_pair, raw_route_data.unpacked_path_segments.front());

    }
};

#endif /* DIRECT_SHORTEST_PATH_HPP */
