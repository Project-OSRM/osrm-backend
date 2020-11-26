#ifndef OSRM_SHORTEST_PATH_IMPL_HPP
#define OSRM_SHORTEST_PATH_IMPL_HPP

#include "engine/routing_algorithms/shortest_path.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace
{

const static constexpr bool DO_NOT_FORCE_LOOP = false;

// allows a uturn at the target_phantom
// searches source forward/reverse -> target forward/reverse
template <typename Algorithm>
void searchWithUTurn(SearchEngineData<Algorithm> &engine_working_data,
                     const DataFacade<Algorithm> &facade,
                     typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                     typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                     const bool search_from_forward_node,
                     const bool search_from_reverse_node,
                     const bool search_to_forward_node,
                     const bool search_to_reverse_node,
                     const PhantomNode &source_phantom,
                     const PhantomNode &target_phantom,
                     const int total_weight_to_forward,
                     const int total_weight_to_reverse,
                     int &new_total_weight,
                     std::vector<NodeID> &leg_packed_path)
{
    forward_heap.Clear();
    reverse_heap.Clear();
    if (search_from_forward_node)
    {
        forward_heap.Insert(source_phantom.forward_segment_id.id,
                            -source_phantom.GetForwardWeightPlusOffset(),
                            source_phantom.forward_segment_id.id);
    }
    if (search_from_reverse_node)
    {
        forward_heap.Insert(source_phantom.reverse_segment_id.id,
                            -source_phantom.GetReverseWeightPlusOffset(),
                            source_phantom.reverse_segment_id.id);
    }
    if (search_to_forward_node)
    {
        reverse_heap.Insert(target_phantom.forward_segment_id.id,
                            target_phantom.GetForwardWeightPlusOffset(),
                            target_phantom.forward_segment_id.id);
    }
    if (search_to_reverse_node)
    {
        reverse_heap.Insert(target_phantom.reverse_segment_id.id,
                            target_phantom.GetReverseWeightPlusOffset(),
                            target_phantom.reverse_segment_id.id);
    }

    // this is only relevent if source and target are on the same compressed edge
    auto is_oneway_source = !(search_from_forward_node && search_from_reverse_node);
    auto is_oneway_target = !(search_to_forward_node && search_to_reverse_node);
    // we only enable loops here if we can't search from forward to backward node
    auto needs_loop_forwards = is_oneway_source && needsLoopForward(source_phantom, target_phantom);
    auto needs_loop_backwards =
        is_oneway_target && needsLoopBackwards(source_phantom, target_phantom);

    search(engine_working_data,
           facade,
           forward_heap,
           reverse_heap,
           new_total_weight,
           leg_packed_path,
           needs_loop_forwards,
           needs_loop_backwards,
           {source_phantom, target_phantom});

    // if no route is found between two parts of the via-route, the entire route becomes
    // invalid. Adding to invalid edge weight sadly doesn't return an invalid edge weight. Here
    // we prevent the possible overflow, faking the addition of infinity + x == infinity
    if (new_total_weight != INVALID_EDGE_WEIGHT)
        new_total_weight += std::min(total_weight_to_forward, total_weight_to_reverse);
}

// searches shortest path between:
// source forward/reverse -> target forward
// source forward/reverse -> target reverse
template <typename Algorithm>
void search(SearchEngineData<Algorithm> &engine_working_data,
            const DataFacade<Algorithm> &facade,
            typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
            typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
            const bool search_from_forward_node,
            const bool search_from_reverse_node,
            const bool search_to_forward_node,
            const bool search_to_reverse_node,
            const PhantomNode &source_phantom,
            const PhantomNode &target_phantom,
            const int total_weight_to_forward,
            const int total_weight_to_reverse,
            int &new_total_weight_to_forward,
            int &new_total_weight_to_reverse,
            std::vector<NodeID> &leg_packed_path_forward,
            std::vector<NodeID> &leg_packed_path_reverse)
{
    if (search_to_forward_node)
    {
        forward_heap.Clear();
        reverse_heap.Clear();
        reverse_heap.Insert(target_phantom.forward_segment_id.id,
                            target_phantom.GetForwardWeightPlusOffset(),
                            target_phantom.forward_segment_id.id);

        if (search_from_forward_node)
        {
            forward_heap.Insert(source_phantom.forward_segment_id.id,
                                total_weight_to_forward -
                                    source_phantom.GetForwardWeightPlusOffset(),
                                source_phantom.forward_segment_id.id);
        }
        if (search_from_reverse_node)
        {
            forward_heap.Insert(source_phantom.reverse_segment_id.id,
                                total_weight_to_reverse -
                                    source_phantom.GetReverseWeightPlusOffset(),
                                source_phantom.reverse_segment_id.id);
        }

        search(engine_working_data,
               facade,
               forward_heap,
               reverse_heap,
               new_total_weight_to_forward,
               leg_packed_path_forward,
               needsLoopForward(source_phantom, target_phantom),
               routing_algorithms::DO_NOT_FORCE_LOOP,
               {source_phantom, target_phantom});
    }

    if (search_to_reverse_node)
    {
        forward_heap.Clear();
        reverse_heap.Clear();
        reverse_heap.Insert(target_phantom.reverse_segment_id.id,
                            target_phantom.GetReverseWeightPlusOffset(),
                            target_phantom.reverse_segment_id.id);
        if (search_from_forward_node)
        {
            forward_heap.Insert(source_phantom.forward_segment_id.id,
                                total_weight_to_forward -
                                    source_phantom.GetForwardWeightPlusOffset(),
                                source_phantom.forward_segment_id.id);
        }
        if (search_from_reverse_node)
        {
            forward_heap.Insert(source_phantom.reverse_segment_id.id,
                                total_weight_to_reverse -
                                    source_phantom.GetReverseWeightPlusOffset(),
                                source_phantom.reverse_segment_id.id);
        }

        search(engine_working_data,
               facade,
               forward_heap,
               reverse_heap,
               new_total_weight_to_reverse,
               leg_packed_path_reverse,
               routing_algorithms::DO_NOT_FORCE_LOOP,
               needsLoopBackwards(source_phantom, target_phantom),
               {source_phantom, target_phantom});
    }
}

template <typename Algorithm>
void unpackLegs(const DataFacade<Algorithm> &facade,
                const std::vector<PhantomNodes> &phantom_nodes_vector,
                const std::vector<NodeID> &total_packed_path,
                const std::vector<std::size_t> &packed_leg_begin,
                const EdgeWeight shortest_path_weight,
                InternalRouteResult &raw_route_data)
{
    raw_route_data.unpacked_path_segments.resize(packed_leg_begin.size() - 1);

    raw_route_data.shortest_path_weight = shortest_path_weight;

    for (const auto current_leg : util::irange<std::size_t>(0UL, packed_leg_begin.size() - 1))
    {
        auto leg_begin = total_packed_path.begin() + packed_leg_begin[current_leg];
        auto leg_end = total_packed_path.begin() + packed_leg_begin[current_leg + 1];
        const auto &unpack_phantom_node_pair = phantom_nodes_vector[current_leg];
        unpackPath(facade,
                   leg_begin,
                   leg_end,
                   unpack_phantom_node_pair,
                   raw_route_data.unpacked_path_segments[current_leg]);

        raw_route_data.source_traversed_in_reverse.push_back(
            (*leg_begin != phantom_nodes_vector[current_leg].source_phantom.forward_segment_id.id));
        raw_route_data.target_traversed_in_reverse.push_back(
            (*std::prev(leg_end) !=
             phantom_nodes_vector[current_leg].target_phantom.forward_segment_id.id));
    }
}

template <typename Algorithm>
inline void initializeHeap(SearchEngineData<Algorithm> &engine_working_data,
                           const DataFacade<Algorithm> &facade)
{

    const auto nodes_number = facade.GetNumberOfNodes();
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(nodes_number);
}

template <>
inline void initializeHeap<mld::Algorithm>(SearchEngineData<mld::Algorithm> &engine_working_data,
                                           const DataFacade<mld::Algorithm> &facade)
{

    const auto nodes_number = facade.GetNumberOfNodes();
    const auto border_nodes_number = facade.GetMaxBorderNodeID() + 1;
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(nodes_number, border_nodes_number);
}
} // namespace

template <typename Algorithm>
InternalRouteResult shortestPathSearch(SearchEngineData<Algorithm> &engine_working_data,
                                       const DataFacade<Algorithm> &facade,
                                       const std::vector<PhantomNodes> &phantom_nodes_vector,
                                       const boost::optional<bool> continue_straight_at_waypoint)
{
    InternalRouteResult raw_route_data;
    raw_route_data.segment_end_coordinates = phantom_nodes_vector;
    const bool allow_uturn_at_waypoint =
        !(continue_straight_at_waypoint ? *continue_straight_at_waypoint
                                        : facade.GetContinueStraightDefault());

    initializeHeap(engine_working_data, facade);

    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;

    int total_weight_to_forward = 0;
    int total_weight_to_reverse = 0;
    bool search_from_forward_node =
        phantom_nodes_vector.front().source_phantom.IsValidForwardSource();
    bool search_from_reverse_node =
        phantom_nodes_vector.front().source_phantom.IsValidReverseSource();

    std::vector<NodeID> prev_packed_leg_to_forward;
    std::vector<NodeID> prev_packed_leg_to_reverse;

    std::vector<NodeID> total_packed_path_to_forward;
    std::vector<std::size_t> packed_leg_to_forward_begin;
    std::vector<NodeID> total_packed_path_to_reverse;
    std::vector<std::size_t> packed_leg_to_reverse_begin;

    std::size_t current_leg = 0;
    // this implements a dynamic program that finds the shortest route through
    // a list of vias
    for (const auto &phantom_node_pair : phantom_nodes_vector)
    {
        int new_total_weight_to_forward = INVALID_EDGE_WEIGHT;
        int new_total_weight_to_reverse = INVALID_EDGE_WEIGHT;

        std::vector<NodeID> packed_leg_to_forward;
        std::vector<NodeID> packed_leg_to_reverse;

        const auto &source_phantom = phantom_node_pair.source_phantom;
        const auto &target_phantom = phantom_node_pair.target_phantom;

        bool search_to_forward_node = target_phantom.IsValidForwardTarget();
        bool search_to_reverse_node = target_phantom.IsValidReverseTarget();

        BOOST_ASSERT(!search_from_forward_node || source_phantom.IsValidForwardSource());
        BOOST_ASSERT(!search_from_reverse_node || source_phantom.IsValidReverseSource());

        if (search_to_reverse_node || search_to_forward_node)
        {
            if (allow_uturn_at_waypoint)
            {
                searchWithUTurn(engine_working_data,
                                facade,
                                forward_heap,
                                reverse_heap,
                                search_from_forward_node,
                                search_from_reverse_node,
                                search_to_forward_node,
                                search_to_reverse_node,
                                source_phantom,
                                target_phantom,
                                total_weight_to_forward,
                                total_weight_to_reverse,
                                new_total_weight_to_forward,
                                packed_leg_to_forward);
                // if only the reverse node is valid (e.g. when using the match plugin) we
                // actually need to move
                if (!target_phantom.IsValidForwardTarget())
                {
                    BOOST_ASSERT(target_phantom.IsValidReverseTarget());
                    new_total_weight_to_reverse = new_total_weight_to_forward;
                    packed_leg_to_reverse = std::move(packed_leg_to_forward);
                    new_total_weight_to_forward = INVALID_EDGE_WEIGHT;

                    // (*)
                    //
                    //   Below we have to check if new_total_weight_to_forward is invalid.
                    //   This prevents use-after-move on packed_leg_to_forward.
                }
                else if (target_phantom.IsValidReverseTarget())
                {
                    new_total_weight_to_reverse = new_total_weight_to_forward;
                    packed_leg_to_reverse = packed_leg_to_forward;
                }
            }
            else
            {
                search(engine_working_data,
                       facade,
                       forward_heap,
                       reverse_heap,
                       search_from_forward_node,
                       search_from_reverse_node,
                       search_to_forward_node,
                       search_to_reverse_node,
                       source_phantom,
                       target_phantom,
                       total_weight_to_forward,
                       total_weight_to_reverse,
                       new_total_weight_to_forward,
                       new_total_weight_to_reverse,
                       packed_leg_to_forward,
                       packed_leg_to_reverse);
            }
        }

        // Note: To make sure we do not access the moved-from packed_leg_to_forward
        // we guard its access by a check for invalid edge weight. See  (*) above.

        // No path found for both target nodes?
        if ((INVALID_EDGE_WEIGHT == new_total_weight_to_forward) &&
            (INVALID_EDGE_WEIGHT == new_total_weight_to_reverse))
        {
            return raw_route_data;
        }

        // we need to figure out how the new legs connect to the previous ones
        if (current_leg > 0)
        {
            bool forward_to_forward =
                (new_total_weight_to_forward != INVALID_EDGE_WEIGHT) &&
                packed_leg_to_forward.front() == source_phantom.forward_segment_id.id;
            bool reverse_to_forward =
                (new_total_weight_to_forward != INVALID_EDGE_WEIGHT) &&
                packed_leg_to_forward.front() == source_phantom.reverse_segment_id.id;
            bool forward_to_reverse =
                (new_total_weight_to_reverse != INVALID_EDGE_WEIGHT) &&
                packed_leg_to_reverse.front() == source_phantom.forward_segment_id.id;
            bool reverse_to_reverse =
                (new_total_weight_to_reverse != INVALID_EDGE_WEIGHT) &&
                packed_leg_to_reverse.front() == source_phantom.reverse_segment_id.id;

            BOOST_ASSERT(!forward_to_forward || !reverse_to_forward);
            BOOST_ASSERT(!forward_to_reverse || !reverse_to_reverse);

            // in this case we always need to copy
            if (forward_to_forward && forward_to_reverse)
            {
                // in this case we copy the path leading to the source forward node
                // and change the case
                total_packed_path_to_reverse = total_packed_path_to_forward;
                packed_leg_to_reverse_begin = packed_leg_to_forward_begin;
                forward_to_reverse = false;
                reverse_to_reverse = true;
            }
            else if (reverse_to_forward && reverse_to_reverse)
            {
                total_packed_path_to_forward = total_packed_path_to_reverse;
                packed_leg_to_forward_begin = packed_leg_to_reverse_begin;
                reverse_to_forward = false;
                forward_to_forward = true;
            }
            BOOST_ASSERT(!forward_to_forward || !forward_to_reverse);
            BOOST_ASSERT(!reverse_to_forward || !reverse_to_reverse);

            // in this case we just need to swap to regain the correct mapping
            if (reverse_to_forward || forward_to_reverse)
            {
                total_packed_path_to_forward.swap(total_packed_path_to_reverse);
                packed_leg_to_forward_begin.swap(packed_leg_to_reverse_begin);
            }
        }

        if (new_total_weight_to_forward != INVALID_EDGE_WEIGHT)
        {
            BOOST_ASSERT(target_phantom.IsValidForwardTarget());

            packed_leg_to_forward_begin.push_back(total_packed_path_to_forward.size());
            total_packed_path_to_forward.insert(total_packed_path_to_forward.end(),
                                                packed_leg_to_forward.begin(),
                                                packed_leg_to_forward.end());
            search_from_forward_node = true;
        }
        else
        {
            total_packed_path_to_forward.clear();
            packed_leg_to_forward_begin.clear();
            search_from_forward_node = false;
        }

        if (new_total_weight_to_reverse != INVALID_EDGE_WEIGHT)
        {
            BOOST_ASSERT(target_phantom.IsValidReverseTarget());

            packed_leg_to_reverse_begin.push_back(total_packed_path_to_reverse.size());
            total_packed_path_to_reverse.insert(total_packed_path_to_reverse.end(),
                                                packed_leg_to_reverse.begin(),
                                                packed_leg_to_reverse.end());
            search_from_reverse_node = true;
        }
        else
        {
            total_packed_path_to_reverse.clear();
            packed_leg_to_reverse_begin.clear();
            search_from_reverse_node = false;
        }

        prev_packed_leg_to_forward = std::move(packed_leg_to_forward);
        prev_packed_leg_to_reverse = std::move(packed_leg_to_reverse);

        total_weight_to_forward = new_total_weight_to_forward;
        total_weight_to_reverse = new_total_weight_to_reverse;

        ++current_leg;
    }

    BOOST_ASSERT(total_weight_to_forward != INVALID_EDGE_WEIGHT ||
                 total_weight_to_reverse != INVALID_EDGE_WEIGHT);

    // We make sure the fastest route is always in packed_legs_to_forward
    if (total_weight_to_forward < total_weight_to_reverse ||
        (total_weight_to_forward == total_weight_to_reverse &&
         total_packed_path_to_forward.size() < total_packed_path_to_reverse.size()))
    {
        // insert sentinel
        packed_leg_to_forward_begin.push_back(total_packed_path_to_forward.size());
        BOOST_ASSERT(packed_leg_to_forward_begin.size() == phantom_nodes_vector.size() + 1);

        unpackLegs(facade,
                   phantom_nodes_vector,
                   total_packed_path_to_forward,
                   packed_leg_to_forward_begin,
                   total_weight_to_forward,
                   raw_route_data);
    }
    else
    {
        // insert sentinel
        packed_leg_to_reverse_begin.push_back(total_packed_path_to_reverse.size());
        BOOST_ASSERT(packed_leg_to_reverse_begin.size() == phantom_nodes_vector.size() + 1);

        unpackLegs(facade,
                   phantom_nodes_vector,
                   total_packed_path_to_reverse,
                   packed_leg_to_reverse_begin,
                   total_weight_to_reverse,
                   raw_route_data);
    }

    return raw_route_data;
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* OSRM_SHORTEST_PATH_IMPL_HPP */
