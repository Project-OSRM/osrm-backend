#ifndef TRIP_FARTHEST_INSERTION_HPP
#define TRIP_FARTHEST_INSERTION_HPP

#include "util/dist_table_wrapper.hpp"
#include "util/typedefs.hpp"

#include "osrm/json_container.hpp"
#include <boost/assert.hpp>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace trip
{

// given a route and a new location, find the best place of insertion and
// check the distance of roundtrip when the new location is additionally visited
using NodeIDIter = std::vector<NodeID>::iterator;
inline std::pair<EdgeWeight, NodeIDIter>
GetShortestRoundTrip(const NodeID new_loc,
                     const util::DistTableWrapper<EdgeWeight> &dist_table,
                     const std::size_t number_of_locations,
                     std::vector<NodeID> &route)
{
    (void)number_of_locations; // unused

    auto min_trip_distance = INVALID_EDGE_WEIGHT;
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
        const auto trip_dist = dist_from + dist_to - dist_table(*from_node, *to_node);

        // If the edge_weight is very large (INVALID_EDGE_WEIGHT) then the algorithm will not choose
        // this edge in final minimal path. So instead of computing all the permutations after this
        // large edge, discard this edge right here and don't consider the path after this edge.
        if (dist_from == INVALID_EDGE_WEIGHT || dist_to == INVALID_EDGE_WEIGHT)
            continue;
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
    BOOST_ASSERT_MSG(min_trip_distance != INVALID_EDGE_WEIGHT, "trip has invalid edge weight");

    return std::make_pair(min_trip_distance, next_insert_point_candidate);
}

// given two initial start nodes, find a roundtrip route using the farthest insertion algorithm
inline std::vector<NodeID> FindRoute(const std::size_t &number_of_locations,
                                     const util::DistTableWrapper<EdgeWeight> &dist_table,
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
        auto farthest_distance = std::numeric_limits<int>::min();
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

                BOOST_ASSERT_MSG(insert_candidate.first != INVALID_EDGE_WEIGHT,
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

inline std::vector<NodeID>
FarthestInsertionTrip(const std::size_t number_of_locations,
                      const util::DistTableWrapper<EdgeWeight> &dist_table)
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

} // namespace trip
} // namespace engine
} // namespace osrm

#endif // TRIP_FARTHEST_INSERTION_HPP
