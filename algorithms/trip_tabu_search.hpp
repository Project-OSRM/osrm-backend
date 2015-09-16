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
#include "../util/simple_logger.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>
#include <limits>

namespace osrm
{
namespace trip
{

// todo: yet to be implemented
void TabuSearchTrip(std::vector<unsigned> &location,
                    const PhantomNodeArray &phantom_node_vector,
                    const std::vector<EdgeWeight> &dist_table,
                    InternalRouteResult &min_route,
                    std::vector<int> &min_loc_permutation)
{
}

void TabuSearchTrip(const PhantomNodeArray &phantom_node_vector,
                    const std::vector<EdgeWeight> &dist_table,
                    InternalRouteResult &min_route,
                    std::vector<int> &min_loc_permutation)
{
}
}
}
#endif // TRIP_BRUTE_FORCE_HPP