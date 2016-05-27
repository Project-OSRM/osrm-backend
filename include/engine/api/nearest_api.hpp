#ifndef ENGINE_API_NEAREST_API_HPP
#define ENGINE_API_NEAREST_API_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/nearest_parameters.hpp"

#include "engine/api/json_factory.hpp"
#include "engine/phantom_node.hpp"

#include <boost/assert.hpp>

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

class NearestAPI final : public BaseAPI
{
  public:
    NearestAPI(const datafacade::BaseDataFacade &facade_, const NearestParameters &parameters_)
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
        std::transform(phantom_nodes.front().begin(),
                       phantom_nodes.front().end(),
                       waypoints.values.begin(),
                       [this](const PhantomNodeWithDistance &phantom_with_distance) {
                           auto waypoint = MakeWaypoint(phantom_with_distance.phantom_node);
                           waypoint.values["distance"] = phantom_with_distance.distance;
                           return waypoint;
                       });

        response.values["code"] = "Ok";
        response.values["waypoints"] = std::move(waypoints);
    }

    const NearestParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
