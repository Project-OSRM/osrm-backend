#ifndef ENGINE_API_TRIP_HPP
#define ENGINE_API_TRIP_HPP

#include "engine/api/route_api.hpp"
#include "engine/api/trip_parameters.hpp"

#include "engine/datafacade/datafacade_base.hpp"

#include "engine/internal_route_result.hpp"

#include "util/integer_range.hpp"

namespace osrm
{
namespace engine
{
namespace api
{

class TripAPI final : public RouteAPI
{
  public:
    TripAPI(const datafacade::BaseDataFacade &facade_, const TripParameters &parameters_)
        : RouteAPI(facade_, parameters_), parameters(parameters_)
    {
    }
    void MakeResponse(const std::vector<std::vector<NodeID>> &sub_trips,
                      const std::vector<InternalRouteResult> &sub_routes,
                      const std::vector<PhantomNode> &phantoms,
                      osrm::engine::api::ResultT &response) const
    {
        BOOST_ASSERT(sub_trips.size() == sub_routes.size());

        if (response.is<flatbuffers::FlatBufferBuilder>())
        {
            auto &fb_result = response.get<flatbuffers::FlatBufferBuilder>();
            MakeResponse(sub_trips, sub_routes, phantoms, fb_result);
        }
        else
        {
            auto &json_result = response.get<util::json::Object>();
            MakeResponse(sub_trips, sub_routes, phantoms, json_result);
        }
    }
    void MakeResponse(const std::vector<std::vector<NodeID>> &sub_trips,
                      const std::vector<InternalRouteResult> &sub_routes,
                      const std::vector<PhantomNode> &phantoms,
                      flatbuffers::FlatBufferBuilder &fb_result) const
    {
        auto data_timestamp = facade.GetTimestamp();
        boost::optional<flatbuffers::Offset<flatbuffers::String>> data_version_string = boost::none;
        if (!data_timestamp.empty())
        {
            data_version_string = fb_result.CreateString(data_timestamp);
        }

        auto response =
            MakeFBResponse(sub_routes, fb_result, [this, &fb_result, &sub_trips, &phantoms]() {
                return MakeWaypoints(fb_result, sub_trips, phantoms);
            });

        if (data_version_string)
        {
            response.add_data_version(*data_version_string);
        }
        fb_result.Finish(response.Finish());
    }
    void MakeResponse(const std::vector<std::vector<NodeID>> &sub_trips,
                      const std::vector<InternalRouteResult> &sub_routes,
                      const std::vector<PhantomNode> &phantoms,
                      util::json::Object &response) const
    {
        auto number_of_routes = sub_trips.size();
        util::json::Array routes;
        routes.values.reserve(number_of_routes);
        for (auto index : util::irange<std::size_t>(0UL, sub_trips.size()))
        {
            auto route = MakeRoute(sub_routes[index].segment_end_coordinates,
                                   sub_routes[index].unpacked_path_segments,
                                   sub_routes[index].source_traversed_in_reverse,
                                   sub_routes[index].target_traversed_in_reverse);
            routes.values.push_back(std::move(route));
        }
        response.values["waypoints"] = MakeWaypoints(sub_trips, phantoms);
        response.values["trips"] = std::move(routes);
        response.values["code"] = "Ok";
    }

  protected:
    // FIXME this logic is a little backwards. We should change the output format of the
    // trip plugin routing algorithm to be easier to consume here.

    struct TripIndex
    {
        TripIndex() = default;

        TripIndex(unsigned sub_trip_index_, unsigned point_index_)
            : sub_trip_index(sub_trip_index_), point_index(point_index_)
        {
        }

        unsigned sub_trip_index = std::numeric_limits<unsigned>::max();
        unsigned point_index = std::numeric_limits<unsigned>::max();

        bool NotUsed()
        {
            return sub_trip_index == std::numeric_limits<unsigned>::max() &&
                   point_index == std::numeric_limits<unsigned>::max();
        }
    };

    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
    MakeWaypoints(flatbuffers::FlatBufferBuilder &fb_result,
                  const std::vector<std::vector<NodeID>> &sub_trips,
                  const std::vector<PhantomNode> &phantoms) const
    {
        std::vector<flatbuffers::Offset<fbresult::Waypoint>> waypoints;
        waypoints.reserve(parameters.coordinates.size());

        auto input_idx_to_trip_idx = MakeTripIndices(sub_trips);

        for (auto input_index : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {
            auto trip_index = input_idx_to_trip_idx[input_index];
            BOOST_ASSERT(!trip_index.NotUsed());

            auto waypoint = BaseAPI::MakeWaypoint(fb_result, phantoms[input_index]);
            waypoint.add_waypoint_index(trip_index.point_index);
            waypoint.add_trips_index(trip_index.sub_trip_index);
            waypoints.push_back(waypoint.Finish());
        }

        return fb_result.CreateVector(waypoints);
    }

    util::json::Array MakeWaypoints(const std::vector<std::vector<NodeID>> &sub_trips,
                                    const std::vector<PhantomNode> &phantoms) const
    {
        util::json::Array waypoints;
        waypoints.values.reserve(parameters.coordinates.size());

        auto input_idx_to_trip_idx = MakeTripIndices(sub_trips);

        for (auto input_index : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {
            auto trip_index = input_idx_to_trip_idx[input_index];
            BOOST_ASSERT(!trip_index.NotUsed());

            auto waypoint = BaseAPI::MakeWaypoint(phantoms[input_index]);
            waypoint.values["trips_index"] = trip_index.sub_trip_index;
            waypoint.values["waypoint_index"] = trip_index.point_index;
            waypoints.values.push_back(std::move(waypoint));
        }

        return waypoints;
    }

    std::vector<TripIndex> MakeTripIndices(const std::vector<std::vector<NodeID>> &sub_trips) const
    {
        std::vector<TripIndex> input_idx_to_trip_idx(parameters.coordinates.size());
        for (auto sub_trip_index : util::irange<unsigned>(0u, sub_trips.size()))
        {
            for (auto point_index : util::irange<unsigned>(0u, sub_trips[sub_trip_index].size()))
            {
                input_idx_to_trip_idx[sub_trips[sub_trip_index][point_index]] =
                    TripIndex{sub_trip_index, point_index};
            }
        }
        return input_idx_to_trip_idx;
    }

    const TripParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
