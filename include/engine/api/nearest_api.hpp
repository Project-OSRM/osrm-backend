#ifndef ENGINE_API_NEAREST_API_HPP
#define ENGINE_API_NEAREST_API_HPP

#include "engine/api/base_api.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/nearest_result.hpp"

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

    NearestResult
    MakeResponse(const std::vector<std::vector<PhantomNodeWithDistance>> &phantom_nodes) const
    {
        BOOST_ASSERT(phantom_nodes.size() == 1);
        BOOST_ASSERT(parameters.coordinates.size() == 1);

        NearestResult result;
        result.waypoints.resize(phantom_nodes.size());
        std::transform(phantom_nodes.front().begin(),
                       phantom_nodes.front().end(),
                       result.waypoints.begin(),
                       [this](const PhantomNodeWithDistance &phantom_with_distance) {
                           auto name = facade.GetNameForID(facade.GetNameIndex(
                               phantom_with_distance.phantom_node.forward_segment_id.id));
                           return Waypoint{phantom_with_distance.distance,
                                           name.data(),
                                           phantom_with_distance.phantom_node.location};
                       });

        return result;
    }

    const NearestParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
