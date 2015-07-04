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

#ifndef TSP_FARTHEST_INSERTION_HPP
#define TSP_FARTHEST_INSERTION_HPP


#include "../data_structures/search_engine.hpp"
#include "../util/string_util.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>

#include <algorithm>
#include <string>
#include <vector>
#include <limits>
#include <math.h>

namespace osrm
{
namespace tsp
{

void FarthestInsertionTSP(const PhantomNodeArray & phantom_node_vector,
                          const std::vector<EdgeWeight> & dist_table,
                          InternalRouteResult & min_route,
                          std::vector<int> & min_loc_permutation) {
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // START FARTHEST INSERTION HERE
    // 1. start at a random round trip of 2 locations
    // 2. find the location that is the farthest away from the visited locations and whose insertion will make the round trip the longest
    // 3. add the found location to the current round trip such that round trip is the shortest
    // 4. repeat 2-3 until all locations are visited
    // 5. DONE!
    //////////////////////////////////////////////////////////////////////////////////////////////////

    // const auto number_of_locations = phantom_node_vector.size();
    const int number_of_locations = sqrt(dist_table.size());
    // list of the trip that will be found incrementally
    std::list<int> current_trip;
    // tracks which nodes have been already visited
    std::vector<bool> visited(number_of_locations, false);

    // PrintDistTable(dist_table, number_of_locations);


    // find the pair of location with the biggest distance and make the pair the initial start trip
    const auto index = std::distance(dist_table.begin(), std::max_element(dist_table.begin(), dist_table.end()));
    const int max_from = index / number_of_locations;
    const int max_to = index % number_of_locations;

    visited[max_from] = true;
    visited[max_to] = true;
    current_trip.push_back(max_from);
    current_trip.push_back(max_to);

    // add all other nodes missing (two nodes are already in the initial start trip)
    for (int j = 2; j < number_of_locations; ++j) {
        auto shortest_max_tour = -1;
        int next_node = -1;
        std::list<int>::iterator min_max_insert;

        // find unvisited loc i that is the farthest away from all other visited locs
        for (int i = 0; i < number_of_locations; ++i) {
            if (!visited[i]) {
                // longest_min_tour is the distance of the longest of all insertions with the minimal distance
                auto longest_min_tour = std::numeric_limits<int>::max();
                // following_loc is the location that comes after the location that is to be inserted
                std::list<int>::iterator following_loc;

                // for all nodes in the current trip find the best insertion resulting in the shortest path
                for (auto from_node = current_trip.begin(); from_node != std::prev(current_trip.end()); ++from_node) {
                    auto to_node = std::next(from_node);

                    auto dist_from = *(dist_table.begin() + (*from_node * number_of_locations) + i);
                    auto dist_to = *(dist_table.begin() + (i * number_of_locations) + *to_node);
                    auto trip_dist = dist_from + dist_to - *(dist_table.begin() + (*from_node * number_of_locations) + *to_node);

                    // from all possible insertions to the current trip, choose the longest of all minimal insertions
                    if (trip_dist < longest_min_tour) {
                        longest_min_tour = trip_dist;
                        following_loc = to_node;
                    }
                }
                {   // check insertion between last and first location too
                    auto from_node = std::prev(current_trip.end());
                    auto to_node = current_trip.begin();

                    auto dist_from = *(dist_table.begin() + (*from_node * number_of_locations) + i);
                    auto dist_to = *(dist_table.begin() + (i * number_of_locations) + *to_node);
                    auto trip_dist = dist_from + dist_to - *(dist_table.begin() + (*from_node * number_of_locations) + *to_node);
                    if (trip_dist < longest_min_tour) {
                        longest_min_tour = trip_dist;
                        following_loc = to_node;
                    }
                }

                // add the location to the current trip such that it results in the shortest total tour
                if (longest_min_tour > shortest_max_tour) {
                    shortest_max_tour = longest_min_tour;
                    next_node = i;
                    min_max_insert = following_loc;
                }
            }
        }
        // mark as visited and insert node
        visited[next_node] = true;
        current_trip.insert(min_max_insert, next_node);
    }

    // given he final trip, compute total distance and return the route and location permutation
    PhantomNodes viapoint;
    int perm = 0;
    for (auto it = current_trip.begin(); it != std::prev(current_trip.end()); ++it) {
        auto from_node = *it;
        auto to_node = *std::next(it);

        viapoint = PhantomNodes{phantom_node_vector[from_node][0], phantom_node_vector[to_node][0]};
        min_route.segment_end_coordinates.emplace_back(viapoint);

        min_loc_permutation[from_node] = perm;
        ++perm;
    }
    {   // check dist between last and first location too
        viapoint = PhantomNodes{phantom_node_vector[*std::prev(current_trip.end())][0], phantom_node_vector[current_trip.front()][0]};
        min_route.segment_end_coordinates.emplace_back(viapoint);
        min_loc_permutation[*std::prev(current_trip.end())] = perm;
    }
}


}
}

#endif // TSP_FARTHEST_INSERTION_HPP