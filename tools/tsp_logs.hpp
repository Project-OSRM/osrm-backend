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

#ifndef TSP_LOGS_HPP
#define TSP_LOGS_HPP


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


inline void PrintDistTable(const std::vector<EdgeWeight> & dist_table, const int number_of_locations) {
    int j = 0;
    for (auto i = dist_table.begin(); i != dist_table.end(); ++i){
        if (j % number_of_locations == 0) {
            std::cout << std::endl;
        }
        std::cout << std::setw(6) << *i << " ";
        ++j;
    }
    std::cout << std::endl;
}

bool CheckSymmetricTable(const std::vector<EdgeWeight> & dist_table, const int number_of_locations) {
    bool is_quadratic = true;
    for (int i = 0; i < number_of_locations; ++i) {
        for(int j = 0; j < number_of_locations; ++j) {
            int a = *(dist_table.begin() + (i * number_of_locations) + j);
            int b = *(dist_table.begin() + (j * number_of_locations) + i);
            if (a !=b) {
                is_quadratic = false;
            }
        }
    }
    return is_quadratic;
}

void LogRoute(std::vector<int> location_ids){
    SimpleLogger().Write() << "LOC ORDER";
    for (auto x : location_ids) {
        SimpleLogger().Write() << x;
    }
}

int ReturnDistanceFI(const std::vector<EdgeWeight> & dist_table, std::list<int> current_trip, const int number_of_locations) {
    int route_dist = 0;

    // compute length and stop if length is longer than route already found
    for (auto i = current_trip.begin(); i != std::prev(current_trip.end()); ++i) {
        //get distance from location i to location i+1
        route_dist += *(dist_table.begin() + (*i * number_of_locations) + *std::next(i));
    }
    //get distance from last location to first location
    route_dist += *(dist_table.begin() + (*std::prev(current_trip.end()) * number_of_locations) + current_trip.front());

    return route_dist;
}


}
}
#endif // TSP_LOGS_HPP