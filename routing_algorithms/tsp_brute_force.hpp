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
#include <iostream>
#include "../util/simple_logger.hpp"




namespace osrm
{
namespace tsp
{

template <typename number>
int ReturnDistance(const std::vector<EdgeWeight> & dist_table,
                   const std::vector<number> & location_order,
                   const int min_route_dist,
                   const int number_of_locations) {
    int route_dist = 0;
    int i = 0;
    while (i < location_order.size() - 1/* && route_dist < min_route_dist*/) {
        route_dist += *(dist_table.begin() + (location_order[i] * number_of_locations) + location_order[i+1]);
        ++i;
    }
    //get distance from last location to first location
    route_dist += *(dist_table.begin() + (location_order[location_order.size()-1] * number_of_locations) + location_order[0]);
    return route_dist;
}

void BruteForceTSP(std::vector<unsigned> & component,
                   const PhantomNodeArray & phantom_node_vector,
                   const std::vector<EdgeWeight> & dist_table,
                   std::vector<unsigned> & route) {

    const unsigned component_size = component.size();
    unsigned min_route_dist = std::numeric_limits<unsigned>::max();

    // check length of all possible permutation of the component ids
    do {
        unsigned new_distance = ReturnDistance(dist_table, component, min_route_dist, component_size);
        if (new_distance < min_route_dist) {
            min_route_dist = new_distance;
            route = component;
        }
    } while(std::next_permutation(component.begin(), component.end()));
}

void BruteForceTSP(const PhantomNodeArray & phantom_node_vector,
                   const std::vector<EdgeWeight> & dist_table,
                   std::vector<unsigned> & route) {
    const auto number_of_locations = phantom_node_vector.size();
    // fill a vector with node ids
    std::vector<unsigned> location_ids(number_of_locations);
    std::iota(location_ids.begin(), location_ids.end(), 0);

    unsigned min_route_dist = std::numeric_limits<unsigned>::max();
    // check length of all possible permutation of the location ids
    do {
        unsigned new_distance = ReturnDistance(dist_table, location_ids, min_route_dist, number_of_locations);

        if (new_distance < min_route_dist) {
            min_route_dist = new_distance;
            route = location_ids;
        }
    } while(std::next_permutation(location_ids.begin(), location_ids.end()));
}

}
}
#endif // TSP_BRUTE_FORCE_HPP