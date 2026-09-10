#ifndef OSRM_ENGINE_ISOCHRONE_SEARCH_RESULT_MATERIALIZATION_HPP
#define OSRM_ENGINE_ISOCHRONE_SEARCH_RESULT_MATERIALIZATION_HPP

#include "engine/datafacade/datafacade_base.hpp"
#include "engine/isochrone/search_result.hpp"
#include "engine/isochrone/weighted_polyline_materialization.hpp"

namespace osrm::engine::isochrone
{

// Converts a completed graph search into duration-labeled road geometry. Search results contain
// original edge-based nodes only; CH shortcuts are never traversed by the duration search.
MaterializationResult materializeSearchResult(const datafacade::BaseDataFacade &facade,
                                              const SearchResult &search_result,
                                              EdgeDuration maximum_duration,
                                              bool inbound,
                                              const MaterializationLimits &limits);

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_SEARCH_RESULT_MATERIALIZATION_HPP
