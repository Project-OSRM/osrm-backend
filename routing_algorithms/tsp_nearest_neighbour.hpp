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

void NearestNeighbourTSP(const std::vector<unsigned> & locations,
                         const PhantomNodeArray & phantom_node_vector,
                         const std::vector<EdgeWeight> & dist_table,
                         InternalRouteResult & min_route,
                         std::vector<int> & min_loc_permutation) {
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

    const auto number_of_locations = phantom_node_vector.size();
    const int size_of_component = locations.size();
    min_route.shortest_path_length = std::numeric_limits<int>::max();

    // ALWAYS START AT ANOTHER STARTING POINT
    for(auto start_node : locations)
    {
        int curr_node = start_node;
        InternalRouteResult raw_route;
        //TODO: Should we always use the same vector or does it not matter at all because of loop scope?
        std::vector<int> loc_permutation(number_of_locations, -1);
        loc_permutation[start_node] = 0;
        // visited[i] indicates whether node i was already visited by the salesman
        std::vector<bool> visited(number_of_locations, false);
        visited[start_node] = true;

        PhantomNodes viapoint;
        // 3. REPEAT FOR EVERY UNVISITED NODE
        int trip_dist = 0;
        for(int via_point = 1; via_point < size_of_component; ++via_point)
        {
            int min_dist = std::numeric_limits<int>::max();
            int min_id = -1;

            // 2. FIND NEAREST NEIGHBOUR
            for (auto next : locations) {
                if(!visited[next] &&
                   *(dist_table.begin() + curr_node * number_of_locations + next) < min_dist) {
                    min_dist = *(dist_table.begin() + curr_node * number_of_locations + next);
                    min_id = next;
                }
            }
            loc_permutation[min_id] = via_point;
            visited[min_id] = true;
            viapoint = PhantomNodes{phantom_node_vector[curr_node][0], phantom_node_vector[min_id][0]};
            raw_route.segment_end_coordinates.emplace_back(viapoint);
            trip_dist += min_dist;
            curr_node = min_id;
        }

        // 4. ROUTE BACK TO STARTING POINT
        viapoint = PhantomNodes{raw_route.segment_end_coordinates.back().target_phantom, phantom_node_vector[start_node][0]};
        raw_route.segment_end_coordinates.emplace_back(viapoint);

        // check round trip with this starting point is shorter than the shortest round trip found till now
        if (trip_dist < min_route.shortest_path_length) {
            min_route = raw_route;
            min_route.shortest_path_length = trip_dist;
            //TODO: this gets copied right? fix this
            min_loc_permutation = loc_permutation;
        }
    }

    // // ALWAYS START AT ANOTHER STARTING POINT
    // for(auto start_node : locations) {
    //     SimpleLogger().Write() << "STARTING AT " << start_node;
    //     int curr_node = start_node;
    //     InternalRouteResult raw_route;
    //     //TODO: Should we always use the same vector or does it not matter at all because of loop scope?
    //     std::vector<int> loc_permutation(number_of_locations, -1);

    //     // visited[i] indicates whether node i was already visited by the salesman
    //     std::vector<bool> visited(number_of_locations, false);
    //     visited[start_node] = true;
    //     loc_permutation[start_node] = 0;

    //     PhantomNodes viapoint;
    //     // 3. REPEAT FOR EVERY UNVISITED NODE
    //     int trip_dist = 0;
    //     for(int via_point = 1; via_point < size_of_component; ++via_point)
    //     {
    //         int min_dist = std::numeric_limits<int>::max();
    //         int min_id = -1;

    //         // 2. FIND NEAREST NEIGHBOUR
    //         for (auto next : locations) {
    //             if(!visited[next] &&
    //                *(dist_table.begin() + curr_node * number_of_locations + next) < min_dist) {
    //                 min_dist = *(dist_table.begin() + curr_node * number_of_locations + next);
    //                 min_id = next;
    //             }
    //         }

    //         loc_permutation[min_id] = via_point;
    //         visited[min_id] = true;
    //         SimpleLogger().Write() << "MOVING TO " << min_id;
    //         viapoint = PhantomNodes{phantom_node_vector[curr_node][0], phantom_node_vector[min_id][0]};
    //         raw_route.segment_end_coordinates.emplace_back(viapoint);
    //         trip_dist += min_dist;
    //         curr_node = min_id;
    //     }

    //     // 4. ROUTE BACK TO STARTING POINT
    //     viapoint = PhantomNodes{raw_route.segment_end_coordinates.back().target_phantom, phantom_node_vector[start_node][0]};
    //     raw_route.segment_end_coordinates.emplace_back(viapoint);

    //     // check round trip with this starting point is shorter than the shortest round trip found till now
    //     if (trip_dist < min_route.shortest_path_length) {
    //         min_route = raw_route;
    //         min_route.shortest_path_length = trip_dist;
    //         //TODO: this gets copied right? fix this
    //         min_loc_permutation = loc_permutation;
    //     }
    // }
}

void NearestNeighbourTSP(const PhantomNodeArray & phantom_node_vector,
                         const std::vector<EdgeWeight> & dist_table,
                         InternalRouteResult & min_route,
                         std::vector<int> & min_loc_permutation) {
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

    const auto number_of_locations = phantom_node_vector.size();
    min_route.shortest_path_length = std::numeric_limits<int>::max();

    // ALWAYS START AT ANOTHER STARTING POINT
    for(int start_node = 0; start_node < number_of_locations; ++start_node)
    {
        int curr_node = start_node;
        InternalRouteResult raw_route;
        //TODO: Should we always use the same vector or does it not matter at all because of loop scope?
        std::vector<int> loc_permutation(number_of_locations, -1);
        loc_permutation[start_node] = 0;
        // visited[i] indicates whether node i was already visited by the salesman
        std::vector<bool> visited(number_of_locations, false);
        visited[start_node] = true;

        PhantomNodes viapoint;
        // 3. REPEAT FOR EVERY UNVISITED NODE
        int trip_dist = 0;
        for(int via_point = 1; via_point < number_of_locations; ++via_point)
        {
            int min_dist = std::numeric_limits<int>::max();
            int min_id = -1;

            // 2. FIND NEAREST NEIGHBOUR
            auto row_begin_iterator = dist_table.begin() + (curr_node * number_of_locations);
            auto row_end_iterator = dist_table.begin() + ((curr_node + 1) * number_of_locations);
            for (auto it = row_begin_iterator; it != row_end_iterator; ++it) {
                auto index = std::distance(row_begin_iterator, it);
                if (!visited[index] && *it < min_dist)
                {
                    min_dist = *it;
                    min_id = index;
                }
            }
            loc_permutation[min_id] = via_point;
            visited[min_id] = true;
            viapoint = PhantomNodes{phantom_node_vector[curr_node][0], phantom_node_vector[min_id][0]};
            raw_route.segment_end_coordinates.emplace_back(viapoint);
            trip_dist += min_dist;
            curr_node = min_id;
        }

        // 4. ROUTE BACK TO STARTING POINT
        viapoint = PhantomNodes{raw_route.segment_end_coordinates.back().target_phantom, phantom_node_vector[start_node][0]};
        raw_route.segment_end_coordinates.emplace_back(viapoint);

        // check round trip with this starting point is shorter than the shortest round trip found till now
        if (trip_dist < min_route.shortest_path_length) {
            min_route = raw_route;
            min_route.shortest_path_length = trip_dist;
            //TODO: this gets copied right? fix this
            min_loc_permutation = loc_permutation;
        }
    }
}

}
}
#endif // TSP_NEAREST_NEIGHBOUR_HPP