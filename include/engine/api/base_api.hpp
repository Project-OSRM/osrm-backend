#ifndef ENGINE_API_BASE_API_HPP
#define ENGINE_API_BASE_API_HPP

#include "engine/api/base_parameters.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include "engine/api/json_factory.hpp"
#include "engine/hint.hpp"

#include <boost/assert.hpp>

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

    util::json::Array
    MakeWaypoints(const std::vector<PhantomNodes> &segment_end_coordinates) const
    {
        BOOST_ASSERT(parameters.coordinates.size() > 0);
        BOOST_ASSERT(parameters.coordinates.size() == segment_end_coordinates.size() + 1);

        util::json::Array waypoints;
        waypoints.values.resize(parameters.coordinates.size());
        waypoints.values[0] = MakeWaypoint(parameters.coordinates.front(),
                                           segment_end_coordinates.front().source_phantom);

        auto coord_iter = std::next(parameters.coordinates.begin());
        auto out_iter = std::next(waypoints.values.begin());
        for (const auto &phantoms : segment_end_coordinates)
        {
            *out_iter++ = MakeWaypoint(*coord_iter++, phantoms.target_phantom);
        }
        return waypoints;
    }

  protected:
    util::json::Object MakeWaypoint(const util::FixedPointCoordinate input_coordinate,
                                            const PhantomNode &phantom) const
    {
        return json::makeWaypoint(phantom.location, facade.get_name_for_id(phantom.name_id),
                                  Hint{input_coordinate, phantom, facade.GetCheckSum()});
    }

    const datafacade::BaseDataFacade &facade;
    const BaseParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
