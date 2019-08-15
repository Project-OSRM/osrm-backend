#ifndef ENGINE_API_BASE_API_HPP
#define ENGINE_API_BASE_API_HPP

#include "engine/api/base_parameters.hpp"
#include "engine/api/flatbuffers/fbresult_generated.h"
#include "engine/datafacade/datafacade_base.hpp"

#include "engine/api/json_factory.hpp"
#include "engine/hint.hpp"
#include "util/coordinate_calculation.hpp"

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

    util::json::Array MakeWaypoints(const std::vector<PhantomNodes> &segment_end_coordinates) const
    {
        BOOST_ASSERT(parameters.coordinates.size() > 0);
        BOOST_ASSERT(parameters.coordinates.size() == segment_end_coordinates.size() + 1);

        util::json::Array waypoints;
        waypoints.values.resize(parameters.coordinates.size());
        waypoints.values[0] = MakeWaypoint(segment_end_coordinates.front().source_phantom);

        auto out_iter = std::next(waypoints.values.begin());
        boost::range::transform(
            segment_end_coordinates, out_iter, [this](const PhantomNodes &phantom_pair) {
                return MakeWaypoint(phantom_pair.target_phantom);
            });
        return waypoints;
    }

    // FIXME: gcc 4.9 does not like MakeWaypoints to be protected
    // protected:
    util::json::Object MakeWaypoint(const PhantomNode &phantom) const
    {
        if (parameters.generate_hints)
        {
            // TODO: check forward/reverse
            return json::makeWaypoint(
                phantom.location,
                util::coordinate_calculation::fccApproximateDistance(phantom.location,
                                                                     phantom.input_location),
                facade.GetNameForID(facade.GetNameIndex(phantom.forward_segment_id.id)).to_string(),
                Hint{phantom, facade.GetCheckSum()});
        }
        else
        {
            // TODO: check forward/reverse
            return json::makeWaypoint(
                phantom.location,
                util::coordinate_calculation::fccApproximateDistance(phantom.location,
                                                                     phantom.input_location),
                facade.GetNameForID(facade.GetNameIndex(phantom.forward_segment_id.id))
                    .to_string());
        }
    }

    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
    MakeWaypoints(flatbuffers::FlatBufferBuilder &builder,
                  const std::vector<PhantomNodes> &segment_end_coordinates) const
    {
        BOOST_ASSERT(parameters.coordinates.size() > 0);
        BOOST_ASSERT(parameters.coordinates.size() == segment_end_coordinates.size() + 1);

        std::vector<flatbuffers::Offset<fbresult::Waypoint>> waypoints;
        waypoints.resize(parameters.coordinates.size());
        waypoints[0] =
            MakeWaypoint(builder, segment_end_coordinates.front().source_phantom).Finish();

        std::transform(segment_end_coordinates.begin(),
                       segment_end_coordinates.end(),
                       std::next(waypoints.begin()),
                       [this, &builder](const PhantomNodes &phantom_pair) {
                           return MakeWaypoint(builder, phantom_pair.target_phantom).Finish();
                       });
        return builder.CreateVector(waypoints);
    }

    // FIXME: gcc 4.9 does not like MakeWaypoints to be protected
    // protected:
    fbresult::WaypointBuilder MakeWaypoint(flatbuffers::FlatBufferBuilder &builder,
                                           const PhantomNode &phantom) const
    {

        auto location =
            fbresult::Position(static_cast<double>(util::toFloating(phantom.location.lon)),
                               static_cast<double>(util::toFloating(phantom.location.lat)));
        auto name_string = builder.CreateString(
            facade.GetNameForID(facade.GetNameIndex(phantom.forward_segment_id.id)).to_string());

        boost::optional<flatbuffers::Offset<flatbuffers::String>> hint_string = boost::none;
        if (parameters.generate_hints)
        {
            hint_string = builder.CreateString(Hint{phantom, facade.GetCheckSum()}.ToBase64());
        }

        fbresult::WaypointBuilder waypoint(builder);
        waypoint.add_location(&location);
        waypoint.add_distance(util::coordinate_calculation::fccApproximateDistance(
            phantom.location, phantom.input_location));
        waypoint.add_name(name_string);
        if (hint_string)
        {
            waypoint.add_hint(*hint_string);
        }
        return waypoint;
    }

    const datafacade::BaseDataFacade &facade;
    const BaseParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
