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
#include <iterator>

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

    /// This function decompresses routes given as list of _edge based_ edge ids.
    ///
    /// (a,b,c) and (b,c,d) form an edge based edge. We will get the edge based
    /// edge ids 0 and 1 as input and the phantom nodes s and t.
    ///           s
    ///       b-----a
    ///       |
    ///       |
    /// d-----c
    ///     t
    /// -> s, b, c, t
    ///
    /// An edge based edge is compressed if the start edge based node is compressed.
    /// e.g. (a,b,c) is marked as compressed if (a,b) is compressed. As such we only
    /// decompress (a,b) and add it to the result path, (b,c) is covered by the next edge.
    ///            s
    ///       b-*-*-a
    ///       |
    ///       *
    ///       |
    /// d-*-*-c
    ///    t
    /// (0, 1) -> (s, *, *, b, *, c, *, t)
    template<typename ForwardIter>
    void UncompressPath(ForwardIter unpacked_path_begin, ForwardIter unpacked_path_end,
                        const PhantomNodes &phantom_node_pair,
                        const bool source_traversed_in_reverse,
                        const bool target_traversed_in_reverse,
                        std::vector<PathData> &uncompressed_path) const
    {
        std::cout << "source: " << phantom_node_pair.source_phantom.forward_node_id << " / " << phantom_node_pair.source_phantom.reverse_node_id << std::endl;
        std::cout << "target: " << phantom_node_pair.target_phantom.forward_node_id << " / " << phantom_node_pair.target_phantom.reverse_node_id << std::endl;
        bool same_edge = phantom_node_pair.source_phantom.forward_node_id == phantom_node_pair.target_phantom.forward_node_id;
        BOOST_ASSERT(!same_edge || phantom_node_pair.source_phantom.reverse_node_id == phantom_node_pair.target_phantom.reverse_node_id);

        // outputs either [first_id, last_id) or in reverse [last_id, first_id)
        // depening on which is bigger.
        // 3, 6 -> 3, 4, 5
        // 3, 0 -> 0, 1, 2
        auto add_node_id_range = [this](unsigned first_id, unsigned last_id, unsigned name_index,
                                        TravelMode travel_mode, std::vector<PathData>& uncompressed_path)
            {
                if (first_id < last_id)
                {
                    for (unsigned node_id = first_id; node_id < last_id; node_id++)
                    {
                        uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id),
                            name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                }
                else if (first_id > last_id)
                {
                    BOOST_ASSERT(first_id > 0);
                    for (unsigned node_id = first_id-1; node_id > last_id; node_id--)
                    {
                        BOOST_ASSERT(node_id > 0);
                        uncompressed_path.emplace_back(node_id, facade->GetCoordinateOfNode(node_id),
                            name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                    // we need this because of possible node_id underflows if last_id = 0
                    uncompressed_path.emplace_back(last_id, facade->GetCoordinateOfNode(last_id),
                        name_index, TurnInstruction::NoTurn, 0, travel_mode);
                }
            };

        // this return SPECIAL_NODEID if the phantom node is on
        // a uncompressed edge
        auto get_phantom_id = [this](const PhantomNode& node)
        {
            const auto geometry_index = node.packed_geometry_id;
            if (geometry_index != SPECIAL_NODEID)
            {
                return facade->BeginGeometry(geometry_index) +
                       node.fwd_segment_position;
            }
            else
            {
                return geometry_index;
            }
        };

        const auto source_id = get_phantom_id(phantom_node_pair.source_phantom);
        const auto target_id = get_phantom_id(phantom_node_pair.target_phantom);
        const unsigned source_name_index = phantom_node_pair.source_phantom.name_id;
        const unsigned target_name_index = phantom_node_pair.target_phantom.name_id;
        const TravelMode source_travel_mode = source_traversed_in_reverse ?
            phantom_node_pair.source_phantom.backward_travel_mode :
            phantom_node_pair.source_phantom.forward_travel_mode;
        const TravelMode target_travel_mode = target_traversed_in_reverse ?
            phantom_node_pair.target_phantom.backward_travel_mode :
            phantom_node_pair.target_phantom.forward_travel_mode;
        const auto source_duration = source_traversed_in_reverse ?
            phantom_node_pair.source_phantom.GetReverseWeightPlusOffset() :
            phantom_node_pair.source_phantom.GetForwardWeightPlusOffset();
        const auto target_duration = target_traversed_in_reverse ?
            phantom_node_pair.target_phantom.GetReverseWeightPlusOffset() :
            phantom_node_pair.target_phantom.GetForwardWeightPlusOffset();

        // source and target are on the same compressed edge
        if (same_edge)
        {
            BOOST_ASSERT(std::distance(unpacked_path_begin, unpacked_path_end) == 0);

            std::cout << "same egde!" << std::endl;

            const EdgeWeight duration = source_duration - target_duration;
            uncompressed_path.emplace_back(source_id, phantom_node_pair.source_phantom.location,
                                           source_name_index, TurnInstruction::HeadOn, duration, source_travel_mode);

            // single compressed edge (start x is not encoded in the geometry bucket)
            // x---->0--->1--->2--->3--->4
            //   t                    s
            // -> 3, 2, 1, 0
            //
            // x---->0--->1--->2--->3--->4
            //    s                    t
            // -> 0, 1, 2, 3
            if (source_id != SPECIAL_NODEID)
            {
                BOOST_ASSERT(source_id >= facade->BeginGeometry(phantom_node_pair.source_phantom.packed_geometry_id));
                BOOST_ASSERT(source_id <  facade->EndGeometry(phantom_node_pair.source_phantom.packed_geometry_id));
                BOOST_ASSERT(target_id >= facade->BeginGeometry(phantom_node_pair.source_phantom.packed_geometry_id));
                BOOST_ASSERT(target_id <  facade->EndGeometry(phantom_node_pair.source_phantom.packed_geometry_id));
                add_node_id_range(source_id, target_id, source_name_index, source_travel_mode, uncompressed_path);

            }

            uncompressed_path.emplace_back(target_id, phantom_node_pair.target_phantom.location,
                                           target_name_index, TurnInstruction::ReachedYourDestination, 0, target_travel_mode);
        }
        else
        {
            BOOST_ASSERT(std::distance(unpacked_path_begin, unpacked_path_end) > 0);

            uncompressed_path.emplace_back(source_id, phantom_node_pair.source_phantom.location,
                                           phantom_node_pair.source_phantom.name_id,
                                           TurnInstruction::HeadOn,
                                           0,
                                           source_travel_mode);

            if (phantom_node_pair.source_phantom.packed_geometry_id != SPECIAL_NODEID)
            {
                const auto geometry_index = phantom_node_pair.source_phantom.packed_geometry_id;
                const auto end_id = source_traversed_in_reverse ?
                                    facade->BeginGeometry(geometry_index) :
                                    facade->EndGeometry(geometry_index);
                BOOST_ASSERT(source_id >= facade->BeginGeometry(geometry_index));
                BOOST_ASSERT(source_id <  facade->EndGeometry(geometry_index));
                add_node_id_range(source_id, end_id, source_name_index, source_travel_mode, uncompressed_path);
            }
            else
            {
                const auto source_edge_id = *unpacked_path_begin;
                BOOST_ASSERT(!facade->EdgeIsCompressed(source_edge_id));
                const auto geometry_index = facade->GetGeometryIndexForEdgeID(source_edge_id);
                uncompressed_path.emplace_back(geometry_index, facade->GetCoordinateOfNode(geometry_index), source_name_index,
                                           TurnInstruction::NoTurn, 0, source_travel_mode);
            }

            // fix last node on source edge: needs the turn instruction and correct distance to source
            // phantom node
            uncompressed_path.back().turn_instruction = facade->GetTurnInstructionForEdgeID(*unpacked_path_begin);
            uncompressed_path.back().segment_duration = source_duration;

            for (auto iter = std::next(unpacked_path_begin); iter != unpacked_path_end; ++iter)
            {
                const auto edge_id = *iter;
                const auto data = facade->GetEdgeData(edge_id);
                const auto duration = data.distance;

                unsigned name_index = facade->GetNameIndexFromEdgeID(edge_id);
                const TurnInstruction turn_instruction = facade->GetTurnInstructionForEdgeID(edge_id);
                const TravelMode travel_mode = facade->GetTravelModeForEdgeID(edge_id);

                if (facade->EdgeIsCompressed(edge_id))
                {
                    const auto geometry_index = facade->GetGeometryIndexForEdgeID(edge_id);
                    const auto begin_id = facade->BeginGeometry(geometry_index);
                    const auto end_id = facade->EndGeometry(geometry_index);

                    // these points all have 0 duration, we only need the geometry
                    add_node_id_range(begin_id, end_id, name_index, travel_mode, uncompressed_path);

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

            // we don't have an edge based egde that covers the path to the target node,
            // so we need to check if its compressed this way
            if (phantom_node_pair.target_phantom.packed_geometry_id != SPECIAL_NODEID)
            {
                const auto geometry_index = phantom_node_pair.target_phantom.packed_geometry_id;
                const auto end_id = target_traversed_in_reverse ?
                                    facade->EndGeometry(geometry_index) :
                                    facade->BeginGeometry(geometry_index);
                BOOST_ASSERT(target_id >= facade->BeginGeometry(geometry_index));
                BOOST_ASSERT(target_id <  facade->EndGeometry(geometry_index));
                add_node_id_range(target_id, end_id, target_name_index, target_travel_mode, uncompressed_path);
            }

            uncompressed_path.emplace_back(target_id, phantom_node_pair.target_phantom.location,
                                           target_name_index, TurnInstruction::ReachedYourDestination,
                                           target_duration, target_travel_mode);

            std::cout << "Unpacked size: " << uncompressed_path.size() << std::endl;
        }
    }

    /// Gets a list of edge based node ids that potentially cover shortcuts.
    /// This recursively unpacks shortcuts until only simple edges are left.
    /// Returns a list of edge-based _edge_ ids.
    template<typename ForwardIter>
    void UnpackPath(ForwardIter packed_path_begin, ForwardIter packed_path_end,
                    std::vector<NodeID> &unpacked_path) const
    {
        BOOST_ASSERT(std::distance(packed_path_begin, packed_path_end) > 0);

        // handle local path on same edge, they are always unpacked
        if (std::distance(packed_path_begin, packed_path_end) < 2)
        {
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

            const EdgeData &data = facade->GetEdgeData(smaller_edge_id);
            if (data.shortcut)
            {
                std::cout << "Shortcut!" << std::endl;
                const NodeID middle_node_id = data.id;
                // again, we need to this in reversed order
                recursion_stack.emplace(middle_node_id, edge.second);
                recursion_stack.emplace(edge.first, middle_node_id);
            }
            else
            {
                std::cout << "Added " << data.id << std::endl;
                // add edge based egde id to result vector
                unpacked_path.emplace_back(data.id);
            }
        }

        std::cout << "packed: ";
        std::copy(packed_path_begin, packed_path_end, std::ostream_iterator<unsigned>(std::cout, ","));
        std::cout << std::endl;
        std::cout << "unpacked: ";
        std::copy(unpacked_path.begin(), unpacked_path.end(), std::ostream_iterator<unsigned>(std::cout, ","));
        std::cout << std::endl;
    }

    /// Unpacks the edge starting at s and ending at t.
    /// Returns the list of unpacked edge based _node_ ids.
    /// Important: This is in contrast to UnpackPath which returns _edge_ ids.
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


            auto source_traversed_in_reverse = packed_leg.front() != nodes.source_phantom.forward_node_id;
            auto target_traversed_in_reverse = packed_leg.back() != nodes.target_phantom.forward_node_id;
            std::vector<PathData> uncompressed_path;
            UncompressPath(unpacked_path.begin(), unpacked_path.end(),
                           nodes, source_traversed_in_reverse, target_traversed_in_reverse,
                           uncompressed_path);

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
