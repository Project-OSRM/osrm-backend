#ifndef TRIP_NEAREST_NEIGHBOUR_HPP
#define TRIP_NEAREST_NEIGHBOUR_HPP

#include "util/dist_table_wrapper.hpp"
#include "util/log.hpp"
#include "util/typedefs.hpp"

#include "osrm/json_container.hpp"

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

template <typename NodeIDIterator>
std::vector<NodeID> NearestNeighbourTrip(const NodeIDIterator &start,
                                         const NodeIDIterator &end,
                                         const std::size_t number_of_locations,
                                         const util::DistTableWrapper<EdgeWeight> &dist_table)
{
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // START GREEDY NEAREST NEIGHBOUR HERE
    // 1. grab a random location and mark as starting point
    // 2. find the nearest unvisited neighbour, set it as the current location and mark as visited
    // 3. repeat 2 until there is no unvisited location
    // 4. return route back to starting point
    // 5. compute route
    // 6. repeat 1-5 with different starting points and choose iteration with shortest trip
    // 7. DONE!
    //////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<NodeID> route;
    route.reserve(number_of_locations);

    const auto component_size = std::distance(start, end);
    auto shortest_trip_distance = INVALID_EDGE_WEIGHT;

    // ALWAYS START AT ANOTHER STARTING POINT
    for (auto start_node = start; start_node != end; ++start_node)
    {
        NodeID curr_node = *start_node;

        std::vector<NodeID> curr_route;
        curr_route.reserve(component_size);
        curr_route.push_back(*start_node);

        // visited[i] indicates whether node i was already visited by the salesman
        std::vector<bool> visited(number_of_locations, false);
        visited[*start_node] = true;

        // 3. REPEAT FOR EVERY UNVISITED NODE
        EdgeWeight trip_dist = 0;
        for (std::size_t via_point = 1; via_point < component_size; ++via_point)
        {
            EdgeWeight min_dist = INVALID_EDGE_WEIGHT;
            NodeID min_id = SPECIAL_NODEID;

            // 2. FIND NEAREST NEIGHBOUR
            for (auto next = start; next != end; ++next)
            {
                const auto curr_dist = dist_table(curr_node, *next);
                BOOST_ASSERT_MSG(curr_dist != INVALID_EDGE_WEIGHT, "invalid distance found");
                if (!visited[*next] && curr_dist < min_dist)
                {
                    min_dist = curr_dist;
                    min_id = *next;
                }
            }

            BOOST_ASSERT_MSG(min_id != SPECIAL_NODEID, "no next node found");

            visited[min_id] = true;
            curr_route.push_back(min_id);
            trip_dist += min_dist;
            curr_node = min_id;
        }

        // check round trip with this starting point is shorter than the shortest round trip found
        // till now
        if (trip_dist < shortest_trip_distance)
        {
            shortest_trip_distance = trip_dist;
            route = std::move(curr_route);
        }
    }
    return route;
}
} // namespace trip
} // namespace engine
} // namespace osrm

#endif // TRIP_NEAREST_NEIGHBOUR_HPP
