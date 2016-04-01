#ifndef ENGINE_GUIDANCE_ASSEMBLE_LEG_HPP_
#define ENGINE_GUIDANCE_ASSEMBLE_LEG_HPP_

#include "engine/guidance/route_leg.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/internal_route_result.hpp"

#include <cstddef>
#include <cstdint>

#include <vector>
#include <array>
#include <string>
#include <utility>
#include <numeric>
#include <algorithm>

namespace osrm
{
namespace engine
{
namespace guidance
{
namespace detail
{
const constexpr std::size_t MAX_USED_SEGMENTS = 2;
struct NamedSegment
{
    double duration;
    std::uint32_t position;
    std::uint32_t name_id;
};

template <std::size_t SegmentNumber>
std::array<std::uint32_t, SegmentNumber> summarizeRoute(const std::vector<PathData> &route_data)
{
    // merges segments with same name id
    const auto collapse_segments = [](std::vector<NamedSegment> &segments)
    {
        auto out = segments.begin();
        auto end = segments.end();
        for (auto in = segments.begin(); in != end; ++in)
        {
            if (in->name_id == out->name_id)
            {
                out->duration += in->duration;
            }
            else
            {
                ++out;
                BOOST_ASSERT(out != end);
                *out = *in;
            }
        }
        return out;
    };

    std::vector<NamedSegment> segments(route_data.size());
    std::uint32_t index = 0;
    std::transform(
        route_data.begin(), route_data.end(), segments.begin(), [&index](const PathData &point)
        {
            return NamedSegment{point.duration_until_turn / 10.0, index++, point.name_id};
        });
    // this makes sure that the segment with the lowest position comes first
    std::sort(segments.begin(), segments.end(), [](const NamedSegment &lhs, const NamedSegment &rhs)
              {
                  return lhs.name_id < rhs.name_id ||
                         (lhs.name_id == rhs.name_id && lhs.position < rhs.position);
              });
    auto new_end = collapse_segments(segments);
    segments.resize(new_end - segments.begin());
    // sort descending
    std::sort(segments.begin(), segments.end(), [](const NamedSegment &lhs, const NamedSegment &rhs)
              {
                  return lhs.duration > rhs.duration;
              });

    // make sure the segments are sorted by position
    segments.resize(std::min(segments.size(), SegmentNumber));
    std::sort(segments.begin(), segments.end(), [](const NamedSegment &lhs, const NamedSegment &rhs)
              {
                  return lhs.position < rhs.position;
              });

    std::array<std::uint32_t, SegmentNumber> summary;
    std::fill(summary.begin(), summary.end(), 0);
    std::transform(segments.begin(), segments.end(), summary.begin(),
                   [](const NamedSegment &segment)
                   {
                       return segment.name_id;
                   });
    return summary;
}
}

template <typename DataFacadeT>
RouteLeg assembleLeg(const DataFacadeT &facade,
                     const std::vector<PathData> &route_data,
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
                                    [](const double sum, const PathData &data)
                                    {
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
        duration -=
            (target_traversed_in_reverse ? source_node.reverse_weight : source_node.forward_weight) / 10;
    }
    auto summary_array = detail::summarizeRoute<detail::MAX_USED_SEGMENTS>(route_data);

    BOOST_ASSERT(detail::MAX_USED_SEGMENTS > 0);
    BOOST_ASSERT(summary_array.begin() != summary_array.end());
    std::string summary =
        std::accumulate(std::next(summary_array.begin()), summary_array.end(),
                        facade.GetNameForID(summary_array.front()),
                        [&facade](std::string previous, const std::uint32_t name_id)
                        {
                            if (name_id != 0)
                            {
                                previous += ", " + facade.GetNameForID(name_id);
                            }
                            return previous;
                        });

    return RouteLeg{duration, distance, summary, {}};
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
