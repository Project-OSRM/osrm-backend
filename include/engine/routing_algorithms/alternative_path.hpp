#ifndef ALTERNATIVE_PATH_ROUTING_HPP
#define ALTERNATIVE_PATH_ROUTING_HPP

#include "engine/datafacade.hpp"
#include "engine/internal_route_result.hpp"

#include "engine/algorithm.hpp"
#include "engine/search_engine_data.hpp"

#include "util/exception.hpp"

namespace osrm::engine::routing_algorithms
{

InternalManyRoutesResult alternativePathSearch(SearchEngineData<ch::Algorithm> &search_engine_data,
                                               const DataFacade<ch::Algorithm> &facade,
                                               const PhantomEndpointCandidates &endpoint_candidates,
                                               unsigned number_of_alternatives);

InternalManyRoutesResult alternativePathSearch(SearchEngineData<mld::Algorithm> &search_engine_data,
                                               const DataFacade<mld::Algorithm> &facade,
                                               const PhantomEndpointCandidates &endpoint_candidates,
                                               unsigned number_of_alternatives);

} // namespace osrm::engine::routing_algorithms

#endif
