#ifndef TRIP_BRUTE_FORCE_HPP
#define TRIP_BRUTE_FORCE_HPP

#include "engine/search_engine.hpp"
#include "util/simple_logger.hpp"

#include "osrm/json_container.hpp"

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