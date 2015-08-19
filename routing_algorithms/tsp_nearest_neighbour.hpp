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

#ifndef TSP_NEAREST_NEIGHBOUR_HPP
#define TSP_NEAREST_NEIGHBOUR_HPP


#include "../data_structures/search_engine.hpp"
#include "../util/string_util.hpp"
#include "../util/simple_logger.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>
#include <limits>



namespace osrm
{
namespace tsp
{

std::vector<NodeID> NearestNeighbourTSP(const std::vector<NodeID> & locations,
                                        const std::size_t number_of_locations,
                                        const std::vector<EdgeWeight> & dist_table) {
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

    const int component_size = locations.size();
    int shortest_trip_distance = INVALID_EDGE_WEIGHT;

    // ALWAYS START AT ANOTHER STARTING POINT
    for(auto start_node : locations)
    {
        int curr_node = start_node;

        std::vector<NodeID> curr_route;
        curr_route.reserve(component_size);
        curr_route.push_back(start_node);

        // visited[i] indicates whether node i was already visited by the salesman
        std::vector<bool> visited(number_of_locations, false);
        visited[start_node] = true;

        // 3. REPEAT FOR EVERY UNVISITED NODE
        int trip_dist = 0;
        for(int via_point = 1; via_point < component_size; ++via_point)
        {
            int min_dist = INVALID_EDGE_WEIGHT;
            int min_id = -1;

            // 2. FIND NEAREST NEIGHBOUR
            for (auto next : locations) {
                if(!visited[next] &&
                   *(dist_table.begin() + curr_node * number_of_locations + next) < min_dist) {
                    min_dist = *(dist_table.begin() + curr_node * number_of_locations + next);
                    min_id = next;
                }
            }
            visited[min_id] = true;
            curr_route.push_back(min_id);
            trip_dist += min_dist;
            curr_node = min_id;
        }

        // check round trip with this starting point is shorter than the shortest round trip found till now
        if (trip_dist < shortest_trip_distance) {
            shortest_trip_distance = trip_dist;
            route = curr_route;
        }
    }
    return route;
}

}
}
#endif // TSP_NEAREST_NEIGHBOUR_HPP