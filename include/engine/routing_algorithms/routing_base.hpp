#ifndef ROUTING_BASE_HPP
#define ROUTING_BASE_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/search_engine_data.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{

namespace routing_algorithms
{

template <class DataFacadeT, class Derived> class BasicRoutingInterface
{
  private:
    using EdgeData = typename DataFacadeT::EdgeData;

  protected:
    DataFacadeT *facade;

  public:
    explicit BasicRoutingInterface(DataFacadeT *facade) : facade(facade) {}
    ~BasicRoutingInterface() {}

    BasicRoutingInterface(const BasicRoutingInterface &) = delete;
    BasicRoutingInterface &operator=(const BasicRoutingInterface &) = delete;

    /*
    min_edge_offset is needed in case we use multiple
    nodes as start/target nodes with different (even negative) offsets.
    In that case the termination criterion is not correct
    anymore.

    Example:
    forward heap: a(-100), b(0),
    reverse heap: c(0), d(100)

    a --- d
      \ /
      / \
    b --- c

    This is equivalent to running a bi-directional Dijkstra on the following graph:

        a --- d
       /  \ /  \
      y    x    z
       \  / \  /
        b --- c

    The graph is constructed by inserting nodes y and z that are connected to the initial nodes
    using edges (y, a) with weight -100, (y, b) with weight 0 and,
    (d, z) with weight 100, (c, z) with weight 0 corresponding.
    Since we are dealing with a graph that contains _negative_ edges,
    we need to add an offset to the termination criterion.
    */
    void RoutingStep(SearchEngineData::QueryHeap &forward_heap,
                     SearchEngineData::QueryHeap &reverse_heap,
                     NodeID &middle_node_id,
                     std::int32_t &upper_bound,
                     std::int32_t min_edge_offset,
                     const bool forward_direction,
                     const bool stalling,
                     const bool force_loop_forward,
                     const bool force_loop_reverse) const
    {
        const NodeID node = forward_heap.DeleteMin();
        const std::int32_t distance = forward_heap.GetKey(node);

        if (reverse_heap.WasInserted(node))
        {
            const std::int32_t new_distance = reverse_heap.GetKey(node) + distance;
            if (new_distance < upper_bound)
            {
                // if loops are forced, they are so at the source
                if (new_distance >= 0 &&
                    (!force_loop_forward || forward_heap.GetData(node).parent != node) &&
                    (!force_loop_reverse || reverse_heap.GetData(node).parent != node))
                {
                    middle_node_id = node;
                    upper_bound = new_distance;
                }
                else
                {
                    // check whether there is a loop present at the node
                    for (const auto edge : facade->GetAdjacentEdgeRange(node))
                    {
                        const EdgeData &data = facade->GetEdgeData(edge);
                        bool forward_directionFlag =
                            (forward_direction ? data.forward : data.backward);
                        if (forward_directionFlag)
                        {
                            const NodeID to = facade->GetTarget(edge);
                            if (to == node)
                            {
                                const EdgeWeight edge_weight = data.distance;
                                const std::int32_t loop_distance = new_distance + edge_weight;
                                if (loop_distance >= 0 && loop_distance < upper_bound)
                                {
                                    middle_node_id = node;
                                    upper_bound = loop_distance;
                                }
                            }
                        }
                    }
                }
            }
        }

        // make sure we don't terminate too early if we initialize the distance
        // for the nodes in the forward heap with the forward/reverse offset
        BOOST_ASSERT(min_edge_offset <= 0);
        if (distance + min_edge_offset > upper_bound)
        {
            forward_heap.DeleteAll();
            return;
        }

        // Stalling
        if (stalling)
        {
            for (const auto edge : facade->GetAdjacentEdgeRange(node))
            {
                const EdgeData &data = facade->GetEdgeData(edge);
                const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
                if (reverse_flag)
                {
                    const NodeID to = facade->GetTarget(edge);
                    const EdgeWeight edge_weight = data.distance;

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
        }

        for (const auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const EdgeData &data = facade->GetEdgeData(edge);
            bool forward_directionFlag = (forward_direction ? data.forward : data.backward);
            if (forward_directionFlag)
            {

                const NodeID to = facade->GetTarget(edge);
                const EdgeWeight edge_weight = data.distance;

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

    inline EdgeWeight GetLoopWeight(NodeID node) const
    {
        EdgeWeight loop_weight = INVALID_EDGE_WEIGHT;
        for (auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = facade->GetEdgeData(edge);
            if (data.forward)
            {
                const NodeID to = facade->GetTarget(edge);
                if (to == node)
                {
                    loop_weight = std::min(loop_weight, data.distance);
                }
            }
        }
        return loop_weight;
    }

    template <typename RandomIter>
    void UnpackPath(RandomIter packed_path_begin,
                    RandomIter packed_path_end,
                    const PhantomNodes &phantom_node_pair,
                    std::vector<PathData> &unpacked_path) const
    {
        const bool start_traversed_in_reverse =
            (*packed_path_begin != phantom_node_pair.source_phantom.forward_segment_id.id);
        const bool target_traversed_in_reverse =
            (*std::prev(packed_path_end) != phantom_node_pair.target_phantom.forward_segment_id.id);

        BOOST_ASSERT(std::distance(packed_path_begin, packed_path_end) > 0);
        std::stack<std::pair<NodeID, NodeID>> recursion_stack;

        // We have to push the path in reverse order onto the stack because it's LIFO.
        for (auto current = std::prev(packed_path_end); current != packed_path_begin;
             current = std::prev(current))
        {
            recursion_stack.emplace(*std::prev(current), *current);
        }

        BOOST_ASSERT(*packed_path_begin == phantom_node_pair.source_phantom.forward_segment_id.id ||
                     *packed_path_begin == phantom_node_pair.source_phantom.reverse_segment_id.id);
        BOOST_ASSERT(
            *std::prev(packed_path_end) == phantom_node_pair.target_phantom.forward_segment_id.id ||
            *std::prev(packed_path_end) == phantom_node_pair.target_phantom.reverse_segment_id.id);

        std::pair<NodeID, NodeID> edge;
        while (!recursion_stack.empty())
        {
            // edge.first         edge.second
            //     *------------------>*
            //            edge_id
            edge = recursion_stack.top();
            recursion_stack.pop();

            // Contraction might introduce double edges by inserting shortcuts
            // this searching for the smallest upwards edge found by the forward search
            EdgeID smaller_edge_id = SPECIAL_EDGEID;
            EdgeWeight edge_weight = std::numeric_limits<EdgeWeight>::max();
            for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.first))
            {
                const EdgeWeight weight = facade->GetEdgeData(edge_id).distance;
                if ((facade->GetTarget(edge_id) == edge.second) && (weight < edge_weight) &&
                    facade->GetEdgeData(edge_id).forward)
                {
                    smaller_edge_id = edge_id;
                    edge_weight = weight;
                }
            }

            // edge.first         edge.second
            //     *<------------------*
            //            edge_id
            // if we don't find a forward edge, this edge must have been an downwards edge
            // found by the reverse search.
            if (SPECIAL_EDGEID == smaller_edge_id)
            {
                for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.second))
                {
                    const EdgeWeight weight = facade->GetEdgeData(edge_id).distance;
                    if ((facade->GetTarget(edge_id) == edge.first) && (weight < edge_weight) &&
                        facade->GetEdgeData(edge_id).backward)
                    {
                        smaller_edge_id = edge_id;
                        edge_weight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(edge_weight != INVALID_EDGE_WEIGHT, "edge id invalid");

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
                BOOST_ASSERT_MSG(!ed.shortcut, "original edge flagged as shortcut");
                unsigned name_index = facade->GetNameIndexFromEdgeID(ed.id);
                const auto turn_instruction = facade->GetTurnInstructionForEdgeID(ed.id);
                const extractor::TravelMode travel_mode =
                    (unpacked_path.empty() && start_traversed_in_reverse)
                        ? phantom_node_pair.source_phantom.backward_travel_mode
                        : facade->GetTravelModeForEdgeID(ed.id);

                const auto geometry_index = facade->GetGeometryIndexForEdgeID(ed.id);
                std::vector<NodeID> id_vector;
                facade->GetUncompressedGeometry(geometry_index, id_vector);
                BOOST_ASSERT(id_vector.size() > 0);

                std::vector<EdgeWeight> weight_vector;
                facade->GetUncompressedWeights(geometry_index, weight_vector);
                BOOST_ASSERT(weight_vector.size() > 0);

                std::vector<DatasourceID> datasource_vector;
                facade->GetUncompressedDatasources(geometry_index, datasource_vector);

                auto total_weight = std::accumulate(weight_vector.begin(), weight_vector.end(), 0);

                BOOST_ASSERT(weight_vector.size() == id_vector.size());
                const bool is_first_segment = unpacked_path.empty();

                const std::size_t start_index =
                    (is_first_segment
                         ? ((start_traversed_in_reverse)
                                ? id_vector.size() -
                                      phantom_node_pair.source_phantom.fwd_segment_position - 1
                                : phantom_node_pair.source_phantom.fwd_segment_position)
                         : 0);
                const std::size_t end_index = id_vector.size();

                BOOST_ASSERT(start_index >= 0);
                BOOST_ASSERT(start_index < end_index);
                for (std::size_t i = start_index; i < end_index; ++i)
                {
                    unpacked_path.push_back(
                        PathData{id_vector[i],
                                 name_index,
                                 weight_vector[i],
                                 extractor::guidance::TurnInstruction::NO_TURN(),
                                 {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
                                 travel_mode,
                                 INVALID_ENTRY_CLASSID,
                                 datasource_vector[i]});
                }
                BOOST_ASSERT(unpacked_path.size() > 0);
                if (facade->hasLaneData(ed.id))
                    unpacked_path.back().lane_data = facade->GetLaneData(ed.id);

                unpacked_path.back().entry_classid = facade->GetEntryClassID(ed.id);
                unpacked_path.back().turn_instruction = turn_instruction;
                unpacked_path.back().duration_until_turn += (ed.distance - total_weight);
            }
        }
        std::size_t start_index = 0, end_index = 0;
        std::vector<unsigned> id_vector;
        std::vector<EdgeWeight> weight_vector;
        std::vector<DatasourceID> datasource_vector;
        const bool is_local_path = (phantom_node_pair.source_phantom.forward_packed_geometry_id ==
                                    phantom_node_pair.target_phantom.forward_packed_geometry_id) &&
                                   unpacked_path.empty();

        if (target_traversed_in_reverse)
        {
            facade->GetUncompressedGeometry(
                phantom_node_pair.target_phantom.reverse_packed_geometry_id, id_vector);

            facade->GetUncompressedWeights(
                phantom_node_pair.target_phantom.reverse_packed_geometry_id, weight_vector);

            facade->GetUncompressedDatasources(
                phantom_node_pair.target_phantom.reverse_packed_geometry_id, datasource_vector);

            if (is_local_path)
            {
                start_index =
                    id_vector.size() - phantom_node_pair.source_phantom.fwd_segment_position - 1;
            }
            end_index =
                id_vector.size() - phantom_node_pair.target_phantom.fwd_segment_position - 1;
        }
        else
        {
            if (is_local_path)
            {
                start_index = phantom_node_pair.source_phantom.fwd_segment_position;
            }
            end_index = phantom_node_pair.target_phantom.fwd_segment_position;
            facade->GetUncompressedGeometry(
                phantom_node_pair.target_phantom.forward_packed_geometry_id, id_vector);

            facade->GetUncompressedWeights(
                phantom_node_pair.target_phantom.forward_packed_geometry_id, weight_vector);

            facade->GetUncompressedDatasources(
                phantom_node_pair.target_phantom.forward_packed_geometry_id, datasource_vector);
        }

        // Given the following compressed geometry:
        // U---v---w---x---y---Z
        //    s           t
        // s: fwd_segment 0
        // t: fwd_segment 3
        // -> (U, v), (v, w), (w, x)
        // note that (x, t) is _not_ included but needs to be added later.
        for (std::size_t i = start_index; i != end_index; (start_index < end_index ? ++i : --i))
        {
            BOOST_ASSERT(i < id_vector.size());
            BOOST_ASSERT(phantom_node_pair.target_phantom.forward_travel_mode > 0);
            unpacked_path.push_back(PathData{
                id_vector[i],
                phantom_node_pair.target_phantom.name_id,
                weight_vector[i],
                extractor::guidance::TurnInstruction::NO_TURN(),
                {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
                target_traversed_in_reverse ? phantom_node_pair.target_phantom.backward_travel_mode
                                            : phantom_node_pair.target_phantom.forward_travel_mode,
                INVALID_ENTRY_CLASSID,
                datasource_vector[i]});
        }

        if (unpacked_path.size() > 0)
        {
            const auto source_weight = start_traversed_in_reverse
                                           ? phantom_node_pair.source_phantom.reverse_weight
                                           : phantom_node_pair.source_phantom.forward_weight;
            // The above code will create segments for (v, w), (w,x), (x, y) and (y, Z).
            // However the first segment duration needs to be adjusted to the fact that the source
            // phantom is in the middle of the segment. We do this by subtracting v--s from the
            // duration.

            // Since it's possible duration_until_turn can be less than source_weight here if
            // a negative enough turn penalty is used to modify this edge weight during
            // osrm-contract, we clamp to 0 here so as not to return a negative duration
            // for this segment.

            // TODO this creates a scenario where it's possible the duration from a phantom
            // node to the first turn would be the same as from end to end of a segment,
            // which is obviously incorrect and not ideal...
            unpacked_path.front().duration_until_turn =
                std::max(unpacked_path.front().duration_until_turn - source_weight, 0);
        }

        // there is no equivalent to a node-based node in an edge-expanded graph.
        // two equivalent routes may start (or end) at different node-based edges
        // as they are added with the offset how much "distance" on the edge
        // has already been traversed. Depending on offset one needs to remove
        // the last node.
        if (unpacked_path.size() > 1)
        {
            const std::size_t last_index = unpacked_path.size() - 1;
            const std::size_t second_to_last_index = last_index - 1;

            if (unpacked_path[last_index].turn_via_node ==
                unpacked_path[second_to_last_index].turn_via_node)
            {
                unpacked_path.pop_back();
            }
            BOOST_ASSERT(!unpacked_path.empty());
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
            EdgeWeight edge_weight = std::numeric_limits<EdgeWeight>::max();
            for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.first))
            {
                const EdgeWeight weight = facade->GetEdgeData(edge_id).distance;
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
                    const EdgeWeight weight = facade->GetEdgeData(edge_id).distance;
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

    // assumes that heaps are already setup correctly.
    // ATTENTION: This only works if no additional offset is supplied next to the Phantom Node
    // Offsets.
    // In case additional offsets are supplied, you might have to force a loop first.
    // A forced loop might be necessary, if source and target are on the same segment.
    // If this is the case and the offsets of the respective direction are larger for the source
    // than the target
    // then a force loop is required (e.g. source_phantom.forward_segment_id ==
    // target_phantom.forward_segment_id
    // && source_phantom.GetForwardWeightPlusOffset() > target_phantom.GetForwardWeightPlusOffset())
    // requires
    // a force loop, if the heaps have been initialized with positive offsets.
    void Search(SearchEngineData::QueryHeap &forward_heap,
                SearchEngineData::QueryHeap &reverse_heap,
                std::int32_t &distance,
                std::vector<NodeID> &packed_leg,
                const bool force_loop_forward,
                const bool force_loop_reverse,
                const int duration_upper_bound = INVALID_EDGE_WEIGHT) const
    {
        NodeID middle = SPECIAL_NODEID;
        distance = duration_upper_bound;

        // get offset to account for offsets on phantom nodes on compressed edges
        const auto min_edge_offset = std::min(0, forward_heap.MinKey());
        BOOST_ASSERT(min_edge_offset <= 0);
        // we only every insert negative offsets for nodes in the forward heap
        BOOST_ASSERT(reverse_heap.MinKey() >= 0);

        // run two-Target Dijkstra routing step.
        const constexpr bool STALLING_ENABLED = true;
        while (0 < (forward_heap.Size() + reverse_heap.Size()))
        {
            if (!forward_heap.Empty())
            {
                RoutingStep(forward_heap,
                            reverse_heap,
                            middle,
                            distance,
                            min_edge_offset,
                            true,
                            STALLING_ENABLED,
                            force_loop_forward,
                            force_loop_reverse);
            }
            if (!reverse_heap.Empty())
            {
                RoutingStep(reverse_heap,
                            forward_heap,
                            middle,
                            distance,
                            min_edge_offset,
                            false,
                            STALLING_ENABLED,
                            force_loop_reverse,
                            force_loop_forward);
            }
        }

        // No path found for both target nodes?
        if (duration_upper_bound <= distance || SPECIAL_NODEID == middle)
        {
            distance = INVALID_EDGE_WEIGHT;
            return;
        }

        // Was a paths over one of the forward/reverse nodes not found?
        BOOST_ASSERT_MSG((SPECIAL_NODEID != middle && INVALID_EDGE_WEIGHT != distance),
                         "no path found");

        // make sure to correctly unpack loops
        if (distance != forward_heap.GetKey(middle) + reverse_heap.GetKey(middle))
        {
            // self loop makes up the full path
            packed_leg.push_back(middle);
            packed_leg.push_back(middle);
        }
        else
        {
            RetrievePackedPathFromHeap(forward_heap, reverse_heap, middle, packed_leg);
        }
    }

    // assumes that heaps are already setup correctly.
    // A forced loop might be necessary, if source and target are on the same segment.
    // If this is the case and the offsets of the respective direction are larger for the source
    // than the target
    // then a force loop is required (e.g. source_phantom.forward_segment_id ==
    // target_phantom.forward_segment_id
    // && source_phantom.GetForwardWeightPlusOffset() > target_phantom.GetForwardWeightPlusOffset())
    // requires
    // a force loop, if the heaps have been initialized with positive offsets.
    void SearchWithCore(SearchEngineData::QueryHeap &forward_heap,
                        SearchEngineData::QueryHeap &reverse_heap,
                        SearchEngineData::QueryHeap &forward_core_heap,
                        SearchEngineData::QueryHeap &reverse_core_heap,
                        int &distance,
                        std::vector<NodeID> &packed_leg,
                        const bool force_loop_forward,
                        const bool force_loop_reverse,
                        int duration_upper_bound = INVALID_EDGE_WEIGHT) const
    {
        NodeID middle = SPECIAL_NODEID;
        distance = duration_upper_bound;

        std::vector<std::pair<NodeID, EdgeWeight>> forward_entry_points;
        std::vector<std::pair<NodeID, EdgeWeight>> reverse_entry_points;

        // get offset to account for offsets on phantom nodes on compressed edges
        const auto min_edge_offset = std::min(0, forward_heap.MinKey());
        // we only every insert negative offsets for nodes in the forward heap
        BOOST_ASSERT(reverse_heap.MinKey() >= 0);

        const constexpr bool STALLING_ENABLED = true;
        // run two-Target Dijkstra routing step.
        while (0 < (forward_heap.Size() + reverse_heap.Size()))
        {
            if (!forward_heap.Empty())
            {
                if (facade->IsCoreNode(forward_heap.Min()))
                {
                    const NodeID node = forward_heap.DeleteMin();
                    const int key = forward_heap.GetKey(node);
                    forward_entry_points.emplace_back(node, key);
                }
                else
                {
                    RoutingStep(forward_heap,
                                reverse_heap,
                                middle,
                                distance,
                                min_edge_offset,
                                true,
                                STALLING_ENABLED,
                                force_loop_forward,
                                force_loop_reverse);
                }
            }
            if (!reverse_heap.Empty())
            {
                if (facade->IsCoreNode(reverse_heap.Min()))
                {
                    const NodeID node = reverse_heap.DeleteMin();
                    const int key = reverse_heap.GetKey(node);
                    reverse_entry_points.emplace_back(node, key);
                }
                else
                {
                    RoutingStep(reverse_heap,
                                forward_heap,
                                middle,
                                distance,
                                min_edge_offset,
                                false,
                                STALLING_ENABLED,
                                force_loop_reverse,
                                force_loop_forward);
                }
            }
        }
        // TODO check if unordered_set might be faster
        // sort by id and increasing by distance
        auto entry_point_comparator = [](const std::pair<NodeID, EdgeWeight> &lhs,
                                         const std::pair<NodeID, EdgeWeight> &rhs) {
            return lhs.first < rhs.first || (lhs.first == rhs.first && lhs.second < rhs.second);
        };
        std::sort(forward_entry_points.begin(), forward_entry_points.end(), entry_point_comparator);
        std::sort(reverse_entry_points.begin(), reverse_entry_points.end(), entry_point_comparator);

        NodeID last_id = SPECIAL_NODEID;
        forward_core_heap.Clear();
        reverse_core_heap.Clear();
        for (const auto &p : forward_entry_points)
        {
            if (p.first == last_id)
            {
                continue;
            }
            forward_core_heap.Insert(p.first, p.second, p.first);
            last_id = p.first;
        }
        last_id = SPECIAL_NODEID;
        for (const auto &p : reverse_entry_points)
        {
            if (p.first == last_id)
            {
                continue;
            }
            reverse_core_heap.Insert(p.first, p.second, p.first);
            last_id = p.first;
        }

        // get offset to account for offsets on phantom nodes on compressed edges
        int min_core_edge_offset = 0;
        if (forward_core_heap.Size() > 0)
        {
            min_core_edge_offset = std::min(min_core_edge_offset, forward_core_heap.MinKey());
        }
        if (reverse_core_heap.Size() > 0 && reverse_core_heap.MinKey() < 0)
        {
            min_core_edge_offset = std::min(min_core_edge_offset, reverse_core_heap.MinKey());
        }
        BOOST_ASSERT(min_core_edge_offset <= 0);

        // run two-target Dijkstra routing step on core with termination criterion
        const constexpr bool STALLING_DISABLED = false;
        while (0 < forward_core_heap.Size() && 0 < reverse_core_heap.Size() &&
               distance > (forward_core_heap.MinKey() + reverse_core_heap.MinKey()))
        {
            RoutingStep(forward_core_heap,
                        reverse_core_heap,
                        middle,
                        distance,
                        min_core_edge_offset,
                        true,
                        STALLING_DISABLED,
                        force_loop_forward,
                        force_loop_reverse);

            RoutingStep(reverse_core_heap,
                        forward_core_heap,
                        middle,
                        distance,
                        min_core_edge_offset,
                        false,
                        STALLING_DISABLED,
                        force_loop_reverse,
                        force_loop_forward);
        }

        // No path found for both target nodes?
        if (duration_upper_bound <= distance || SPECIAL_NODEID == middle)
        {
            distance = INVALID_EDGE_WEIGHT;
            return;
        }

        // Was a paths over one of the forward/reverse nodes not found?
        BOOST_ASSERT_MSG((SPECIAL_NODEID != middle && INVALID_EDGE_WEIGHT != distance),
                         "no path found");

        // we need to unpack sub path from core heaps
        if (facade->IsCoreNode(middle))
        {
            if (distance != forward_core_heap.GetKey(middle) + reverse_core_heap.GetKey(middle))
            {
                // self loop
                BOOST_ASSERT(forward_core_heap.GetData(middle).parent == middle &&
                             reverse_core_heap.GetData(middle).parent == middle);
                packed_leg.push_back(middle);
                packed_leg.push_back(middle);
            }
            else
            {
                std::vector<NodeID> packed_core_leg;
                RetrievePackedPathFromHeap(
                    forward_core_heap, reverse_core_heap, middle, packed_core_leg);
                BOOST_ASSERT(packed_core_leg.size() > 0);
                RetrievePackedPathFromSingleHeap(forward_heap, packed_core_leg.front(), packed_leg);
                std::reverse(packed_leg.begin(), packed_leg.end());
                packed_leg.insert(packed_leg.end(), packed_core_leg.begin(), packed_core_leg.end());
                RetrievePackedPathFromSingleHeap(reverse_heap, packed_core_leg.back(), packed_leg);
            }
        }
        else
        {
            if (distance != forward_heap.GetKey(middle) + reverse_heap.GetKey(middle))
            {
                // self loop
                BOOST_ASSERT(forward_heap.GetData(middle).parent == middle &&
                             reverse_heap.GetData(middle).parent == middle);
                packed_leg.push_back(middle);
                packed_leg.push_back(middle);
            }
            else
            {
                RetrievePackedPathFromHeap(forward_heap, reverse_heap, middle, packed_leg);
            }
        }
    }

    bool NeedsLoopForward(const PhantomNode &source_phantom,
                          const PhantomNode &target_phantom) const
    {
        return source_phantom.forward_segment_id.enabled &&
               target_phantom.forward_segment_id.enabled &&
               source_phantom.forward_segment_id.id == target_phantom.forward_segment_id.id &&
               source_phantom.GetForwardWeightPlusOffset() >
                   target_phantom.GetForwardWeightPlusOffset();
    }

    bool NeedsLoopBackwards(const PhantomNode &source_phantom,
                            const PhantomNode &target_phantom) const
    {
        return source_phantom.reverse_segment_id.enabled &&
               target_phantom.reverse_segment_id.enabled &&
               source_phantom.reverse_segment_id.id == target_phantom.reverse_segment_id.id &&
               source_phantom.GetReverseWeightPlusOffset() >
                   target_phantom.GetReverseWeightPlusOffset();
    }

    double GetPathDistance(const std::vector<NodeID> &packed_path,
                           const PhantomNode &source_phantom,
                           const PhantomNode &target_phantom) const
    {
        std::vector<PathData> unpacked_path;
        PhantomNodes nodes;
        nodes.source_phantom = source_phantom;
        nodes.target_phantom = target_phantom;
        UnpackPath(packed_path.begin(), packed_path.end(), nodes, unpacked_path);

        using util::coordinate_calculation::detail::DEGREE_TO_RAD;
        using util::coordinate_calculation::detail::EARTH_RADIUS;

        double distance = 0;
        double prev_lat =
            static_cast<double>(toFloating(source_phantom.location.lat)) * DEGREE_TO_RAD;
        double prev_lon =
            static_cast<double>(toFloating(source_phantom.location.lon)) * DEGREE_TO_RAD;
        double prev_cos = std::cos(prev_lat);
        for (const auto &p : unpacked_path)
        {
            const auto current_coordinate = facade->GetCoordinateOfNode(p.turn_via_node);

            const double current_lat =
                static_cast<double>(toFloating(current_coordinate.lat)) * DEGREE_TO_RAD;
            const double current_lon =
                static_cast<double>(toFloating(current_coordinate.lon)) * DEGREE_TO_RAD;
            const double current_cos = std::cos(current_lat);

            const double sin_dlon = std::sin((prev_lon - current_lon) / 2.0);
            const double sin_dlat = std::sin((prev_lat - current_lat) / 2.0);

            const double aharv = sin_dlat * sin_dlat + prev_cos * current_cos * sin_dlon * sin_dlon;
            const double charv = 2. * std::atan2(std::sqrt(aharv), std::sqrt(1.0 - aharv));
            distance += EARTH_RADIUS * charv;

            prev_lat = current_lat;
            prev_lon = current_lon;
            prev_cos = current_cos;
        }

        const double current_lat =
            static_cast<double>(toFloating(target_phantom.location.lat)) * DEGREE_TO_RAD;
        const double current_lon =
            static_cast<double>(toFloating(target_phantom.location.lon)) * DEGREE_TO_RAD;
        const double current_cos = std::cos(current_lat);

        const double sin_dlon = std::sin((prev_lon - current_lon) / 2.0);
        const double sin_dlat = std::sin((prev_lat - current_lat) / 2.0);

        const double aharv = sin_dlat * sin_dlat + prev_cos * current_cos * sin_dlon * sin_dlon;
        const double charv = 2. * std::atan2(std::sqrt(aharv), std::sqrt(1.0 - aharv));
        distance += EARTH_RADIUS * charv;

        return distance;
    }

    // Requires the heaps for be empty
    // If heaps should be adjusted to be initialized outside of this function,
    // the addition of force_loop parameters might be required
    double GetNetworkDistanceWithCore(SearchEngineData::QueryHeap &forward_heap,
                                      SearchEngineData::QueryHeap &reverse_heap,
                                      SearchEngineData::QueryHeap &forward_core_heap,
                                      SearchEngineData::QueryHeap &reverse_core_heap,
                                      const PhantomNode &source_phantom,
                                      const PhantomNode &target_phantom,
                                      int duration_upper_bound = INVALID_EDGE_WEIGHT) const
    {
        BOOST_ASSERT(forward_heap.Empty());
        BOOST_ASSERT(reverse_heap.Empty());

        if (source_phantom.forward_segment_id.enabled)
        {
            forward_heap.Insert(source_phantom.forward_segment_id.id,
                                -source_phantom.GetForwardWeightPlusOffset(),
                                source_phantom.forward_segment_id.id);
        }
        if (source_phantom.reverse_segment_id.enabled)
        {
            forward_heap.Insert(source_phantom.reverse_segment_id.id,
                                -source_phantom.GetReverseWeightPlusOffset(),
                                source_phantom.reverse_segment_id.id);
        }

        if (target_phantom.forward_segment_id.enabled)
        {
            reverse_heap.Insert(target_phantom.forward_segment_id.id,
                                target_phantom.GetForwardWeightPlusOffset(),
                                target_phantom.forward_segment_id.id);
        }
        if (target_phantom.reverse_segment_id.enabled)
        {
            reverse_heap.Insert(target_phantom.reverse_segment_id.id,
                                target_phantom.GetReverseWeightPlusOffset(),
                                target_phantom.reverse_segment_id.id);
        }

        const bool constexpr DO_NOT_FORCE_LOOPS =
            false; // prevents forcing of loops, since offsets are set correctly

        int duration = INVALID_EDGE_WEIGHT;
        std::vector<NodeID> packed_path;
        SearchWithCore(forward_heap,
                       reverse_heap,
                       forward_core_heap,
                       reverse_core_heap,
                       duration,
                       packed_path,
                       DO_NOT_FORCE_LOOPS,
                       DO_NOT_FORCE_LOOPS,
                       duration_upper_bound);

        double distance = std::numeric_limits<double>::max();
        if (duration != INVALID_EDGE_WEIGHT)
        {
            return GetPathDistance(packed_path, source_phantom, target_phantom);
        }
        return distance;
    }

    // Requires the heaps for be empty
    // If heaps should be adjusted to be initialized outside of this function,
    // the addition of force_loop parameters might be required
    double GetNetworkDistance(SearchEngineData::QueryHeap &forward_heap,
                              SearchEngineData::QueryHeap &reverse_heap,
                              const PhantomNode &source_phantom,
                              const PhantomNode &target_phantom,
                              int duration_upper_bound = INVALID_EDGE_WEIGHT) const
    {
        BOOST_ASSERT(forward_heap.Empty());
        BOOST_ASSERT(reverse_heap.Empty());

        if (source_phantom.forward_segment_id.enabled)
        {
            forward_heap.Insert(source_phantom.forward_segment_id.id,
                                -source_phantom.GetForwardWeightPlusOffset(),
                                source_phantom.forward_segment_id.id);
        }
        if (source_phantom.reverse_segment_id.enabled)
        {
            forward_heap.Insert(source_phantom.reverse_segment_id.id,
                                -source_phantom.GetReverseWeightPlusOffset(),
                                source_phantom.reverse_segment_id.id);
        }

        if (target_phantom.forward_segment_id.enabled)
        {
            reverse_heap.Insert(target_phantom.forward_segment_id.id,
                                target_phantom.GetForwardWeightPlusOffset(),
                                target_phantom.forward_segment_id.id);
        }
        if (target_phantom.reverse_segment_id.enabled)
        {
            reverse_heap.Insert(target_phantom.reverse_segment_id.id,
                                target_phantom.GetReverseWeightPlusOffset(),
                                target_phantom.reverse_segment_id.id);
        }

        const bool constexpr DO_NOT_FORCE_LOOPS =
            false; // prevents forcing of loops, since offsets are set correctly

        int duration = INVALID_EDGE_WEIGHT;
        std::vector<NodeID> packed_path;
        Search(forward_heap,
               reverse_heap,
               duration,
               packed_path,
               DO_NOT_FORCE_LOOPS,
               DO_NOT_FORCE_LOOPS,
               duration_upper_bound);

        if (duration == INVALID_EDGE_WEIGHT)
        {
            return std::numeric_limits<double>::max();
        }

        return GetPathDistance(packed_path, source_phantom, target_phantom);
    }
};
}
}
}

#endif // ROUTING_BASE_HPP
