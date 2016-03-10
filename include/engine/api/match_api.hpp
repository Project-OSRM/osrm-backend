/*

Copyright (c) 2016, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef ENGINE_API_MATCH_HPP
#define ENGINE_API_MATCH_HPP

#include "engine/api/route_api.hpp"
#include "engine/api/match_parameters.hpp"

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
                      const std::vector<InternalRouteResult> &sub_routes,
                      util::json::Object &response) const
    {
        auto number_of_routes = sub_matchings.size();
        util::json::Array routes;
        routes.values.reserve(number_of_routes);
        BOOST_ASSERT(sub_matchings.size() == sub_routes.size());
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
        response.values["code"] = "ok";
    }

  protected:
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

        for (auto trace_index : util::irange(0UL, parameters.coordinates.size()))
        {
            auto matching_index = trace_idx_to_matching_idx[trace_index];
            if (matching_index.NotMatched())
            {
                waypoints.values.push_back(util::json::Null());
                continue;
            }
            const auto &phantom =
                sub_matchings[matching_index.sub_matching_index].nodes[matching_index.point_index];
            auto waypoint = BaseAPI::MakeWaypoint(parameters.coordinates[trace_index], phantom);
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
