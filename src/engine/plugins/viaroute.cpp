#include "engine/plugins/viaroute.hpp"
#include "engine/api/route_api.hpp"
#include "engine/guidance/result.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/status.hpp"

#include "util/for_each_pair.hpp"
#include "util/integer_range.hpp"

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
    : shortest_path(heaps), alternative_path(heaps), direct_shortest_path(heaps),
      max_locations_viaroute(max_locations_viaroute)
{
}

Status ViaRoutePlugin::HandleRequest(const std::shared_ptr<datafacade::BaseDataFacade> facade,
                                     const api::RouteParameters &route_parameters,
                                     engine::guidance::Result &result) const
{
    BOOST_ASSERT(route_parameters.IsValid());

    if (max_locations_viaroute > 0 &&
        (static_cast<int>(route_parameters.coordinates.size()) > max_locations_viaroute))
    {
        return Status::ErrorTooBig;
    }

    if (!CheckAllCoordinates(route_parameters.coordinates))
    {
        return Status::ErrorInvalidValue;
    }

    auto phantom_node_pairs = GetPhantomNodes(*facade, route_parameters);
    if (phantom_node_pairs.size() != route_parameters.coordinates.size())
    {
        return Status::ErrorNoSegment;
    }
    BOOST_ASSERT(phantom_node_pairs.size() == route_parameters.coordinates.size());

    auto snapped_phantoms = SnapPhantomNodes(phantom_node_pairs);

    const bool continue_straight_at_waypoint = route_parameters.continue_straight
                                                   ? *route_parameters.continue_straight
                                                   : facade->GetContinueStraightDefault();

    InternalRouteResult raw_route;
    auto build_phantom_pairs = [&raw_route, continue_straight_at_waypoint](
        const PhantomNode &first_node, const PhantomNode &second_node) {
        raw_route.segment_end_coordinates.push_back(PhantomNodes{first_node, second_node});
        auto &last_inserted = raw_route.segment_end_coordinates.back();
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

    if (1 == raw_route.segment_end_coordinates.size())
    {
        if (route_parameters.alternatives && facade->GetCoreSize() == 0)
        {
            alternative_path(*facade, raw_route.segment_end_coordinates.front(), raw_route);
        }
        else
        {
            direct_shortest_path(*facade, raw_route.segment_end_coordinates, raw_route);
        }
    }
    else
    {
        shortest_path(*facade,
                      raw_route.segment_end_coordinates,
                      route_parameters.continue_straight,
                      raw_route);
    }

    // we can only know this after the fact, different SCC ids still
    // allow for connection in one direction.
    if (raw_route.is_valid())
    {
        api::RouteAPI route_api{*facade, route_parameters};
        route_api.MakeResponse(raw_route, result);
    }
    else
    {
        return Status::ErrorNoRoute;
    }

    return Status::Ok;
}
}
}
}
