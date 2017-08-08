#ifndef OSRM_SHORTEST_PATH_HPP
#define OSRM_SHORTEST_PATH_HPP

#include "engine/algorithm.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

template <typename Algorithm>
InternalRouteResult shortestPathSearch(SearchEngineData<Algorithm> &engine_working_data,
                                       const DataFacade<Algorithm> &facade,
                                       const std::vector<PhantomNodes> &phantom_nodes_vector,
                                       const boost::optional<bool> continue_straight_at_waypoint);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* OSRM_SHORTEST_PATH_HPP */
