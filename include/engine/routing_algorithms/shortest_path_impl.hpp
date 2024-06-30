#ifndef OSRM_SHORTEST_PATH_IMPL_HPP
#define OSRM_SHORTEST_PATH_IMPL_HPP

#include "engine/routing_algorithms/shortest_path.hpp"

#include <boost/assert.hpp>
#include <optional>

namespace osrm::engine::routing_algorithms
{

namespace
{
const size_t INVALID_LEG_INDEX = std::numeric_limits<size_t>::max();

template <typename Algorithm>
void searchWithUTurn(SearchEngineData<Algorithm> &engine_working_data,
                     const DataFacade<Algorithm> &facade,
                     typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                     typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                     const PhantomEndpointCandidates &candidates,
                     EdgeWeight &leg_weight,
                     std::vector<NodeID> &leg_packed_path)
{
    forward_heap.Clear();
    reverse_heap.Clear();

    for (const auto &source : candidates.source_phantoms)
    {
        if (source.IsValidForwardSource())
        {
            forward_heap.Insert(source.forward_segment_id.id,
                                EdgeWeight{0} - source.GetForwardWeightPlusOffset(),
                                source.forward_segment_id.id);
        }

        if (source.IsValidReverseSource())
        {
            forward_heap.Insert(source.reverse_segment_id.id,
                                EdgeWeight{0} - source.GetReverseWeightPlusOffset(),
                                source.reverse_segment_id.id);
        }
    }
    for (const auto &target : candidates.target_phantoms)
    {
        if (target.IsValidForwardTarget())
        {
            reverse_heap.Insert(target.forward_segment_id.id,
                                target.GetForwardWeightPlusOffset(),
                                target.forward_segment_id.id);
        }
        if (target.IsValidReverseTarget())
        {
            reverse_heap.Insert(target.reverse_segment_id.id,
                                target.GetReverseWeightPlusOffset(),
                                target.reverse_segment_id.id);
        }
    }

    search(engine_working_data,
           facade,
           forward_heap,
           reverse_heap,
           leg_weight,
           leg_packed_path,
           {},
           candidates);
}

template <typename Algorithm>
void search(SearchEngineData<Algorithm> &engine_working_data,
            const DataFacade<Algorithm> &facade,
            typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
            typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
            const std::vector<bool> &search_from_forward_node,
            const std::vector<bool> &search_from_reverse_node,
            const PhantomCandidatesToTarget &candidates,
            const std::vector<EdgeWeight> &total_weight_to_forward,
            const std::vector<EdgeWeight> &total_weight_to_reverse,
            EdgeWeight &new_total_weight_to_forward,
            EdgeWeight &new_total_weight_to_reverse,
            std::vector<NodeID> &leg_packed_path_forward,
            std::vector<NodeID> &leg_packed_path_reverse)
{
    // We want to find the shortest distance from any of the source candidate segments to this
    // specific target.
    const auto &source_candidates = candidates.source_phantoms;
    const auto &target = candidates.target_phantom;
    BOOST_ASSERT(search_from_forward_node.size() == source_candidates.size());
    BOOST_ASSERT(search_from_reverse_node.size() == source_candidates.size());

    if (target.IsValidForwardTarget())
    {
        forward_heap.Clear();
        reverse_heap.Clear();
        reverse_heap.Insert(target.forward_segment_id.id,
                            target.GetForwardWeightPlusOffset(),
                            target.forward_segment_id.id);

        for (const auto i : util::irange<std::size_t>(0UL, source_candidates.size()))
        {
            const auto &candidate = source_candidates[i];
            if (search_from_forward_node[i] && candidate.IsValidForwardSource())
            {
                forward_heap.Insert(candidate.forward_segment_id.id,
                                    total_weight_to_forward[i] -
                                        candidate.GetForwardWeightPlusOffset(),
                                    candidate.forward_segment_id.id);
            }

            if (search_from_reverse_node[i] && candidate.IsValidReverseSource())
            {
                forward_heap.Insert(candidate.reverse_segment_id.id,
                                    total_weight_to_reverse[i] -
                                        candidate.GetReverseWeightPlusOffset(),
                                    candidate.reverse_segment_id.id);
            }
        }

        search(engine_working_data,
               facade,
               forward_heap,
               reverse_heap,
               new_total_weight_to_forward,
               leg_packed_path_forward,
               getForwardForceNodes(candidates),
               candidates);
    }

    if (target.IsValidReverseTarget())
    {
        forward_heap.Clear();
        reverse_heap.Clear();
        reverse_heap.Insert(target.reverse_segment_id.id,
                            target.GetReverseWeightPlusOffset(),
                            target.reverse_segment_id.id);

        BOOST_ASSERT(search_from_forward_node.size() == source_candidates.size());
        for (const auto i : util::irange<std::size_t>(0UL, source_candidates.size()))
        {
            const auto &candidate = source_candidates[i];
            if (search_from_forward_node[i] && candidate.IsValidForwardSource())
            {
                forward_heap.Insert(candidate.forward_segment_id.id,
                                    total_weight_to_forward[i] -
                                        candidate.GetForwardWeightPlusOffset(),
                                    candidate.forward_segment_id.id);
            }

            if (search_from_reverse_node[i] && candidate.IsValidReverseSource())
            {
                forward_heap.Insert(candidate.reverse_segment_id.id,
                                    total_weight_to_reverse[i] -
                                        candidate.GetReverseWeightPlusOffset(),
                                    candidate.reverse_segment_id.id);
            }
        }

        search(engine_working_data,
               facade,
               forward_heap,
               reverse_heap,
               new_total_weight_to_reverse,
               leg_packed_path_reverse,
               getBackwardForceNodes(candidates),
               candidates);
    }
}

template <typename Algorithm>
void unpackLegs(const DataFacade<Algorithm> &facade,
                const std::vector<PhantomEndpoints> &leg_endpoints,
                const std::vector<size_t> &route_path_indices,
                const std::vector<NodeID> &total_packed_path,
                const std::vector<std::size_t> &packed_leg_begin,
                const EdgeWeight shortest_path_weight,
                InternalRouteResult &raw_route_data)
{
    raw_route_data.unpacked_path_segments.resize(route_path_indices.size());

    raw_route_data.shortest_path_weight = shortest_path_weight;

    for (const auto current_leg : util::irange<std::size_t>(0UL, route_path_indices.size()))
    {
        auto leg_begin =
            total_packed_path.begin() + packed_leg_begin[route_path_indices[current_leg]];
        auto leg_end =
            total_packed_path.begin() + packed_leg_begin[route_path_indices[current_leg] + 1];
        const auto &unpack_phantom_node_pair = leg_endpoints[current_leg];
        unpackPath(facade,
                   leg_begin,
                   leg_end,
                   unpack_phantom_node_pair,
                   raw_route_data.unpacked_path_segments[current_leg]);

        raw_route_data.source_traversed_in_reverse.push_back(
            (*leg_begin != leg_endpoints[current_leg].source_phantom.forward_segment_id.id));
        raw_route_data.target_traversed_in_reverse.push_back(
            (*std::prev(leg_end) !=
             leg_endpoints[current_leg].target_phantom.forward_segment_id.id));
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

template <typename Algorithm>
InternalRouteResult
constructRouteResult(const DataFacade<Algorithm> &facade,
                     const std::vector<PhantomNodeCandidates> &waypoint_candidates,
                     const std::vector<std::size_t> &route_path_indices,
                     const std::vector<NodeID> &packed_paths,
                     const std::vector<size_t> &packed_path_begin,
                     const EdgeWeight min_weight)
{

    InternalRouteResult raw_route_data;
    // Find the start/end phantom endpoints
    std::vector<PhantomEndpoints> path_endpoints;
    for (const auto i : util::irange<std::size_t>(0UL, waypoint_candidates.size() - 1))
    {
        const auto &source_candidates = waypoint_candidates[i];
        const auto &target_candidates = waypoint_candidates[i + 1];
        const auto path_index = route_path_indices[i];
        const auto start_node = packed_paths[packed_path_begin[path_index]];
        const auto end_node = packed_paths[packed_path_begin[path_index + 1] - 1];

        auto source_it =
            std::find_if(source_candidates.begin(),
                         source_candidates.end(),
                         [&start_node](const auto &source_phantom)
                         {
                             return (start_node == source_phantom.forward_segment_id.id ||
                                     start_node == source_phantom.reverse_segment_id.id);
                         });
        BOOST_ASSERT(source_it != source_candidates.end());

        auto target_it =
            std::find_if(target_candidates.begin(),
                         target_candidates.end(),
                         [&end_node](const auto &target_phantom)
                         {
                             return (end_node == target_phantom.forward_segment_id.id ||
                                     end_node == target_phantom.reverse_segment_id.id);
                         });
        BOOST_ASSERT(target_it != target_candidates.end());

        path_endpoints.push_back({*source_it, *target_it});
    };
    raw_route_data.leg_endpoints = path_endpoints;

    unpackLegs(facade,
               path_endpoints,
               route_path_indices,
               packed_paths,
               packed_path_begin,
               min_weight,
               raw_route_data);

    return raw_route_data;
}

// Allows a uturn at the waypoints.
// Given that all candidates for a waypoint have the same location,
// this allows us to just find the shortest path from any of the source to any of the targets.
template <typename Algorithm>
InternalRouteResult
shortestPathWithWaypointUTurns(SearchEngineData<Algorithm> &engine_working_data,
                               const DataFacade<Algorithm> &facade,
                               const std::vector<PhantomNodeCandidates> &waypoint_candidates)
{

    EdgeWeight total_weight = {0};
    std::vector<NodeID> total_packed_path;
    std::vector<std::size_t> packed_leg_begin;

    initializeHeap(engine_working_data, facade);

    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;

    for (const auto i : util::irange<std::size_t>(0UL, waypoint_candidates.size() - 1))
    {
        PhantomEndpointCandidates search_candidates{waypoint_candidates[i],
                                                    waypoint_candidates[i + 1]};
        std::vector<NodeID> packed_leg;
        EdgeWeight leg_weight = INVALID_EDGE_WEIGHT;

        // We have a valid path up to this leg
        BOOST_ASSERT(total_weight != INVALID_EDGE_WEIGHT);
        searchWithUTurn(engine_working_data,
                        facade,
                        forward_heap,
                        reverse_heap,
                        search_candidates,
                        leg_weight,
                        packed_leg);

        if (leg_weight == INVALID_EDGE_WEIGHT)
            return {};

        packed_leg_begin.push_back(total_packed_path.size());
        total_packed_path.insert(total_packed_path.end(), packed_leg.begin(), packed_leg.end());
        total_weight += leg_weight;
    };

    // Add sentinel
    packed_leg_begin.push_back(total_packed_path.size());

    BOOST_ASSERT(packed_leg_begin.size() == waypoint_candidates.size());

    std::vector<std::size_t> sequential_indices(packed_leg_begin.size() - 1);
    std::iota(sequential_indices.begin(), sequential_indices.end(), 0);
    return constructRouteResult(facade,
                                waypoint_candidates,
                                sequential_indices,
                                total_packed_path,
                                packed_leg_begin,
                                total_weight);
}

struct leg_connections
{
    // X_to_Y = i can be read as
    // sources[i].X is the source of the shortest leg path to target.Y
    std::optional<size_t> forward_to_forward;
    std::optional<size_t> reverse_to_forward;
    std::optional<size_t> forward_to_reverse;
    std::optional<size_t> reverse_to_reverse;
};

// Identify which of the source candidates segments is being used for paths to the
// forward and reverse segment targets.
leg_connections getLegConnections(const PhantomNodeCandidates &source_candidates,
                                  const std::vector<NodeID> &packed_leg_to_forward,
                                  const std::vector<NodeID> &packed_leg_to_reverse,
                                  const EdgeWeight new_total_weight_to_forward,
                                  const EdgeWeight new_total_weight_to_reverse)
{
    leg_connections connections;
    for (const auto i : util::irange<std::size_t>(0UL, source_candidates.size()))
    {
        const auto &candidate = source_candidates[i];

        if ((new_total_weight_to_forward != INVALID_EDGE_WEIGHT) &&
            candidate.IsValidForwardSource() &&
            packed_leg_to_forward.front() == candidate.forward_segment_id.id)
        {
            BOOST_ASSERT(!connections.forward_to_forward && !connections.reverse_to_forward);
            connections.forward_to_forward = i;
        }
        else if ((new_total_weight_to_forward != INVALID_EDGE_WEIGHT) &&
                 candidate.IsValidReverseSource() &&
                 packed_leg_to_forward.front() == candidate.reverse_segment_id.id)
        {
            BOOST_ASSERT(!connections.forward_to_forward && !connections.reverse_to_forward);
            connections.reverse_to_forward = i;
        }

        if ((new_total_weight_to_reverse != INVALID_EDGE_WEIGHT) &&
            candidate.IsValidForwardSource() &&
            packed_leg_to_reverse.front() == candidate.forward_segment_id.id)
        {
            BOOST_ASSERT(!connections.forward_to_reverse && !connections.reverse_to_reverse);
            connections.forward_to_reverse = i;
        }
        else if ((new_total_weight_to_reverse != INVALID_EDGE_WEIGHT) &&
                 candidate.IsValidReverseSource() &&
                 packed_leg_to_reverse.front() == candidate.reverse_segment_id.id)
        {
            BOOST_ASSERT(!connections.forward_to_reverse && !connections.reverse_to_reverse);
            connections.reverse_to_reverse = i;
        }
    }
    return connections;
}

struct leg_state
{
    // routability to target
    std::vector<bool> reached_forward_node_target;
    std::vector<bool> reached_reverse_node_target;
    // total weight from route start up to and including this leg
    std::vector<EdgeWeight> total_weight_to_forward;
    std::vector<EdgeWeight> total_weight_to_reverse;
    // total nodes from route start up to and including this leg
    std::vector<std::size_t> total_nodes_to_forward;
    std::vector<std::size_t> total_nodes_to_reverse;

    void reset()
    {
        reached_forward_node_target.clear();
        reached_reverse_node_target.clear();
        total_weight_to_forward.clear();
        total_weight_to_reverse.clear();
        total_nodes_to_forward.clear();
        total_nodes_to_reverse.clear();
    }
};

struct route_state
{
    /**
     * To avoid many small vectors or lots of copying, we use a single vector to track all
     * possible route leg paths and combine ths with offset and back link vectors to
     * reconstruct the full shortest path once the search is complete.
     *  path_0 -> path_1 ->...-> path_m-1 -> path_m
     *         \
     *           > path_2
     *                            --path_0--  --path_1-- --path_2-- ....... -path_m-1- --path_m--
     *                           |           |          |          |       |          |          |
     *  total_packed_path        [n0,n1,.....,na,na+1,..,nb,nb+1,..,.......,nx,.......,ny......nz]
     *
     *  packed_leg_path_begin    [0,a,b,...,x,y,z+1]
     *
     *  previous_leg_in_route    [-1,0,0,...,..,m-1]
     */
    std::vector<NodeID> total_packed_paths;
    std::vector<std::size_t> packed_leg_begin;
    std::vector<std::size_t> previous_leg_in_route;

    leg_state last;
    leg_state current;

    size_t current_leg;

    /**
     *  Given the current state after leg n:
     *                           | leg_0_paths | leg_1_paths | ... | leg_n-1_paths | leg_n_paths |
     *  packed_leg_begin         [...............................................................]
     *  previous_leg_in_route    [...............................................................]
     *                                                                             ^
     *                                                                    previous_leg_path_offset
     *
     *  We want to link back to leg_n paths from leg n+1 paths to represent the routes taken.
     *
     *  We know that the target candidates of leg n are the source candidates of leg n+1, so as long
     *  as we store the path vector data in the same order as the waypoint candidates,
     *  we can combine an offset into the path vectors and the candidate index to link back to a
     *  path from a previous leg.
     **/
    size_t previous_leg_path_offset;

    route_state(const PhantomNodeCandidates &init_candidates)
        : current_leg(0), previous_leg_path_offset(0)
    {
        last.total_weight_to_forward.resize(init_candidates.size(), {0});
        last.total_weight_to_reverse.resize(init_candidates.size(), {0});
        // Initialize routability from source validity.
        std::transform(init_candidates.begin(),
                       init_candidates.end(),
                       std::back_inserter(last.reached_forward_node_target),
                       [](const PhantomNode &phantom_node)
                       { return phantom_node.IsValidForwardSource(); });
        std::transform(init_candidates.begin(),
                       init_candidates.end(),
                       std::back_inserter(last.reached_reverse_node_target),
                       [](const PhantomNode &phantom_node)
                       { return phantom_node.IsValidReverseSource(); });
    }

    bool completeLeg()
    {
        std::swap(current, last);
        // Reset current state
        current.reset();

        current_leg++;
        previous_leg_path_offset =
            previous_leg_in_route.size() - 2 * last.total_weight_to_forward.size();

        auto can_reach_leg_forward = std::any_of(last.total_weight_to_forward.begin(),
                                                 last.total_weight_to_forward.end(),
                                                 [](auto v) { return v != INVALID_EDGE_WEIGHT; });
        auto can_reach_leg_reverse = std::any_of(last.total_weight_to_reverse.begin(),
                                                 last.total_weight_to_reverse.end(),
                                                 [](auto v) { return v != INVALID_EDGE_WEIGHT; });

        return can_reach_leg_forward || can_reach_leg_reverse;
    }

    void completeSearch()
    {
        // insert sentinel
        packed_leg_begin.push_back(total_packed_paths.size());
        BOOST_ASSERT(packed_leg_begin.size() == previous_leg_in_route.size() + 1);
    }

    size_t previousForwardPath(size_t previous_leg) const
    {
        return previous_leg_path_offset + 2 * previous_leg;
    }

    size_t previousReversePath(size_t previous_leg) const
    {
        return previous_leg_path_offset + 2 * previous_leg + 1;
    }

    void addSearchResult(const PhantomCandidatesToTarget &candidates,
                         const std::vector<NodeID> &packed_leg_to_forward,
                         const std::vector<NodeID> &packed_leg_to_reverse,
                         EdgeWeight new_total_weight_to_forward,
                         EdgeWeight new_total_weight_to_reverse)
    {

        // we need to figure out how the new legs connect to the previous ones
        if (current_leg > 0)
        {
            const auto leg_connections = getLegConnections(candidates.source_phantoms,
                                                           packed_leg_to_forward,
                                                           packed_leg_to_reverse,
                                                           new_total_weight_to_forward,
                                                           new_total_weight_to_reverse);

            // Make the back link connections between the current and previous legs.
            if (leg_connections.forward_to_forward)
            {
                auto new_total = last.total_nodes_to_forward[*leg_connections.forward_to_forward] +
                                 packed_leg_to_forward.size();
                current.total_nodes_to_forward.push_back(new_total);
                previous_leg_in_route.push_back(
                    previousForwardPath(*leg_connections.forward_to_forward));
            }
            else if (leg_connections.reverse_to_forward)
            {
                auto new_total = last.total_nodes_to_reverse[*leg_connections.reverse_to_forward] +
                                 packed_leg_to_forward.size();
                current.total_nodes_to_forward.push_back(new_total);
                previous_leg_in_route.push_back(
                    previousReversePath(*leg_connections.reverse_to_forward));
            }
            else
            {
                BOOST_ASSERT(new_total_weight_to_forward == INVALID_EDGE_WEIGHT);
                current.total_nodes_to_forward.push_back(0);
                previous_leg_in_route.push_back(INVALID_LEG_INDEX);
            }

            if (leg_connections.forward_to_reverse)
            {
                auto new_total = last.total_nodes_to_forward[*leg_connections.forward_to_reverse] +
                                 packed_leg_to_reverse.size();
                current.total_nodes_to_reverse.push_back(new_total);
                previous_leg_in_route.push_back(
                    previousForwardPath(*leg_connections.forward_to_reverse));
            }
            else if (leg_connections.reverse_to_reverse)
            {
                auto new_total = last.total_nodes_to_reverse[*leg_connections.reverse_to_reverse] +
                                 packed_leg_to_reverse.size();
                current.total_nodes_to_reverse.push_back(new_total);
                previous_leg_in_route.push_back(
                    previousReversePath(*leg_connections.reverse_to_reverse));
            }
            else
            {
                current.total_nodes_to_reverse.push_back(0);
                previous_leg_in_route.push_back(INVALID_LEG_INDEX);
            }
        }
        else
        {
            previous_leg_in_route.push_back(INVALID_LEG_INDEX);
            current.total_nodes_to_forward.push_back(packed_leg_to_forward.size());

            previous_leg_in_route.push_back(INVALID_LEG_INDEX);
            current.total_nodes_to_reverse.push_back(packed_leg_to_reverse.size());
        }

        // Update route paths, weights and reachability
        BOOST_ASSERT(new_total_weight_to_forward == INVALID_EDGE_WEIGHT ||
                     candidates.target_phantom.IsValidForwardTarget());
        current.total_weight_to_forward.push_back(new_total_weight_to_forward);
        current.reached_forward_node_target.push_back(new_total_weight_to_forward !=
                                                      INVALID_EDGE_WEIGHT);

        BOOST_ASSERT(new_total_weight_to_reverse == INVALID_EDGE_WEIGHT ||
                     candidates.target_phantom.IsValidReverseTarget());
        current.total_weight_to_reverse.push_back(new_total_weight_to_reverse);
        current.reached_reverse_node_target.push_back(new_total_weight_to_reverse !=
                                                      INVALID_EDGE_WEIGHT);

        packed_leg_begin.push_back(total_packed_paths.size());
        total_packed_paths.insert(
            total_packed_paths.end(), packed_leg_to_forward.begin(), packed_leg_to_forward.end());
        packed_leg_begin.push_back(total_packed_paths.size());
        total_packed_paths.insert(
            total_packed_paths.end(), packed_leg_to_reverse.begin(), packed_leg_to_reverse.end());
    }

    // Find the final target with the shortest route and backtrack through the legs to find the
    // paths that create this route.
    std::pair<std::vector<size_t>, EdgeWeight> getMinRoute()
    {
        // Find the segment from final leg with the shortest path
        auto forward_range = util::irange<std::size_t>(0UL, last.total_weight_to_forward.size());
        auto forward_min = std::min_element(
            forward_range.begin(),
            forward_range.end(),
            [&](size_t a, size_t b)
            {
                return (last.total_weight_to_forward[a] < last.total_weight_to_forward[b] ||
                        (last.total_weight_to_forward[a] == last.total_weight_to_forward[b] &&
                         last.total_nodes_to_forward[a] < last.total_nodes_to_forward[b]));
            });
        auto reverse_range = util::irange<std::size_t>(0UL, last.total_weight_to_reverse.size());
        auto reverse_min = std::min_element(
            reverse_range.begin(),
            reverse_range.end(),
            [&](size_t a, size_t b)
            {
                return (last.total_weight_to_reverse[a] < last.total_weight_to_reverse[b] ||
                        (last.total_weight_to_reverse[a] == last.total_weight_to_reverse[b] &&
                         last.total_nodes_to_reverse[a] < last.total_nodes_to_reverse[b]));
            });

        auto min_weight = INVALID_EDGE_WEIGHT;
        std::vector<size_t> path_indices;
        if (last.total_weight_to_forward[*forward_min] <
                last.total_weight_to_reverse[*reverse_min] ||
            (last.total_weight_to_forward[*forward_min] ==
                 last.total_weight_to_reverse[*reverse_min] &&
             last.total_nodes_to_forward[*forward_min] < last.total_nodes_to_reverse[*reverse_min]))
        {
            // Get path indices for forward
            auto current_path_index = previousForwardPath(*forward_min);
            path_indices.push_back(current_path_index);
            while (previous_leg_in_route[current_path_index] != INVALID_LEG_INDEX)
            {
                current_path_index = previous_leg_in_route[current_path_index];
                path_indices.push_back(current_path_index);
            }
            min_weight = last.total_weight_to_forward[*forward_min];
        }
        else
        {
            // Get path indices for reverse
            auto current_path_index = previousReversePath(*reverse_min);
            path_indices.push_back(current_path_index);
            while (previous_leg_in_route[current_path_index] != INVALID_LEG_INDEX)
            {
                current_path_index = previous_leg_in_route[current_path_index];
                path_indices.push_back(current_path_index);
            }
            min_weight = last.total_weight_to_reverse[*reverse_min];
        }

        std::reverse(path_indices.begin(), path_indices.end());
        return std::make_pair(std::move(path_indices), min_weight);
    }
};

// Requires segment continuation at a waypoint.
// In this case we need to track paths to each of the waypoint candidate forward/reverse segments,
// as each of them could be a leg in route shortest path.
template <typename Algorithm>
InternalRouteResult
shortestPathWithWaypointContinuation(SearchEngineData<Algorithm> &engine_working_data,
                                     const DataFacade<Algorithm> &facade,
                                     const std::vector<PhantomNodeCandidates> &waypoint_candidates)
{

    route_state route(waypoint_candidates.front());

    initializeHeap(engine_working_data, facade);
    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;

    // this implements a dynamic program that finds the shortest route through
    // a list of leg endpoints.
    for (const auto i : util::irange<std::size_t>(0UL, waypoint_candidates.size() - 1))
    {
        const auto &source_candidates = waypoint_candidates[i];
        const auto &target_candidates = waypoint_candidates[i + 1];
        // We assume each source candidate for this leg was a target candidate from the previous
        // leg, and in the same order.
        BOOST_ASSERT(source_candidates.size() == route.last.reached_forward_node_target.size());
        BOOST_ASSERT(source_candidates.size() == route.last.reached_reverse_node_target.size());

        // We only perform the search for this leg if we reached this waypoint.
        // Note that the waypoint can be a valid target, but still not a valid source
        // (e.g. if an edge weight is set to be invalid after the phantom node on the way),
        // so this doesn't guarantee that it can be used as a source for this leg.
        BOOST_ASSERT(i == 0 ||
                     std::any_of(route.last.reached_forward_node_target.begin(),
                                 route.last.reached_forward_node_target.end(),
                                 [](auto v) { return v; }) ||
                     std::any_of(route.last.reached_reverse_node_target.begin(),
                                 route.last.reached_reverse_node_target.end(),
                                 [](auto v) { return v; }));

        for (const auto &target_phantom : target_candidates)
        {
            PhantomCandidatesToTarget search_candidates{source_candidates, target_phantom};
            EdgeWeight new_total_weight_to_forward = INVALID_EDGE_WEIGHT;
            EdgeWeight new_total_weight_to_reverse = INVALID_EDGE_WEIGHT;

            std::vector<NodeID> packed_leg_to_forward;
            std::vector<NodeID> packed_leg_to_reverse;

            if (target_phantom.IsValidForwardTarget() || target_phantom.IsValidReverseTarget())
            {
                search(engine_working_data,
                       facade,
                       forward_heap,
                       reverse_heap,
                       route.last.reached_forward_node_target,
                       route.last.reached_reverse_node_target,
                       search_candidates,
                       route.last.total_weight_to_forward,
                       route.last.total_weight_to_reverse,
                       new_total_weight_to_forward,
                       new_total_weight_to_reverse,
                       packed_leg_to_forward,
                       packed_leg_to_reverse);
            }

            route.addSearchResult(search_candidates,
                                  packed_leg_to_forward,
                                  packed_leg_to_reverse,
                                  new_total_weight_to_forward,
                                  new_total_weight_to_reverse);
        }

        auto has_valid_path = route.completeLeg();
        // No path found for both target nodes?
        if (!has_valid_path)
            return {};
    };

    BOOST_ASSERT(std::any_of(route.last.total_weight_to_forward.begin(),
                             route.last.total_weight_to_forward.end(),
                             [](const auto weight) { return weight != INVALID_EDGE_WEIGHT; }) ||
                 std::any_of(route.last.total_weight_to_reverse.begin(),
                             route.last.total_weight_to_reverse.end(),
                             [](const auto weight) { return weight != INVALID_EDGE_WEIGHT; }));

    route.completeSearch();
    std::vector<size_t> min_path_indices;
    EdgeWeight min_weight;
    std::tie(min_path_indices, min_weight) = route.getMinRoute();
    BOOST_ASSERT(min_path_indices.size() + 1 == waypoint_candidates.size());

    return constructRouteResult(facade,
                                waypoint_candidates,
                                min_path_indices,
                                route.total_packed_paths,
                                route.packed_leg_begin,
                                min_weight);
}
} // namespace

template <typename Algorithm>
InternalRouteResult
shortestPathSearch(SearchEngineData<Algorithm> &engine_working_data,
                   const DataFacade<Algorithm> &facade,
                   const std::vector<PhantomNodeCandidates> &waypoint_candidates,
                   const std::optional<bool> continue_straight_at_waypoint)
{
    const bool allow_uturn_at_waypoint =
        !(continue_straight_at_waypoint ? *continue_straight_at_waypoint
                                        : facade.GetContinueStraightDefault());

    if (allow_uturn_at_waypoint)
    {
        return shortestPathWithWaypointUTurns(engine_working_data, facade, waypoint_candidates);
    }
    else
    {
        return shortestPathWithWaypointContinuation(
            engine_working_data, facade, waypoint_candidates);
    }
}

} // namespace osrm::engine::routing_algorithms

#endif /* OSRM_SHORTEST_PATH_IMPL_HPP */
