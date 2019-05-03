#ifndef MULTI_HEADING_PATH_HPP
#define MULTI_HEADING_PATH_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

template <typename Algorithm>
InternalManyRoutesResult
multiHeadingDirectShortestPathsSearch(SearchEngineData<Algorithm> &engine_working_data,
                                      const DataFacade<Algorithm> &facade,
                                      const PhantomNodes &phantom_nodes);
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
