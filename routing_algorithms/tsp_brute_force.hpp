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

#ifndef TSP_BRUTE_FORCE_HPP
#define TSP_BRUTE_FORCE_HPP


#include "../data_structures/search_engine.hpp"
#include "../util/string_util.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>

#include <algorithm>
#include <string>
#include <vector>
#include <limits>
#include "../util/simple_logger.hpp"


// HAHAHA. NICE TRY, CHAU.


namespace osrm
{
namespace tsp
{



int ReturnDistance(const std::vector<EdgeWeight> & dist_table, const std::vector<int> location_order, const int min_route_dist, const int number_of_locations) {
    int route_dist = 0;
    int i = 0;

    // compute length and stop if length is longer than route already found
    while (i < number_of_locations - 1 && route_dist < min_route_dist) {
        //get distance from location i to location i+1
        route_dist += *(dist_table.begin() + (location_order[i] * number_of_locations) + location_order[i+1]);
        ++i;
    }
    //get distance from last location to first location
    route_dist += *(dist_table.begin() + (location_order[number_of_locations-1] * number_of_locations) + location_order[0]);

    if (route_dist >= min_route_dist) {
        return -1;
    }
    else {
        return route_dist;
    }
}

void BruteForce(const RouteParameters & route_parameters,
                const PhantomNodeArray & phantom_node_vector,
                const std::vector<EdgeWeight> & dist_table,
                InternalRouteResult & min_route,
                std::vector<int> & min_loc_permutation) {

    const auto number_of_locations = phantom_node_vector.size();
    // fill a vector with node ids
    std::vector<int> location_ids(number_of_locations);
    std::iota(location_ids.begin(), location_ids.end(), 0);

    int min_route_dist = std::numeric_limits<int>::max();

    // check length of all possible permutation of the location ids
    do {
        int new_distance = ReturnDistance(dist_table, location_ids, min_route_dist, number_of_locations);
        if (new_distance != -1) {
            min_route_dist = new_distance;
            //TODO: this gets copied right? fix this
            min_loc_permutation = location_ids;
        }
    } while(std::next_permutation(location_ids.begin(), location_ids.end()));

    PhantomNodes viapoint;
    for (int i = 0; i < number_of_locations - 1; ++i) {
        viapoint = PhantomNodes{phantom_node_vector[i][0], phantom_node_vector[i + 1][0]};
        min_route.segment_end_coordinates.emplace_back(viapoint);
    }
    viapoint = PhantomNodes{phantom_node_vector[number_of_locations - 1][0], phantom_node_vector[0][0]};
    min_route.segment_end_coordinates.emplace_back(viapoint);
}

}
}
#endif // TSP_BRUTE_FORCE_HPP