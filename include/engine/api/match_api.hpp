#ifndef ENGINE_API_MATCH_HPP
#define ENGINE_API_MATCH_HPP

#include "engine/api/match_parameters.hpp"
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
    MatchAPI(const datafacade::BaseDataFacade &facade_, const MatchParameters &parameters_)
        : RouteAPI(facade_, parameters_), parameters(parameters_)
    {
    }

    void MakeResponse(const std::vector<map_matching::SubMatching> &sub_matchings,
                      std::vector<InternalRouteResult> &sub_routes,
                      const std::vector<unsigned int> & timestamps,
                      util::json::Object &response) const
    {
        auto number_of_routes = sub_matchings.size();
        util::json::Array routes;
        routes.values.reserve(number_of_routes);
        BOOST_ASSERT(sub_matchings.size() == sub_routes.size());
        for (auto index : util::irange<std::size_t>(0UL, number_of_routes))
        {
            if (!timestamps.empty())
            {
                // Reannotate segments duration.
                for (auto path_segments_index : util::irange<std::size_t>(0UL, sub_routes[index].unpacked_path_segments.size()))
                {
                    int & source_weight =
                        sub_routes[index].source_traversed_in_reverse[path_segments_index] ? sub_routes[index].segment_end_coordinates[path_segments_index].source_phantom.reverse_weight
                                                                                           : sub_routes[index].segment_end_coordinates[path_segments_index].source_phantom.forward_weight;
                    int & target_weight =
                        sub_routes[index].target_traversed_in_reverse[path_segments_index] ? sub_routes[index].segment_end_coordinates[path_segments_index].target_phantom.reverse_weight
                                                                                           : sub_routes[index].segment_end_coordinates[path_segments_index].target_phantom.forward_weight;

                    int real_duration = timestamps[sub_matchings[index].indices[path_segments_index] + 1] - timestamps[sub_matchings[index].indices[path_segments_index]];

                    auto & path_segments = sub_routes[index].unpacked_path_segments[path_segments_index];
                    if (path_segments.empty())
                    {
                        source_weight = 0;
                        target_weight = real_duration * 10;
                    }
                    else
                    {
                        EdgeWeight path_duration = target_weight;
                        for (auto const & segment : path_segments)
                            path_duration += segment.duration_until_turn;

                        float time_multiplier = real_duration * 10.0 / path_duration;
                        target_weight *= time_multiplier;
                        for (auto & segment : path_segments)
                            segment.duration_until_turn *= time_multiplier;
                    }
                }
            }

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

    // FIXME gcc 4.8 doesn't support for lambdas to call protected member functions
    //  protected:

    // FIXME this logic is a little backwards. We should change the output format of the
    // map_matching
    // routing algorithm to be easier to consume here.
    util::json::Array
    MakeTracepoints(const std::vector<map_matching::SubMatching> &sub_matchings) const
    {
        util::json::Array waypoints;
        waypoints.values.reserve(parameters.coordinates.size());

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

        std::vector<MatchingIndex> trace_idx_to_matching_idx(parameters.coordinates.size());
        for (auto sub_matching_index :
             util::irange(0u, static_cast<unsigned>(sub_matchings.size())))
        {
            for (auto point_index : util::irange(
                     0u, static_cast<unsigned>(sub_matchings[sub_matching_index].indices.size())))
            {
                trace_idx_to_matching_idx[sub_matchings[sub_matching_index].indices[point_index]] =
                    MatchingIndex{sub_matching_index, point_index};
            }
        }

        for (auto trace_index : util::irange<std::size_t>(0UL, parameters.coordinates.size()))
        {
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
            waypoints.values.push_back(std::move(waypoint));
        }

        return waypoints;
    }

    const MatchParameters &parameters;
};

} // ns api
} // ns engine
} // ns osrm

#endif
