#ifndef ENGINE_GUIDANCE_ASSEMBLE_LEG_HPP_
#define ENGINE_GUIDANCE_ASSEMBLE_LEG_HPP_

#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_leg.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/internal_route_result.hpp"

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

inline RouteLeg assembleLeg(const std::vector<PathData> &route_data,
                            const LegGeometry &leg_geometry,
                            const PhantomNode &source_node,
                            const PhantomNode &target_node,
                            const bool target_traversed_in_reverse)
{
    const auto target_duration =
        (target_traversed_in_reverse ? target_node.reverse_weight : target_node.forward_weight) /
        10.;

    auto distance = std::accumulate(leg_geometry.segment_distances.begin(),
                                    leg_geometry.segment_distances.end(), 0.);
    auto duration = std::accumulate(route_data.begin(), route_data.end(), 0.,
                                    [](const double sum, const PathData &data) {
                                        return sum + data.duration_until_turn;
                                    }) /
                    10.;

    //                 s
    //                 |
    // Given a route a---b---c  where there is a right turn at c.
    //                       |
    //                       d
    //                       |--t
    //                       e
    // (a, b, c) gets compressed to (a,c)
    // (c, d, e) gets compressed to (c,e)
    // The duration of the turn (a,c) -> (c,e) will be the duration of (a,c) (e.g. the duration
    // of (a,b,c)).
    // The phantom node of s will contain:
    // `forward_weight`: duration of (a,s)
    // `forward_offset`: 0 (its the first segment)
    // The phantom node of t will contain:
    // `forward_weight`: duration of (d,t)
    // `forward_offset`: duration of (c, d)
    // path_data will have entries for (s,b), (b, c), (c, d) but (d, t) is only
    // caputed by the phantom node. So we need to add the target duration here.
    // On local segments, the target duration is already part of the duration, however.

    duration = duration + target_duration;
    if (route_data.empty())
    {
        duration -= (target_traversed_in_reverse ? source_node.reverse_weight
                                                 : source_node.forward_weight) /
                    10;
    }

    return RouteLeg{duration, distance, {}};
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
