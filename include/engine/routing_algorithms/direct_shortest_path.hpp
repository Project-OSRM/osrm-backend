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

/// This is a striped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrainted
/// by the previous route.
/// This variation is only an optimazation for graphs with slow queries, for example
/// not fully contracted graphs.
InternalRouteResult directShortestPathSearch(
    SearchEngineData<ch::Algorithm> &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<ch::Algorithm> &facade,
    const PhantomNodes &phantom_nodes);

InternalRouteResult directShortestPathSearch(
    SearchEngineData<corech::Algorithm> &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<corech::Algorithm> &facade,
    const PhantomNodes &phantom_nodes);

InternalRouteResult directShortestPathSearch(
    SearchEngineData<mld::Algorithm> &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<mld::Algorithm> &facade,
    const PhantomNodes &phantom_nodes);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* DIRECT_SHORTEST_PATH_HPP */
