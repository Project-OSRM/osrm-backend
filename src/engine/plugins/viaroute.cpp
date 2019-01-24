#include "engine/plugins/viaroute.hpp"
#include "engine/api/route_api.hpp"
#include "engine/routing_algorithms.hpp"
#include "engine/status.hpp"

#include "util/for_each_pair.hpp"
#include "util/integer_range.hpp"
#include "util/json_container.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

ViaRoutePlugin::ViaRoutePlugin(int max_locations_viaroute, int max_alternatives)
    : max_locations_viaroute(max_locations_viaroute), max_alternatives(max_alternatives)
{
}

Status ViaRoutePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                     const api::RouteParameters &route_parameters,
                                     util::json::Object &json_result) const
{
    BOOST_ASSERT(route_parameters.IsValid());

    if (!algorithms.HasShortestPathSearch() && route_parameters.coordinates.size() > 2)
    {
        return Error("NotImplemented",
                     "Shortest path search is not implemented for the chosen search algorithm. "
                     "Only two coordinates supported.",
                     json_result);
    }

    if (!algorithms.HasDirectShortestPathSearch() && !algorithms.HasShortestPathSearch())
    {
        return Error(
            "NotImplemented",
            "Direct shortest path search is not implemented for the chosen search algorithm.",
            json_result);
    }

    if (max_locations_viaroute > 0 &&
        (static_cast<int>(route_parameters.coordinates.size()) > max_locations_viaroute))
    {
        return Error("TooBig",
                     "Number of entries " + std::to_string(route_parameters.coordinates.size()) +
                         " is higher than current maximum (" +
                         std::to_string(max_locations_viaroute) + ")",
                     json_result);
    }

    // Takes care of alternatives=n and alternatives=true
    if ((route_parameters.number_of_alternatives > static_cast<unsigned>(max_alternatives)) ||
        (route_parameters.alternatives && max_alternatives == 0))
    {
        return Error("TooBig",
                     "Requested number of alternatives is higher than current maximum (" +
                         std::to_string(max_alternatives) + ")",
                     json_result);
    }

    if (!CheckAllCoordinates(route_parameters.coordinates))
    {
        return Error("InvalidValue", "Invalid coordinate value.", json_result);
    }

    // Error: first and last points should be waypoints
    if (!route_parameters.waypoints.empty() &&
        (route_parameters.waypoints[0] != 0 ||
         route_parameters.waypoints.back() != (route_parameters.coordinates.size() - 1)))
    {
        return Error("InvalidValue",
                     "First and last coordinates must be specified as waypoints.",
                     json_result);
    }

    if (!CheckAlgorithms(route_parameters, algorithms, json_result))
        return Status::Error;

    const auto &facade = algorithms.GetFacade();
    auto phantom_node_pairs = GetPhantomNodes(facade, route_parameters);
    if (phantom_node_pairs.size() != route_parameters.coordinates.size())
    {
        return Error("NoSegment",
                     std::string("Could not find a matching segment for coordinate ") +
                         std::to_string(phantom_node_pairs.size()),
                     json_result);
    }
    BOOST_ASSERT(phantom_node_pairs.size() == route_parameters.coordinates.size());

    auto snapped_phantoms = SnapPhantomNodes(phantom_node_pairs);

    std::vector<PhantomNodes> start_end_nodes;
    auto build_phantom_pairs = [&start_end_nodes](const PhantomNode &first_node,
                                                  const PhantomNode &second_node) {
        start_end_nodes.push_back(PhantomNodes{first_node, second_node});
    };
    util::for_each_pair(snapped_phantoms, build_phantom_pairs);

    api::RouteAPI route_api{facade, route_parameters};

    InternalManyRoutesResult routes;

    // TODO: in v6 we should remove the boolean and only keep the number parameter.
    // For now just force them to be in sync. and keep backwards compatibility.
    const auto wants_alternatives =
        (max_alternatives > 0) &&
        (route_parameters.alternatives || route_parameters.number_of_alternatives > 0);
    const auto number_of_alternatives = std::max(1u, route_parameters.number_of_alternatives);

    // Alternatives do not support vias, only direct s,t queries supported
    // See the implementation notes and high-level outline.
    // https://github.com/Project-OSRM/osrm-backend/issues/3905
    if (1 == start_end_nodes.size() && algorithms.HasAlternativePathSearch() && wants_alternatives)
    {
        routes = algorithms.AlternativePathSearch(start_end_nodes.front(), number_of_alternatives);
    }
    else if (1 == start_end_nodes.size() && algorithms.HasDirectShortestPathSearch())
    {
        routes = algorithms.DirectShortestPathSearch(start_end_nodes.front());
    }
    else
    {
        routes = algorithms.ShortestPathSearch(start_end_nodes, route_parameters.continue_straight);
    }

    // The post condition for all path searches is we have at least one route in our result.
    // This route might be invalid by means of INVALID_EDGE_WEIGHT as shortest path weight.
    BOOST_ASSERT(!routes.routes.empty());

    // we can only know this after the fact, different SCC ids still
    // allow for connection in one direction.

    if (routes.routes[0].is_valid())
    {
        auto collapse_legs = !route_parameters.waypoints.empty();
        if (collapse_legs)
        {
            std::vector<bool> waypoint_legs(route_parameters.coordinates.size(), false);
            std::for_each(route_parameters.waypoints.begin(),
                          route_parameters.waypoints.end(),
                          [&](const std::size_t waypoint_index) {
                              BOOST_ASSERT(waypoint_index < waypoint_legs.size());
                              waypoint_legs[waypoint_index] = true;
                          });
            // First and last coordinates should always be waypoints
            // This gets validated earlier, but double-checking here, jic
            BOOST_ASSERT(waypoint_legs.front());
            BOOST_ASSERT(waypoint_legs.back());
            for (std::size_t i = 0; i < routes.routes.size(); i++)
            {
                routes.routes[i] = CollapseInternalRouteResult(routes.routes[i], waypoint_legs);
            }
        }

        route_api.MakeResponse(routes, start_end_nodes, json_result);
    }
    else
    {
        auto first_component_id = snapped_phantoms.front().component.id;
        auto not_in_same_component = std::any_of(snapped_phantoms.begin(),
                                                 snapped_phantoms.end(),
                                                 [first_component_id](const PhantomNode &node) {
                                                     return node.component.id != first_component_id;
                                                 });

        if (not_in_same_component)
        {
            return Error("NoRoute", "Impossible route between points", json_result);
        }
        else
        {
            return Error("NoRoute", "No route found between points", json_result);
        }
    }

    return Status::Ok;
}
}
}
}
