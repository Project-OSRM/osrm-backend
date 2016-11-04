#ifndef ENGINE_API_BASE_API_HPP
#define ENGINE_API_BASE_API_HPP

#include "engine/api/base_parameters.hpp"
#include "engine/guidance/waypoint.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include "engine/hint.hpp"

#include <boost/assert.hpp>
#include <boost/range/algorithm/transform.hpp>

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

class BaseAPI
{
  public:
    BaseAPI(const datafacade::BaseDataFacade &facade_, const BaseParameters &parameters_)
        : facade(facade_), parameters(parameters_)
    {
    }

    std::vector<guidance::Waypoint>
    MakeWaypoints(const std::vector<PhantomNodes> &segment_end_coordinates) const
    {
        BOOST_ASSERT(parameters.coordinates.size() > 0);
        BOOST_ASSERT(parameters.coordinates.size() == segment_end_coordinates.size() + 1);

        std::vector<guidance::Waypoint> waypoints;
        waypoints.resize(parameters.coordinates.size());
        waypoints[0] = MakeWaypoint(segment_end_coordinates.front().source_phantom);

        auto out_iter = std::next(waypoints.begin());
        boost::range::transform(
            segment_end_coordinates, out_iter, [this](const PhantomNodes &phantom_pair) {
                return MakeWaypoint(phantom_pair.target_phantom);
            });
        return waypoints;
    }

    // FIXME gcc 4.8 doesn't support for lambdas to call protected member functions
    //  protected:
    guidance::Waypoint MakeWaypoint(const PhantomNode &phantom) const
    {
        return guidance::Waypoint{phantom.location,
                                 facade.GetNameForID(phantom.name_id),
                                 Hint{phantom, facade.GetCheckSum()}};
    }

    const datafacade::BaseDataFacade &facade;
    const BaseParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
