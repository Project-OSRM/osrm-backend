#ifndef SHORTEST_PATH_HPP
#define SHORTEST_PATH_HPP

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

template <typename AlgorithmT>
InternalRouteResult
shortestPathSearch(SearchEngineData<AlgorithmT> &engine_working_data,
                   const datafacade::AlgorithmDataFacade<AlgorithmT> &alg_facade,
                   const datafacade::BaseDataFacade &base_facade,
                   const std::vector<PhantomNodes> &phantom_nodes_vector,
                   const boost::optional<bool> continue_straight_at_waypoint);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* SHORTEST_PATH_HPP */
