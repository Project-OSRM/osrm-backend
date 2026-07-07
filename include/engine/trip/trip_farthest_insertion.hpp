#ifndef TRIP_FARTHEST_INSERTION_HPP
#define TRIP_FARTHEST_INSERTION_HPP

#include "util/dist_table_wrapper.hpp"
#include "util/typedefs.hpp"

#include "osrm/json_container.hpp"
#include <boost/assert.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>

namespace osrm::engine::trip
{

// given a route and a new location, find the best place of insertion and
// check the distance of roundtrip when the new location is additionally visited
using NodeIDIter = std::vector<NodeID>::iterator;
inline std::pair<EdgeDuration, NodeIDIter>
GetShortestRoundTrip(const NodeID new_loc,
                     const util::DistTableWrapper<EdgeDuration> &dist_table,
                     const std::size_t number_of_locations,
                     std::vector<NodeID> &route)
{
    (void)number_of_locations; // unused

    auto min_trip_distance = INVALID_EDGE_DURATION;
    NodeIDIter next_insert_point_candidate;

    // for all nodes in the current trip find the best insertion resulting in the shortest path
    // assert min 2 nodes in route
    const auto start = std::begin(route);
    const auto end = std::end(route);
    for (auto from_node = start; from_node != end; ++from_node)
    {
        auto to_node = std::next(from_node);
        if (to_node == end)
        {
            to_node = start;
        }

        const auto dist_from = dist_table(*from_node, new_loc);
        const auto dist_to = dist_table(new_loc, *to_node);
        // If the edge_weight is very large (INVALID_EDGE_DURATION) then the algorithm will not
        // choose this edge in final minimal path. So instead of computing all the permutations
        // after this large edge, discard this edge right here and don't consider the path after
        // this edge.
        if (dist_from == INVALID_EDGE_DURATION || dist_to == INVALID_EDGE_DURATION)
            continue;

        const auto trip_dist = dist_from + dist_to - dist_table(*from_node, *to_node);

        // This is not neccessarily true:
        // Lets say you have an edge (u, v) with duration 100. If you place a coordinate exactly in
        // the middle of the segment yielding (u, v'), the adjusted duration will be 100 * 0.5 = 50.
        // Now imagine two coordinates. One placed at 0.99 and one at 0.999. This means (u, v') now
        // has a duration of 100 * 0.99 = 99, but (u, v'') also has a duration of 100 * 0.995 = 99.
        // In which case (v', v'') has a duration of 0.
        // BOOST_ASSERT_MSG(trip_dist >= 0, "previous trip was not minimal. something's wrong");

        // from all possible insertions to the current trip, choose the shortest of all insertions
        if (trip_dist < min_trip_distance)
        {
            min_trip_distance = trip_dist;
            next_insert_point_candidate = to_node;
        }
    }
    BOOST_ASSERT_MSG(min_trip_distance != INVALID_EDGE_DURATION, "trip has invalid edge weight");

    return std::make_pair(min_trip_distance, next_insert_point_candidate);
}

// given two initial start nodes, find a roundtrip route using the farthest insertion algorithm
inline std::vector<NodeID> FindRoute(const std::size_t &number_of_locations,
                                     const util::DistTableWrapper<EdgeDuration> &dist_table,
                                     const NodeID &start1,
                                     const NodeID &start2)
{
    BOOST_ASSERT_MSG(number_of_locations * number_of_locations == dist_table.size(),
                     "number_of_locations and dist_table size do not match");

    std::vector<NodeID> route;
    route.reserve(number_of_locations);

    // tracks which nodes have been already visited
    std::vector<bool> visited(number_of_locations, false);

    visited[start1] = true;
    visited[start2] = true;
    route.push_back(start1);
    route.push_back(start2);

    // two nodes are already in the initial start trip, so we need to add all other nodes
    for (std::size_t added_nodes = 2; added_nodes < number_of_locations; ++added_nodes)
    {
        auto farthest_distance = EdgeDuration{std::numeric_limits<EdgeDuration::value_type>::min()};
        auto next_node = -1;
        NodeIDIter next_insert_point;

        // find unvisited node that is the farthest away from all other visited locs
        for (std::size_t id = 0; id < number_of_locations; ++id)
        {
            // find the shortest distance from i to all visited nodes
            if (!visited[id])
            {
                const auto insert_candidate =
                    GetShortestRoundTrip(id, dist_table, number_of_locations, route);

                BOOST_ASSERT_MSG(insert_candidate.first != INVALID_EDGE_DURATION,
                                 "shortest round trip is invalid");

                // add the location to the current trip such that it results in the shortest total
                // tour
                if (insert_candidate.first > farthest_distance)
                {
                    farthest_distance = insert_candidate.first;
                    next_node = id;
                    next_insert_point = insert_candidate.second;
                }
            }
        }

        BOOST_ASSERT_MSG(next_node >= 0, "next node to visit is invalid");

        // mark as visited and insert node
        visited[next_node] = true;
        route.insert(next_insert_point, next_node);
    }
    return route;
}

inline std::vector<NodeID> TwoOptTrip(std::vector<NodeID> route,
                                      const util::DistTableWrapper<EdgeDuration> &dist_table)
{
    if (route.size() < 4)
        return route;

    bool improved = true;
    while (improved)
    {
        improved = false;
        const auto route_size = route.size();
        std::vector<EdgeDuration> forward(route_size);
        std::vector<std::int64_t> prefix_delta(route_size + 1, 0);
        std::vector<std::size_t> prefix_invalid_reverse(route_size + 1, 0);

        // Precompute edge costs and prefix sums for directed 2-opt delta in O(1) per candidate.
        for (std::size_t edge = 0; edge < route_size; ++edge)
        {
            const auto from = route[edge];
            const auto to = route[(edge + 1) % route_size];
            const auto edge_forward = dist_table(from, to);
            if (edge_forward == INVALID_EDGE_DURATION)
            {
                return route;
            }

            const auto edge_reverse = dist_table(to, from);

            forward[edge] = edge_forward;
            prefix_delta[edge + 1] = prefix_delta[edge];
            prefix_invalid_reverse[edge + 1] = prefix_invalid_reverse[edge];

            if (edge_reverse == INVALID_EDGE_DURATION)
            {
                ++prefix_invalid_reverse[edge + 1];
            }
            else
            {
                prefix_delta[edge + 1] +=
                    static_cast<std::int64_t>(static_cast<EdgeDuration::value_type>(edge_reverse)) -
                    static_cast<std::int64_t>(static_cast<EdgeDuration::value_type>(edge_forward));
            }
        }

        for (std::size_t i = 0; i + 2 < route_size && !improved; ++i)
        {
            const auto a = route[i];
            const auto b = route[(i + 1) % route_size];

            for (std::size_t k = i + 2; k < route_size; ++k)
            {
                if (i == 0 && k + 1 == route_size)
                    continue;

                const auto c = route[k];
                const auto d = route[(k + 1) % route_size];
                const auto ab = forward[i];
                const auto cd = forward[k];
                const auto ac = dist_table(a, c);
                const auto bd = dist_table(b, d);

                if (ac == INVALID_EDGE_DURATION || bd == INVALID_EDGE_DURATION)
                {
                    continue;
                }

                // Reversing the segment [i+1, k] turns each interior edge (u->v) into (v->u).
                const auto invalid_reverse_edges =
                    prefix_invalid_reverse[k] - prefix_invalid_reverse[i + 1];
                if (invalid_reverse_edges > 0)
                {
                    continue;
                }

                const auto interior_delta = prefix_delta[k] - prefix_delta[i + 1];
                const auto boundary_delta =
                    static_cast<std::int64_t>(static_cast<EdgeDuration::value_type>(ac)) +
                    static_cast<std::int64_t>(static_cast<EdgeDuration::value_type>(bd)) -
                    static_cast<std::int64_t>(static_cast<EdgeDuration::value_type>(ab)) -
                    static_cast<std::int64_t>(static_cast<EdgeDuration::value_type>(cd));

                if (boundary_delta + interior_delta < 0)
                {
                    std::reverse(std::begin(route) + static_cast<std::ptrdiff_t>(i + 1),
                                 std::begin(route) + static_cast<std::ptrdiff_t>(k + 1));
                    improved = true;
                    break;
                }
            }
        }
    }

    return route;
}

inline std::vector<NodeID>
FarthestInsertionTrip(const std::size_t number_of_locations,
                      const util::DistTableWrapper<EdgeDuration> &dist_table)
{
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // START FARTHEST INSERTION HERE
    // 1. start at a random round trip of 2 locations
    // 2. find the location that is the farthest away from the visited locations and whose insertion
    // will make the round trip the longest
    // 3. add the found location to the current round trip such that round trip is the shortest
    // 4. repeat 2-3 until all locations are visited
    // 5. DONE!
    //////////////////////////////////////////////////////////////////////////////////////////////////

    // Guard against division-by-zero in the code path below.
    BOOST_ASSERT(number_of_locations > 0);

    // Guard against dist_table being empty therefore max_element returning the end iterator.
    BOOST_ASSERT(dist_table.size() > 0);

    BOOST_ASSERT_MSG(number_of_locations * number_of_locations == dist_table.size(),
                     "number_of_locations and dist_table size do not match");

    // find the pair of location with the biggest distance and make the pair the initial start
    // trip. Skipping over the very first element (0,0), we make sure not to end up with the
    // same start/end in the special case where all entries are the same.
    const auto index_of_farthest_distance = std::distance(
        std::begin(dist_table), std::max_element(std::begin(dist_table) + 1, std::end(dist_table)));
    // distance table is a nxn matrix with the distance(u,v) in column u and row v
    // but the distance table is stored in an 1D array of distances
    // to get the actual (u,v), get the row by dividing and the column by computing modulo n
    NodeID max_from = index_of_farthest_distance / number_of_locations;
    NodeID max_to = index_of_farthest_distance % number_of_locations;

    BOOST_ASSERT_MSG(static_cast<std::size_t>(max_from) < number_of_locations, "start node");
    BOOST_ASSERT_MSG(static_cast<std::size_t>(max_to) < number_of_locations, "start node");
    return FindRoute(number_of_locations, dist_table, max_from, max_to);
}

} // namespace osrm::engine::trip

#endif // TRIP_FARTHEST_INSERTION_HPP
