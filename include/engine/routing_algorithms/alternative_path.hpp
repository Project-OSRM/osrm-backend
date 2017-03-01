#ifndef ALTERNATIVE_PATH_ROUTING_HPP
#define ALTERNATIVE_PATH_ROUTING_HPP

#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/internal_route_result.hpp"

#include "engine/algorithm.hpp"
#include "engine/search_engine_data.hpp"

#include "util/exception.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

InternalRouteResult
alternativePathSearch(SearchEngineData &search_engine_data,
                      const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                      const PhantomNodes &phantom_node_pair);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
