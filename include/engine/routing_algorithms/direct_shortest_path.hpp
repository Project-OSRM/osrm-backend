#ifndef DIRECT_SHORTEST_PATH_HPP
#define DIRECT_SHORTEST_PATH_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/internal_route_result.hpp"
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
directShortestPathSearch(SearchEngineData &,
                         const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &,
                         const std::vector<PhantomNodes> &)
{
    throw util::exception(std::string("directShortestPathSearch is not implemented for ") +
                          typeid(AlgorithmT).name());
}

/// This is a striped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrainted
/// by the previous route.
/// This variation is only an optimazation for graphs with slow queries, for example
/// not fully contracted graphs.
InternalRouteResult directShortestPathSearch(
    SearchEngineData &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
    const PhantomNodes &phantom_nodes);

InternalRouteResult directShortestPathSearch(
    SearchEngineData &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CoreCH> &facade,
    const PhantomNodes &phantom_nodes);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* DIRECT_SHORTEST_PATH_HPP */
