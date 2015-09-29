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

#ifndef ROUTING_BASE_HPP
#define ROUTING_BASE_HPP

#include "../algorithms/coordinate_calculation.hpp"
#include "../data_structures/internal_route_result.hpp"
#include "../data_structures/search_engine_data.hpp"
#include "../data_structures/turn_instructions.hpp"
// #include "../util/simple_logger.hpp"

#include <boost/assert.hpp>

#include <stack>

SearchEngineData::SearchEngineHeapPtr SearchEngineData::forward_heap_1;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::reverse_heap_1;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forward_heap_2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::reverse_heap_2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forward_heap_3;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::reverse_heap_3;

template <class DataFacadeT, class Derived> class BasicRoutingInterface
{
  private:
    using EdgeData = typename DataFacadeT::EdgeData;

  protected:
    DataFacadeT *facade;

  public:
    BasicRoutingInterface() = delete;
    BasicRoutingInterface(const BasicRoutingInterface &) = delete;
    explicit BasicRoutingInterface(DataFacadeT *facade) : facade(facade) {}
    ~BasicRoutingInterface() {}

    void RoutingStep(SearchEngineData::QueryHeap &forward_heap,
                     SearchEngineData::QueryHeap &reverse_heap,
                     NodeID *middle_node_id,
                     int *upper_bound,
                     const int min_edge_offset,
                     const bool forward_direction) const
    {
        const NodeID node = forward_heap.DeleteMin();
        const int distance = forward_heap.GetKey(node);

        // const NodeID parentnode = forward_heap.GetData(node).parent;
        // SimpleLogger().Write() << (forward_direction ? "[fwd] " : "[rev] ") << "settled edge ("
        // << parentnode << "," << node << "), dist: " << distance;

        if (reverse_heap.WasInserted(node))
        {
            const int new_distance = reverse_heap.GetKey(node) + distance;
            if (new_distance < *upper_bound)
            {
                if (new_distance >= 0)
                {
                    *middle_node_id = node;
                    *upper_bound = new_distance;
                    //     SimpleLogger().Write() << "accepted middle node " << node << " at
                    //     distance " << new_distance;
                    // } else {
                    //     SimpleLogger().Write() << "discared middle node " << node << " at
                    //     distance " << new_distance;
                }
            }
        }

        if (distance + min_edge_offset > *upper_bound)
        {
            // SimpleLogger().Write() << "min_edge_offset: " << min_edge_offset;
            forward_heap.DeleteAll();
            return;
        }

        // Stalling
        for (const auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const EdgeData &data = facade->GetEdgeData(edge);
            const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
            if (reverse_flag)
            {
                const NodeID to = facade->GetTarget(edge);
                const int edge_weight = data.distance;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");

                if (forward_heap.WasInserted(to))
                {
                    if (forward_heap.GetKey(to) + edge_weight < distance)
                    {
                        return;
                    }
                }
            }
        }

        for (const auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const EdgeData &data = facade->GetEdgeData(edge);
            bool forward_directionFlag = (forward_direction ? data.forward : data.backward);
            if (forward_directionFlag)
            {

                const NodeID to = facade->GetTarget(edge);
                const int edge_weight = data.distance;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const int to_distance = distance + edge_weight;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!forward_heap.WasInserted(to))
                {
                    forward_heap.Insert(to, to_distance, node);
                }
                // Found a shorter Path -> Update distance
                else if (to_distance < forward_heap.GetKey(to))
                {
                    // new parent
                    forward_heap.GetData(to).parent = node;
                    forward_heap.DecreaseKey(to, to_distance);
                }
            }
        }
    }

    /// This function decompresses routes given as list of _edge based_ node ids.
    ///
    /// (0,1,2) and (1,2,3) form an edge based edge each and the node list we are going
    /// to get will represent (0,1)(1,2)(2,3).
    /// If all edges are uncompressedwe want to extract is (1, 2).
    ///           s
    ///       1-----0
    ///       |
    ///       |
    /// 3-----2
    ///     t
    ///
    /// If (0, 1), (1,2) and (2,3) are actually compressed that means we need to
    /// add all the segments inbetween:
    ///            s
    ///       1-b-a-0
    ///       |
    ///       c
    ///       |
    /// 3-e-d-2
    ///    t
    /// -> (a, b, 1, c, 2, d)
    template<typename ForwardIter>
    void UncompressPath(ForwardIter unpacked_path_begin, ForwardIter unpacked_path_end,
                        const PhantomNodes &phantom_node_pair,
                        std::vector<PathData> &uncompressed_path) const
    {
        BOOST_ASSERT(std::distance(unpacked_path_begin, unpacked_path_end) > 0);

        const bool source_traversed_in_reverse =
            (*unpacked_path_begin != phantom_node_pair.source_phantom.forward_node_id);
        const bool target_traversed_in_reverse =
            (*std::prev(unpacked_path_end) != phantom_node_pair.target_phantom.forward_node_id);

        BOOST_ASSERT(*unpacked_path_begin == phantom_node_pair.source_phantom.forward_node_id ||
                     *unpacked_path_begin == phantom_node_pair.source_phantom.reverse_node_id);
        BOOST_ASSERT(std::prev(unpacked_path_end) == phantom_node_pair.target_phantom.forward_node_id ||
                     std::prev(unpacked_path_end) == phantom_node_pair.target_phantom.reverse_node_id);

        bool same_edge = phantom_node_pair.source_phantom.forward_node_id == phantom_node_pair.target_phantom.forward_node_id;
        BOOST_ASSERT(!same_edge || phantom_node_pair.source_phantom.reverse_node_id == phantom_node_pair.target_phantom.reverse_node_id);

        // source and target are on the same compressed edge
        if (same_edge)
        {
            const auto edge_id = *unpacked_path_begin;

            unsigned name_index = phantom_node_pair.source_phantom.name_id;
            const TravelMode travel_mode = source_traversed_in_reverse ? phantom_node_pair.source_phantom.backward_travel_mode :
                                            phantom_node_pair.source_phantom.forward_travel_mode;
            const EdgeWeight duration = [&]() {
                if (source_traversed_in_reverse)
                {
                  BOOST_ASSERT(target_traversed_in_reverse);
                  return phantom_node_pair.source_phantom.GetForwardWeightPlusOffset() - phantom_node_pair.target_phantom.GetForwardWeightPlusOffset();
                }
                else
                {
                  BOOST_ASSERT(!target_traversed_in_reverse);
                  return phantom_node_pair.source_phantom.GetReverseWeightPlusOffset() - phantom_node_pair.target_phantom.GetReverseWeightPlusOffset();
                }
            }();

            if (facade->EdgeIsCompressed(edge_id))
            {
                std::vector<unsigned> uncompressed_node_ids;
                facade->GetUncompressedGeometry(facade->GetGeometryIndexForEdgeID(edge_id), uncompressed_node_ids);

                auto source_node_id = uncompressed_node_ids[phantom_node_pair.source_phantom.fwd_segment_position];
                uncompressed_path.emplace_back(source_node_id, phantom_node_pair.source_phantom.location,
                                               name_index, TurnInstruction::HeadOn, 0, travel_mode);
                if (source_traversed_in_reverse)
                {
                    // single compressed edge
                    // 0--->1--->2--->3--->4
                    //   t              s
                    // -> 3, 2, 1
                    for (int idx = phantom_node_pair.source_phantom.fwd_segment_position+1;
                         idx > phantom_node_pair.target_phantom.fwd_segment_position; idx--)
                    {
                        auto node_id = uncompressed_node_ids[idx];
                        uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id),
                                                       name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                }
                else
                {
                    // single compressed edge
                    // 0--->1--->2--->3--->4
                    //   s              t
                    // -> 1, 2, 3
                    for (int idx = phantom_node_pair.source_phantom.fwd_segment_position+1;
                         idx <= phantom_node_pair.target_phantom.fwd_segment_position; idx++)
                    {
                        auto node_id = uncompressed_node_ids[idx];
                        uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id),
                                                       name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                }

                auto target_node_id = uncompressed_node_ids[phantom_node_pair.target_phantom.fwd_segment_position];
                uncompressed_path.emplace_back(target_node_id, phantom_node_pair.target_phantom.location,
                                               name_index, TurnInstruction::ReachedYourDestination, duration, travel_mode);
            }
            else
            {
                uncompressed_path.emplace_back(facade->GetGeometryIndexForEdgeID(edge_id), phantom_node_pair.source_phantom.location,
                                               name_index, TurnInstruction::HeadOn, 0, travel_mode);
                uncompressed_path.emplace_back(facade->GetGeometryIndexForEdgeID(edge_id), phantom_node_pair.target_phantom.location,
                                               name_index, TurnInstruction::ReachedYourDestination, duration, travel_mode);
            }
        }
        else
        {
            for (auto iter = unpacked_path_begin; iter != unpacked_path_end; ++iter)
            {
                const auto edge_id = *iter;
                const auto data = facade->GetEdgeData(edge_id);
                const auto duration = data.distance;

                unsigned name_index = facade->GetNameIndexFromEdgeID(edge_id);
                const TurnInstruction turn_instruction = facade->GetTurnInstructionForEdgeID(edge_id);
                const TravelMode travel_mode = facade->GetTravelModeForEdgeID(edge_id);

                if (facade->EdgeIsCompressed(edge_id))
                {
                    std::vector<unsigned> uncompressed_node_ids;
                    facade->GetUncompressedGeometry(facade->GetGeometryIndexForEdgeID(edge_id),
                                                    uncompressed_node_ids);

                    const auto end_index = uncompressed_node_ids.size();
                    const auto begin_index = [&]() {
                        if (uncompressed_path.size() > 0)
                            return 0UL;

                        if (source_traversed_in_reverse)
                        {
                            return end_index - phantom_node_pair.source_phantom.fwd_segment_position - 1;
                        }
                        else
                        {
                            return static_cast<unsigned long>(phantom_node_pair.source_phantom.fwd_segment_position);
                        }
                    }();

                    BOOST_ASSERT(begin_index >= 0);
                    BOOST_ASSERT(begin_index <= end_index);
                    for (auto idx = begin_index; idx < end_index; ++idx)
                    {
                        auto node_id = uncompressed_node_ids[idx];
                        uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id),
                                                       name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                    uncompressed_path.back().turn_instruction = turn_instruction;
                    uncompressed_path.back().segment_duration = duration;
                }
                else
                {
                    auto node_id = facade->GetGeometryIndexForEdgeID(edge_id);
                    uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id), name_index,
                                               turn_instruction, duration, travel_mode);
                }
            }

            if (SPECIAL_EDGEID != phantom_node_pair.target_phantom.packed_geometry_id)
            {
                std::vector<unsigned> uncompressed_node_ids;
                facade->GetUncompressedGeometry(phantom_node_pair.target_phantom.packed_geometry_id,
                                                uncompressed_node_ids);

                const auto end_index = phantom_node_pair.target_phantom.fwd_segment_position;
                unsigned name_index = phantom_node_pair.target_phantom.name_id;
                auto target_node_id = uncompressed_node_ids[phantom_node_pair.target_phantom.fwd_segment_position];

                //         t
                //         <<<<<<<<
                // u--->x--->x--->v
                if (target_traversed_in_reverse)
                {
                    TravelMode travel_mode = phantom_node_pair.target_phantom.backward_travel_mode;
                    EdgeWeight duration = phantom_node_pair.target_phantom.reverse_offset;

                    BOOST_ASSERT(begin_index >= end_index);
                    for (auto idx = uncompressed_node_ids.size()-1; idx > end_index; --idx)
                    {
                        auto node_id = uncompressed_node_ids[idx];
                        uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id),
                                                       name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                    uncompressed_path.emplace_back(target_node_id, phantom_node_pair.target_phantom.location,
                                                   name_index, TurnInstruction::ReachedYourDestination, duration, travel_mode);
                }
                //         t
                // >>>>>>>>>
                // u--->x--->x--->v
                else
                {
                    TravelMode travel_mode = phantom_node_pair.target_phantom.forward_travel_mode;
                    EdgeWeight duration = phantom_node_pair.target_phantom.forward_weight;

                    BOOST_ASSERT(begin_index >= end_index);
                    for (auto idx = 0; idx < end_index; ++idx)
                    {
                        auto node_id = uncompressed_node_ids[idx];
                        uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id),
                                                       name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                    uncompressed_path.emplace_back(target_node_id, phantom_node_pair.target_phantom.location,
                                                   name_index, TurnInstruction::ReachedYourDestination, duration, travel_mode);
                }
            }
        }
    }

    /// Gets a list of node ids that potentially cover shortcuts. This recursively
    /// unpacks shortcuts until only simple edges are left.
    /// Returns a list of edge-based node ids.
    template<typename ForwardIter>
    void UnpackPath(ForwardIter packed_path_begin, ForwardIter packed_path_end,
                    std::vector<NodeID> &unpacked_path) const
    {
        BOOST_ASSERT(std::distance(packed_path_begin, packed_path_end) > 0);

        // handle local path on same edge, they are always unpacked
        if (std::distance(packed_path_begin, packed_path_end) < 2)
        {
            unpacked_path.emplace_back(*packed_path_begin);
            return;
        }

        std::stack<std::pair<NodeID, NodeID>> recursion_stack;

        // We have to push the path in reverse order onto the stack because it's LIFO.
        for (auto iter = std::prev(packed_path_end); iter != packed_path_begin; --iter)
        {
            recursion_stack.emplace(*std::prev(iter), *iter);
        }

        std::pair<NodeID, NodeID> edge;
        while (!recursion_stack.empty())
        {
            edge = recursion_stack.top();
            recursion_stack.pop();

            // facade->FindEdge does not suffice here in case of shortcuts.
            // The above explanation unclear? Think!
            EdgeID smaller_edge_id = facade->FindSmallestEdge(edge.first, edge.second);

            if (SPECIAL_EDGEID == smaller_edge_id)
            {
                smaller_edge_id = facade->FindSmallestEdge(edge.second, edge.first);
            }
            BOOST_ASSERT_MSG(smaller_edge_id != SPECIAL_EDGEID, "edge id invalid");

            const EdgeData &data = facade->GetEdgeData(smaller_edge_id);
            if (data.shortcut)
            {
                const NodeID middle_node_id = data.id;
                // again, we need to this in reversed order
                recursion_stack.emplace(middle_node_id, edge.second);
                recursion_stack.emplace(edge.first, middle_node_id);
            }
            else
            {
                unpacked_path.emplace_back(data.id);
            }
        }
    }

    void UnpackEdge(const NodeID s, const NodeID t, std::vector<NodeID> &unpacked_path) const
    {
        std::stack<std::pair<NodeID, NodeID>> recursion_stack;
        recursion_stack.emplace(s, t);

        std::pair<NodeID, NodeID> edge;
        while (!recursion_stack.empty())
        {
            edge = recursion_stack.top();
            recursion_stack.pop();

            EdgeID smaller_edge_id = SPECIAL_EDGEID;
            int edge_weight = std::numeric_limits<EdgeWeight>::max();
            for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.first))
            {
                const int weight = facade->GetEdgeData(edge_id).distance;
                if ((facade->GetTarget(edge_id) == edge.second) && (weight < edge_weight) &&
                    facade->GetEdgeData(edge_id).forward)
                {
                    smaller_edge_id = edge_id;
                    edge_weight = weight;
                }
            }

            if (SPECIAL_EDGEID == smaller_edge_id)
            {
                for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.second))
                {
                    const int weight = facade->GetEdgeData(edge_id).distance;
                    if ((facade->GetTarget(edge_id) == edge.first) && (weight < edge_weight) &&
                        facade->GetEdgeData(edge_id).backward)
                    {
                        smaller_edge_id = edge_id;
                        edge_weight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(edge_weight != std::numeric_limits<EdgeWeight>::max(),
                             "edge weight invalid");

            const EdgeData &ed = facade->GetEdgeData(smaller_edge_id);
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
                unpacked_path.emplace_back(edge.first);
            }
        }
        unpacked_path.emplace_back(t);
    }

    void RetrievePackedPathFromHeap(const SearchEngineData::QueryHeap &forward_heap,
                                    const SearchEngineData::QueryHeap &reverse_heap,
                                    const NodeID middle_node_id,
                                    std::vector<NodeID> &packed_path) const
    {
        RetrievePackedPathFromSingleHeap(forward_heap, middle_node_id, packed_path);
        std::reverse(packed_path.begin(), packed_path.end());
        packed_path.emplace_back(middle_node_id);
        RetrievePackedPathFromSingleHeap(reverse_heap, middle_node_id, packed_path);
    }

    void RetrievePackedPathFromSingleHeap(const SearchEngineData::QueryHeap &search_heap,
                                          const NodeID middle_node_id,
                                          std::vector<NodeID> &packed_path) const
    {
        NodeID current_node_id = middle_node_id;
        while (current_node_id != search_heap.GetData(current_node_id).parent)
        {
            current_node_id = search_heap.GetData(current_node_id).parent;
            packed_path.emplace_back(current_node_id);
        }
    }

    double get_network_distance(SearchEngineData::QueryHeap &forward_heap,
                                SearchEngineData::QueryHeap &reverse_heap,
                                const PhantomNode &source_phantom,
                                const PhantomNode &target_phantom) const
    {
        EdgeWeight upper_bound = INVALID_EDGE_WEIGHT;
        NodeID middle_node = SPECIAL_NODEID;
        EdgeWeight edge_offset = std::min(0, -source_phantom.GetForwardWeightPlusOffset());
        edge_offset = std::min(edge_offset, -source_phantom.GetReverseWeightPlusOffset());

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

        // search from s and t till new_min/(1+epsilon) > length_of_shortest_path
        while (0 < (forward_heap.Size() + reverse_heap.Size()))
        {
            if (0 < forward_heap.Size())
            {
                RoutingStep(forward_heap, reverse_heap, &middle_node, &upper_bound, edge_offset,
                            true);
            }
            if (0 < reverse_heap.Size())
            {
                RoutingStep(reverse_heap, forward_heap, &middle_node, &upper_bound, edge_offset,
                            false);
            }
        }

        double distance = std::numeric_limits<double>::max();
        if (upper_bound != INVALID_EDGE_WEIGHT)
        {
            std::vector<NodeID> packed_leg;
            RetrievePackedPathFromHeap(forward_heap, reverse_heap, middle_node, packed_leg);
            PhantomNodes nodes;
            nodes.source_phantom = source_phantom;
            nodes.target_phantom = target_phantom;
            std::vector<NodeID> unpacked_path;
            UnpackPath(packed_leg.begin(), packed_leg.end(), unpacked_path);
            std::vector<PathData> uncompressed_path;
            UncompressPath(unpacked_path.begin(), unpacked_path.end(), nodes, uncompressed_path);

            FixedPointCoordinate previous_coordinate = source_phantom.location;
            FixedPointCoordinate current_coordinate;
            distance = 0;
            for (const auto &p : uncompressed_path)
            {
                current_coordinate = p.location;
                distance += coordinate_calculation::great_circle_distance(previous_coordinate,
                                                                          current_coordinate);
                previous_coordinate = current_coordinate;
            }
        }
        return distance;
    }
};

#endif // ROUTING_BASE_HPP
