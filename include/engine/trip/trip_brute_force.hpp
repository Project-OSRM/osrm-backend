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

#ifndef TRIP_BRUTE_FORCE_HPP
#define TRIP_BRUTE_FORCE_HPP

#include "../data_structures/search_engine.hpp"
#include "../util/dist_table_wrapper.hpp"
#include "../util/simple_logger.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>
#include <algorithm>
#include <string>
#include <iterator>
#include <vector>
#include <limits>

namespace osrm
{
namespace trip
{

// computes the distance of a given permutation
EdgeWeight ReturnDistance(const DistTableWrapper<EdgeWeight> &dist_table,
                          const std::vector<NodeID> &location_order,
                          const EdgeWeight min_route_dist,
                          const std::size_t component_size)
{
    EdgeWeight route_dist = 0;
    std::size_t i = 0;
    while (i < location_order.size() && (route_dist < min_route_dist))
    {
        route_dist += dist_table(location_order[i], location_order[(i + 1) % component_size]);
        BOOST_ASSERT_MSG(dist_table(location_order[i], location_order[(i + 1) % component_size]) !=
                             INVALID_EDGE_WEIGHT,
                         "invalid route found");
        ++i;
    }

    return route_dist;
}

// computes the route by computing all permutations and selecting the shortest
template <typename NodeIDIterator>
std::vector<NodeID> BruteForceTrip(const NodeIDIterator start,
                                   const NodeIDIterator end,
                                   const std::size_t number_of_locations,
                                   const DistTableWrapper<EdgeWeight> &dist_table)
{
    (void)number_of_locations; // unused

    const auto component_size = std::distance(start, end);

    std::vector<NodeID> perm(start, end);
    std::vector<NodeID> route;
    route.reserve(component_size);

    EdgeWeight min_route_dist = INVALID_EDGE_WEIGHT;

    // check length of all possible permutation of the component ids

    BOOST_ASSERT_MSG(perm.size() > 0, "no permutation given");
    BOOST_ASSERT_MSG(*(std::max_element(std::begin(perm), std::end(perm))) < number_of_locations,
                     "invalid node id");
    BOOST_ASSERT_MSG(*(std::min_element(std::begin(perm), std::end(perm))) >= 0, "invalid node id");

    do
    {
        const auto new_distance = ReturnDistance(dist_table, perm, min_route_dist, component_size);
        if (new_distance <= min_route_dist)
        {
            min_route_dist = new_distance;
            route = perm;
        }
    } while (std::next_permutation(std::begin(perm), std::end(perm)));

    return route;
}

} // end namespace trip
} // end namespace osrm
#endif // TRIP_BRUTE_FORCE_HPP
