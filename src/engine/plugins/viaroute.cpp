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

ViaRoutePlugin::ViaRoutePlugin(int max_locations_viaroute)
    : max_locations_viaroute(max_locations_viaroute)
{
}

Status
ViaRoutePlugin::HandleRequest(const datafacade::ContiguousInternalMemoryDataFacadeBase &facade,
                              const RoutingAlgorithmsInterface &algorithms,
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

    if (!CheckAllCoordinates(route_parameters.coordinates))
    {
        return Error("InvalidValue", "Invalid coordinate value.", json_result);
    }

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

    const bool continue_straight_at_waypoint = route_parameters.continue_straight
                                                   ? *route_parameters.continue_straight
                                                   : facade.GetContinueStraightDefault();

    std::vector<PhantomNodes> start_end_nodes;
    auto build_phantom_pairs = [&start_end_nodes, continue_straight_at_waypoint](
        const PhantomNode &first_node, const PhantomNode &second_node) {
        start_end_nodes.push_back(PhantomNodes{first_node, second_node});
        auto &last_inserted = start_end_nodes.back();
        // enable forward direction if possible
        if (last_inserted.source_phantom.forward_segment_id.id != SPECIAL_SEGMENTID)
        {
            last_inserted.source_phantom.forward_segment_id.enabled |=
                !continue_straight_at_waypoint;
        }
        // enable reverse direction if possible
        if (last_inserted.source_phantom.reverse_segment_id.id != SPECIAL_SEGMENTID)
        {
            last_inserted.source_phantom.reverse_segment_id.enabled |=
                !continue_straight_at_waypoint;
        }
    };
    util::for_each_pair(snapped_phantoms, build_phantom_pairs);

    InternalRouteResult raw_route;
    if (1 == start_end_nodes.size() && algorithms.HasAlternativePathSearch() &&
        route_parameters.alternatives)
    {
        raw_route = algorithms.AlternativePathSearch(start_end_nodes.front());
    }
    else if (1 == start_end_nodes.size() && algorithms.HasDirectShortestPathSearch())
    {
        raw_route = algorithms.DirectShortestPathSearch(start_end_nodes.front());
    }
    else
    {
        raw_route =
            algorithms.ShortestPathSearch(start_end_nodes, route_parameters.continue_straight);
    }

    // we can only know this after the fact, different SCC ids still
    // allow for connection in one direction.
    if (raw_route.is_valid())
    {
        api::RouteAPI route_api{facade, route_parameters};
        route_api.MakeResponse(raw_route, json_result);
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
