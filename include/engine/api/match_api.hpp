#ifndef ENGINE_API_MATCH_HPP
#define ENGINE_API_MATCH_HPP

#include "engine/api/match_parameters.hpp"
#include "engine/api/match_parameters_tidy.hpp"
#include "engine/api/route_api.hpp"

#include "engine/datafacade/datafacade_base.hpp"

#include "engine/internal_route_result.hpp"
#include "engine/map_matching/sub_matching.hpp"

#include "util/integer_range.hpp"

namespace osrm
{
namespace engine
{
namespace api
{

class MatchAPI final : public RouteAPI
{
  public:
    MatchAPI(const datafacade::BaseDataFacade &facade_,
             const MatchParameters &parameters_,
             const tidy::Result &tidy_result_)
        : RouteAPI(facade_, parameters_), parameters(parameters_), tidy_result(tidy_result_)
    {
    }

    void MakeResponse(const std::vector<map_matching::SubMatching> &sub_matchings,
                      const std::vector<InternalRouteResult> &sub_routes,
                      osrm::engine::api::ResultT &response) const
    {
        BOOST_ASSERT(sub_matchings.size() == sub_routes.size());
        if (response.is<flatbuffers::FlatBufferBuilder>())
        {
            auto &fb_result = response.get<flatbuffers::FlatBufferBuilder>();
            MakeResponse(sub_matchings, sub_routes, fb_result);
        }
        else
        {
            auto &json_result = response.get<util::json::Object>();
            MakeResponse(sub_matchings, sub_routes, json_result);
        }
    }
    void MakeResponse(const std::vector<map_matching::SubMatching> &sub_matchings,
                      const std::vector<InternalRouteResult> &sub_routes,
                      flatbuffers::FlatBufferBuilder &fb_result) const
    {
        auto data_timestamp = facade.GetTimestamp();
        boost::optional<flatbuffers::Offset<flatbuffers::String>> data_version_string = boost::none;
        if (!data_timestamp.empty())
        {
            data_version_string = fb_result.CreateString(data_timestamp);
        }

        auto response = MakeFBResponse(sub_routes, fb_result, [this, &fb_result, &sub_matchings]() {
            return MakeTracepoints(fb_result, sub_matchings);
        });

        if (data_version_string)
        {
            response.add_data_version(*data_version_string);
        }

        fb_result.Finish(response.Finish());
    }
    void MakeResponse(const std::vector<map_matching::SubMatching> &sub_matchings,
                      const std::vector<InternalRouteResult> &sub_routes,
                      util::json::Object &response) const
    {
        auto number_of_routes = sub_matchings.size();
        util::json::Array routes;
        routes.values.reserve(number_of_routes);
        for (auto index : util::irange<std::size_t>(0UL, sub_matchings.size()))
        {
            auto route = MakeRoute(sub_routes[index].segment_end_coordinates,
                                   sub_routes[index].unpacked_path_segments,
                                   sub_routes[index].source_traversed_in_reverse,
                                   sub_routes[index].target_traversed_in_reverse);
            route.values["confidence"] = sub_matchings[index].confidence;
            routes.values.push_back(std::move(route));
        }
        response.values["tracepoints"] = MakeTracepoints(sub_matchings);
        response.values["matchings"] = std::move(routes);
        response.values["code"] = "Ok";
    }

  protected:
    // FIXME this logic is a little backwards. We should change the output format of the
    // map_matching
    // routing algorithm to be easier to consume here.
    struct MatchingIndex
    {
        MatchingIndex() = default;
        MatchingIndex(unsigned sub_matching_index_, unsigned point_index_)
            : sub_matching_index(sub_matching_index_), point_index(point_index_)
        {
        }

        unsigned sub_matching_index = std::numeric_limits<unsigned>::max();
        unsigned point_index = std::numeric_limits<unsigned>::max();

        bool NotMatched()
        {
            return sub_matching_index == std::numeric_limits<unsigned>::max() &&
                   point_index == std::numeric_limits<unsigned>::max();
        }
    };

    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbresult::Waypoint>>>
    MakeTracepoints(flatbuffers::FlatBufferBuilder &fb_result,
                    const std::vector<map_matching::SubMatching> &sub_matchings) const
    {
        std::vector<flatbuffers::Offset<fbresult::Waypoint>> waypoints;
        waypoints.reserve(parameters.coordinates.size());

        auto trace_idx_to_matching_idx = MakeMatchingIndices(sub_matchings);

        BOOST_ASSERT(parameters.waypoints.empty() || sub_matchings.size() == 1);

        std::size_t was_waypoint_idx = 0;
        for (auto trace_index : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {

            if (tidy_result.can_be_removed[trace_index])
            {
                waypoints.push_back(fbresult::WaypointBuilder(fb_result).Finish());
                continue;
            }
            auto matching_index = trace_idx_to_matching_idx[trace_index];
            if (matching_index.NotMatched())
            {
                waypoints.push_back(fbresult::WaypointBuilder(fb_result).Finish());
                continue;
            }
            const auto &phantom =
                sub_matchings[matching_index.sub_matching_index].nodes[matching_index.point_index];
            auto waypoint = BaseAPI::MakeWaypoint(fb_result, phantom);
            waypoint.add_matchings_index(matching_index.sub_matching_index);
            waypoint.add_alternatives_count(sub_matchings[matching_index.sub_matching_index]
                                                .alternatives_count[matching_index.point_index]);
            // waypoint indices need to be adjusted if route legs were collapsed
            // waypoint parameter assumes there is only one match object
            if (!parameters.waypoints.empty())
            {
                if (tidy_result.was_waypoint[trace_index])
                {
                    waypoint.add_waypoint_index(was_waypoint_idx);
                    was_waypoint_idx++;
                }
                else
                {
                    waypoint.add_waypoint_index(0);
                }
            }
            else
            {
                waypoint.add_waypoint_index(matching_index.point_index);
            }
            waypoints.push_back(waypoint.Finish());
        }

        return fb_result.CreateVector(waypoints);
    }

    util::json::Array
    MakeTracepoints(const std::vector<map_matching::SubMatching> &sub_matchings) const
    {
        util::json::Array waypoints;
        waypoints.values.reserve(parameters.coordinates.size());

        auto trace_idx_to_matching_idx = MakeMatchingIndices(sub_matchings);

        BOOST_ASSERT(parameters.waypoints.empty() || sub_matchings.size() == 1);

        std::size_t was_waypoint_idx = 0;
        for (auto trace_index : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {
            if (tidy_result.can_be_removed[trace_index])
            {
                waypoints.values.push_back(util::json::Null());
                continue;
            }
            auto matching_index = trace_idx_to_matching_idx[trace_index];
            if (matching_index.NotMatched())
            {
                waypoints.values.push_back(util::json::Null());
                continue;
            }
            const auto &phantom =
                sub_matchings[matching_index.sub_matching_index].nodes[matching_index.point_index];
            auto waypoint = BaseAPI::MakeWaypoint(phantom);
            waypoint.values["matchings_index"] = matching_index.sub_matching_index;
            waypoint.values["waypoint_index"] = matching_index.point_index;
            waypoint.values["alternatives_count"] =
                sub_matchings[matching_index.sub_matching_index]
                    .alternatives_count[matching_index.point_index];
            // waypoint indices need to be adjusted if route legs were collapsed
            // waypoint parameter assumes there is only one match object
            if (!parameters.waypoints.empty())
            {
                if (tidy_result.was_waypoint[trace_index])
                {
                    waypoint.values["waypoint_index"] = was_waypoint_idx;
                    was_waypoint_idx++;
                }
                else
                {
                    waypoint.values["waypoint_index"] = util::json::Null();
                }
            }
            waypoints.values.push_back(std::move(waypoint));
        }

        return waypoints;
    }

    std::vector<MatchingIndex>
    MakeMatchingIndices(const std::vector<map_matching::SubMatching> &sub_matchings) const
    {
        std::vector<MatchingIndex> trace_idx_to_matching_idx(parameters.coordinates.size());
        for (auto sub_matching_index :
             util::irange(0u, static_cast<unsigned>(sub_matchings.size())))
        {
            for (auto point_index : util::irange(
                     0u, static_cast<unsigned>(sub_matchings[sub_matching_index].indices.size())))
            {
                // tidied_to_original: index of the input coordinate that a tidied coordinate
                // corresponds to.
                // sub_matching indices: index of the coordinate passed to map matching plugin that
                // a matched node corresponds to.
                trace_idx_to_matching_idx[tidy_result
                                              .tidied_to_original[sub_matchings[sub_matching_index]
                                                                      .indices[point_index]]] =
                    MatchingIndex{sub_matching_index, point_index};
            }
        }
        return trace_idx_to_matching_idx;
    }

    const MatchParameters &parameters;
    const tidy::Result &tidy_result;
};

} // ns api
} // ns engine
} // ns osrm

#endif
