#include "engine/routing_algorithms/routing_base_ch.hpp"
#include "engine/routing_algorithms/routing_base_mld.hpp"
#include "engine/routing_algorithms/shortest_path_impl.hpp"

namespace osrm::engine::routing_algorithms
{

template InternalRouteResult
shortestPathSearch(SearchEngineData<ch::Algorithm> &engine_working_data,
                   const DataFacade<ch::Algorithm> &facade,
                   const std::vector<PhantomNodeCandidates> &waypoint_candidates,
                   const std::optional<bool> continue_straight_at_waypoint);

template InternalRouteResult
shortestPathSearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                   const DataFacade<mld::Algorithm> &facade,
                   const std::vector<PhantomNodeCandidates> &waypoint_candidates,
                   const std::optional<bool> continue_straight_at_waypoint);

} // namespace osrm::engine::routing_algorithms
