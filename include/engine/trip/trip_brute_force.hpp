#ifndef TRIP_BRUTE_FORCE_HPP
#define TRIP_BRUTE_FORCE_HPP

#include "util/dist_table_wrapper.hpp"
#include "util/log.hpp"
#include "util/typedefs.hpp"

#include "osrm/json_container.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace trip
{

// computes the distance of a given permutation
inline EdgeWeight ReturnDistance(const util::DistTableWrapper<EdgeWeight> &dist_table,
                                 const std::vector<NodeID> &location_order,
                                 const EdgeWeight min_route_dist,
                                 const std::size_t number_of_locations)
{
    EdgeWeight route_dist = 0;
    std::size_t current_index = 0;
    while (current_index < location_order.size() && (route_dist < min_route_dist))
    {

        std::size_t next_index = (current_index + 1) % number_of_locations;
        auto edge_weight = dist_table(location_order[current_index], location_order[next_index]);

        // If the edge_weight is very large (INVALID_EDGE_WEIGHT) then the algorithm will not choose
        // this edge in final minimal path. So instead of computing all the permutations after this
        // large edge, discard this edge right here and don't consider the path after this edge.
        if (edge_weight == INVALID_EDGE_WEIGHT)
        {
            return INVALID_EDGE_WEIGHT;
        }
        else
        {
            route_dist += edge_weight;
        }

        // This boost assert should not be reached if TFSE table
        BOOST_ASSERT_MSG(dist_table(location_order[current_index], location_order[next_index]) !=
                             INVALID_EDGE_WEIGHT,
                         "invalid route found");
        ++current_index;
    }

    return route_dist;
}

// computes the route by computing all permutations and selecting the shortest
inline std::vector<NodeID> BruteForceTrip(const std::size_t number_of_locations,
                                          const util::DistTableWrapper<EdgeWeight> &dist_table)
{
    // set initial order in which nodes are visited to 0, 1, 2, 3, ...
    std::vector<NodeID> node_order(number_of_locations);
    std::iota(std::begin(node_order), std::end(node_order), 0);
    std::vector<NodeID> route = node_order;

    EdgeWeight min_route_dist = INVALID_EDGE_WEIGHT;

    // check length of all possible permutation of the component ids
    BOOST_ASSERT_MSG(node_order.size() > 0, "no order permutation given");
    BOOST_ASSERT_MSG(*(std::max_element(std::begin(node_order), std::end(node_order))) <
                         number_of_locations,
                     "invalid node id");

    do
    {
        const auto new_distance =
            ReturnDistance(dist_table, node_order, min_route_dist, number_of_locations);
        // we can use `<` instead of `<=` here, since all distances are `!=` INVALID_EDGE_WEIGHT
        // In case we really sum up to invalid edge weight for all permutations, keeping the very
        // first one is fine too.
        if (new_distance < min_route_dist)
        {
            min_route_dist = new_distance;
            route = node_order;
        }
    } while (std::next_permutation(std::begin(node_order), std::end(node_order)));

    return route;
}

} // namespace trip
} // namespace engine
} // namespace osrm

#endif // TRIP_BRUTE_FORCE_HPP
