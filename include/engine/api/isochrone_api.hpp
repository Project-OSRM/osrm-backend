//
// Created by robin on 4/13/16.
//

#ifndef ENGINE_API_ISOCHRONE_HPP
#define ENGINE_API_ISOCHRONE_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/isochrone_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"

namespace osrm
{
namespace engine
{
namespace api
{

class IsochroneAPI final : public BaseAPI
{

  public:
    const IsochroneParameters &parameters;

    IsochroneAPI(const datafacade::BaseDataFacade &facade_, const IsochroneParameters &parameters_)
        : BaseAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    void MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes,
                      util::json::Object &response) const
    {
        BOOST_ASSERT(phantom_nodes.size() == 1);
        BOOST_ASSERT(parameters.coordinates.size() == 1);

        util::json::Array waypoints;
        waypoints.values.resize(phantom_nodes.front().size());
        std::transform(phantom_nodes.front().begin(), phantom_nodes.front().end(),
                       waypoints.values.begin(),
                       [this](const PhantomNodeWithDistance &phantom_with_distance)
                       {
                           auto waypoint = MakeWaypoint(phantom_with_distance.phantom_node);
                           waypoint.values["distance"] = phantom_with_distance.distance;
                           return waypoint;
                       });

        response.values["code"] = "Ok";
        response.values["waypoints"] = std::move(waypoints);
    }


};
}
}
}
#endif // ENGINE_API_ISOCHRONE_HPP
