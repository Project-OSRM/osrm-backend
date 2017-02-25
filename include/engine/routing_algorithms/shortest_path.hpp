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
shortestPathSearch(SearchEngineData &,
                   const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &,
                   const std::vector<PhantomNodes> &,
                   const boost::optional<bool>)
{
    throw util::exception(std::string("shortestPathSearch is not implemented for ") +
                          typeid(AlgorithmT).name());
}

InternalRouteResult
shortestPathSearch(SearchEngineData &engine_working_data,
                   const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                   const std::vector<PhantomNodes> &phantom_nodes_vector,
                   const boost::optional<bool> continue_straight_at_waypoint);

InternalRouteResult
shortestPathSearch(SearchEngineData &engine_working_data,
                   const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CoreCH> &facade,
                   const std::vector<PhantomNodes> &phantom_nodes_vector,
                   const boost::optional<bool> continue_straight_at_waypoint);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* SHORTEST_PATH_HPP */
