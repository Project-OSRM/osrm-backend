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

#include "routing_base.hpp"
#include "../data_structures/search_engine_data.hpp"
#include "../util/integer_range.hpp"
#include "../util/timing_util.hpp"
#include "../typedefs.h"

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
        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());

        QueryHeap &forward_heap = *(engine_working_data.forward_heap_1);
        QueryHeap &reverse_heap = *(engine_working_data.reverse_heap_1);

        // Get distance to next pair of target nodes.
        BOOST_ASSERT_MSG(1 == phantom_nodes_vector.size(),
                                         "Direct Shortest Path Query only accepts a single source and target pair. Multiple ones have been specified.");

        const auto& phantom_node_pair = phantom_nodes_vector.front();

        forward_heap.Clear();
        reverse_heap.Clear();
        int distance = INVALID_EDGE_WEIGHT;
        NodeID middle = SPECIAL_NODEID;

        const EdgeWeight min_edge_offset =
            std::min(-phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                     -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset());

        // insert new starting nodes into forward heap, adjusted by previous distances.
        if (phantom_node_pair.source_phantom.forward_node_id != SPECIAL_NODEID)
        {
            forward_heap.Insert(
                phantom_node_pair.source_phantom.forward_node_id,
                -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                phantom_node_pair.source_phantom.forward_node_id);
        }
        if ( phantom_node_pair.source_phantom.reverse_node_id != SPECIAL_NODEID)
        {
            forward_heap.Insert(
                phantom_node_pair.source_phantom.reverse_node_id,
                -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset(),
                phantom_node_pair.source_phantom.reverse_node_id);
        }

        // insert new backward nodes into backward heap, unadjusted.
        if (phantom_node_pair.target_phantom.forward_node_id != SPECIAL_NODEID)
        {
            reverse_heap.Insert(phantom_node_pair.target_phantom.forward_node_id,
                                 phantom_node_pair.target_phantom.GetForwardWeightPlusOffset(),
                                 phantom_node_pair.target_phantom.forward_node_id);
        }

        if (phantom_node_pair.target_phantom.reverse_node_id != SPECIAL_NODEID)
        {
            reverse_heap.Insert(phantom_node_pair.target_phantom.reverse_node_id,
                                 phantom_node_pair.target_phantom.GetReverseWeightPlusOffset(),
                                 phantom_node_pair.target_phantom.reverse_node_id);
        }

        // run two-Target Dijkstra routing step.
        while (0 < (forward_heap.Size() + reverse_heap.Size()) )
        {
            if (!forward_heap.Empty())
            {
                super::RoutingStep(forward_heap, reverse_heap, &middle, &distance,
                                   min_edge_offset, true);
            }
            if (!reverse_heap.Empty())
            {
                super::RoutingStep(reverse_heap, forward_heap, &middle, &distance,
                                   min_edge_offset, false);
            }
        }

        // No path found for both target nodes?
        if ((INVALID_EDGE_WEIGHT == distance))
        {
            raw_route_data.shortest_path_length = INVALID_EDGE_WEIGHT;
            raw_route_data.alternative_path_length = INVALID_EDGE_WEIGHT;
            return;
        }

        // Was a paths over one of the forward/reverse nodes not found?
        BOOST_ASSERT_MSG((SPECIAL_NODEID == middle || INVALID_EDGE_WEIGHT != distance),
                         "no path found");

        // Unpack paths if they exist
        std::vector<NodeID> packed_leg;
        if (INVALID_EDGE_WEIGHT != distance)
        {
            super::RetrievePackedPathFromHeap(forward_heap, reverse_heap, middle, packed_leg);

            BOOST_ASSERT_MSG(!packed_leg.empty(), "packed path empty");

            raw_route_data.unpacked_path_segments.resize(1);
            raw_route_data.source_traversed_in_reverse.push_back(
                (packed_leg.front() != phantom_node_pair.source_phantom.forward_node_id));
            raw_route_data.target_traversed_in_reverse.push_back(
                (packed_leg.back() != phantom_node_pair.target_phantom.forward_node_id));

            super::UnpackPath(packed_leg, phantom_node_pair, raw_route_data.unpacked_path_segments.front());
        }

        raw_route_data.shortest_path_length = distance;
    }
};

#endif /* DIRECT_SHORTEST_PATH_HPP */
