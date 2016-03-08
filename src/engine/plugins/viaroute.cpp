#include "engine/plugins/viaroute.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/api/route_api.hpp"
#include "engine/object_encoder.hpp"
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

ViaRoutePlugin::ViaRoutePlugin(datafacade::BaseDataFacade &facade_, int max_locations_viaroute)
    : BasePlugin(facade_), shortest_path(&facade_, heaps), alternative_path(&facade_, heaps),
      direct_shortest_path(&facade_, heaps), max_locations_viaroute(max_locations_viaroute)
{
}

Status ViaRoutePlugin::HandleRequest(const api::RouteParameters &route_parameters,
                                     util::json::Object &json_result)
{
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

    auto phantom_node_pairs = GetPhantomNodes(route_parameters);
    if (phantom_node_pairs.size() != route_parameters.coordinates.size())
    {
        return Error("NoSegment",
                     std::string("Could not find a matching segment for coordinate ") +
                         std::to_string(phantom_node_pairs.size()),
                     json_result);
    }
    BOOST_ASSERT(phantom_node_pairs.size() == route_parameters.coordinates.size());

    auto snapped_phantoms = SnapPhantomNodes(phantom_node_pairs);

    InternalRouteResult raw_route;
    auto build_phantom_pairs = [&raw_route](const PhantomNode &first_node,
                                            const PhantomNode &second_node)
    {
        raw_route.segment_end_coordinates.push_back(PhantomNodes{first_node, second_node});
    };
    util::for_each_pair(snapped_phantoms, build_phantom_pairs);

    if (1 == raw_route.segment_end_coordinates.size())
    {
        if (route_parameters.alternatives)
        {
            alternative_path(raw_route.segment_end_coordinates.front(), raw_route);
        }
        else
        {
            direct_shortest_path(raw_route.segment_end_coordinates, raw_route);
        }
    }
    else
    {
        shortest_path(raw_route.segment_end_coordinates, route_parameters.uturns, raw_route);
    }

    // we can only know this after the fact, different SCC ids still
    // allow for connection in one direction.
    if (raw_route.is_valid())
    {
        api::RouteAPI route_api{BasePlugin::facade, route_parameters};
        route_api.MakeResponse(raw_route, json_result);
    }
    else
    {
        auto first_component_id = snapped_phantoms.front().component.id;
        auto not_in_same_component = std::any_of(snapped_phantoms.begin(), snapped_phantoms.end(),
                                                 [first_component_id](const PhantomNode &node)
                                                 {
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
