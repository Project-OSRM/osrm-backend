#ifndef ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
#define ENGINE_GUIDANCE_SEGMENT_LIST_HPP_

#include "osrm/coordinate.hpp"

#include "engine/douglas_peucker.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/segment_information.hpp"
#include "util/integer_range.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/for_each_pair.hpp"
#include "extractor/turn_instructions.hpp"

#include <boost/assert.hpp>

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <vector>

// transfers the internal edge based data structures to a more useable format
namespace osrm
{
namespace engine
{
namespace guidance
{
template <typename DataFacadeT> class SegmentList
{
  public:
    using DataFacade = DataFacadeT;
    SegmentList(const InternalRouteResult &raw_route,
                const bool extract_alternative,
                const unsigned zoom_level,
                const bool allow_simplification,
                const DataFacade *facade);

    const std::vector<std::uint32_t> &GetViaIndices() const;
    std::uint32_t GetDistance() const;
    std::uint32_t GetDuration() const;

    const std::vector<SegmentInformation> &Get() const;

  private:
    void InitRoute(const PhantomNode &phantom_node, const bool traversed_in_reverse);
    void AddLeg(const std::vector<PathData> &leg_data,
                const PhantomNode &target_node,
                const bool traversed_in_reverse,
                const bool is_via_leg,
                const DataFacade *facade);

    void AppendSegment(const FixedPointCoordinate coordinate, const PathData &path_point);
    void Finalize(const bool extract_alternative,
                  const InternalRouteResult &raw_route,
                  const unsigned zoom_level,
                  const bool allow_simplification);

    // journey length in tenth of a second
    std::uint32_t total_distance;
    // journey distance in meter (TODO: verify)
    std::uint32_t total_duration;

    // segments that are required to keep
    std::vector<std::uint32_t> via_indices;

    // a list of node based segments
    std::vector<SegmentInformation> segments;
};

template <typename DataFacadeT>
SegmentList<DataFacadeT>::SegmentList(const InternalRouteResult &raw_route,
                                      const bool extract_alternative,
                                      const unsigned zoom_level,
                                      const bool allow_simplification,
                                      const DataFacade *facade)
    : total_distance(0), total_duration(0)
{
    if (!raw_route.is_valid())
    {
        return;
    }

    if (extract_alternative)
    {
        BOOST_ASSERT(raw_route.has_alternative());
        InitRoute(raw_route.segment_end_coordinates.front().source_phantom,
                  raw_route.alt_source_traversed_in_reverse.front());
        AddLeg(raw_route.unpacked_alternative,
               raw_route.segment_end_coordinates.back().target_phantom,
               raw_route.alt_source_traversed_in_reverse.back(), false, facade);
    }
    else
    {
        InitRoute(raw_route.segment_end_coordinates.front().source_phantom,
                  raw_route.source_traversed_in_reverse.front());
        for (std::size_t raw_index = 0; raw_index < raw_route.segment_end_coordinates.size();
             ++raw_index)
        {
            AddLeg(raw_route.unpacked_path_segments[raw_index],
                   raw_route.segment_end_coordinates[raw_index].target_phantom,
                   raw_route.target_traversed_in_reverse[raw_index],
                   raw_route.is_via_leg(raw_index), facade);
            if (raw_route.is_via_leg(raw_index))
            {
                const auto &source_phantom =
                    raw_route.segment_end_coordinates[raw_index].target_phantom;
                if (raw_route.target_traversed_in_reverse[raw_index] != raw_route.source_traversed_in_reverse[raw_index+1])
                {
                    bool traversed_in_reverse = raw_route.target_traversed_in_reverse[raw_index];
                    const extractor::TravelMode travel_mode =
                        (traversed_in_reverse ? source_phantom.backward_travel_mode
                                              : source_phantom.forward_travel_mode);
                    const bool constexpr IS_NECESSARY = true;
                    const bool constexpr IS_VIA_LOCATION = true;
                    segments.emplace_back(source_phantom.location, source_phantom.name_id, 0, 0.f,
                                          extractor::TurnInstruction::UTurn, IS_NECESSARY,
                                          IS_VIA_LOCATION, travel_mode);
                }
            }
        }
    }

    if (!allow_simplification)
    {
        // to prevent any simplifications, we mark all segments as necessary
        for (auto &segment : segments)
        {
            segment.necessary = true;
        }
    }

    Finalize(extract_alternative, raw_route, zoom_level, allow_simplification);
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::InitRoute(const PhantomNode &node, const bool traversed_in_reverse)
{
    const auto segment_duration =
        (traversed_in_reverse ? node.reverse_weight : node.forward_weight);
    const auto travel_mode =
        (traversed_in_reverse ? node.backward_travel_mode : node.forward_travel_mode);

    AppendSegment(node.location, PathData(0, node.name_id, extractor::TurnInstruction::HeadOn,
                                          segment_duration, travel_mode));
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::AddLeg(const std::vector<PathData> &leg_data,
                                      const PhantomNode &target_node,
                                      const bool traversed_in_reverse,
                                      const bool is_via_leg,
                                      const DataFacade *facade)
{
    for (const PathData &path_data : leg_data)
    {
        AppendSegment(facade->GetCoordinateOfNode(path_data.node), path_data);
    }

    const EdgeWeight segment_duration =
        (traversed_in_reverse ? target_node.reverse_weight : target_node.forward_weight);
    const extractor::TravelMode travel_mode =
        (traversed_in_reverse ? target_node.backward_travel_mode : target_node.forward_travel_mode);
    const bool constexpr IS_NECESSARY = true;
    const bool constexpr IS_VIA_LOCATION = true;
    segments.emplace_back(target_node.location, target_node.name_id, segment_duration, 0.f,
                          is_via_leg ? extractor::TurnInstruction::ReachViaLocation
                                     : extractor::TurnInstruction::NoTurn,
                          IS_NECESSARY, IS_VIA_LOCATION, travel_mode);
}

template <typename DataFacadeT> std::uint32_t SegmentList<DataFacadeT>::GetDistance() const
{
    return total_distance;
}
template <typename DataFacadeT> std::uint32_t SegmentList<DataFacadeT>::GetDuration() const
{
    return total_duration;
}

template <typename DataFacadeT>
std::vector<std::uint32_t> const &SegmentList<DataFacadeT>::GetViaIndices() const
{
    return via_indices;
}

template <typename DataFacadeT>
std::vector<SegmentInformation> const &SegmentList<DataFacadeT>::Get() const
{
    return segments;
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::AppendSegment(const FixedPointCoordinate coordinate,
                                             const PathData &path_point)
{
    // if the start location is on top of a node, the first movement might be zero-length,
    // in which case we dont' add a new description, but instead update the existing one
    if ((1 == segments.size()) && (segments.front().location == coordinate))
    {
        if (path_point.segment_duration > 0)
        {
            segments.front().name_id = path_point.name_id;
            segments.front().travel_mode = path_point.travel_mode;
        }
        return;
    }

    // make sure mode changes are announced, even when there otherwise is no turn
    const auto getTurn = [](const PathData &path_point, const extractor::TravelMode previous_mode)
    {
        if (extractor::TurnInstruction::NoTurn == path_point.turn_instruction &&
            previous_mode != path_point.travel_mode && path_point.segment_duration > 0)
        {
            return extractor::TurnInstruction::GoStraight;
        }
        return path_point.turn_instruction;
    };

    // TODO check why we require .front() here
    const auto turn = segments.size() ? getTurn(path_point, segments.front().travel_mode)
                                      : path_point.turn_instruction;

    segments.emplace_back(coordinate, path_point.name_id, path_point.segment_duration, 0.f, turn,
                          path_point.travel_mode);
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::Finalize(const bool extract_alternative,
                                        const InternalRouteResult &raw_route,
                                        const unsigned zoom_level,
                                        const bool allow_simplification)
{
    if (segments.empty())
        return;

    segments[0].length = 0.f;
    for (const auto i : util::irange<std::size_t>(1, segments.size()))
    {
        // move down names by one, q&d hack
        segments[i - 1].name_id = segments[i].name_id;
        segments[i].length = util::coordinate_calculation::greatCircleDistance(
            segments[i - 1].location, segments[i].location);
    }

    float segment_length = 0.;
    EdgeWeight segment_duration = 0;
    std::size_t segment_start_index = 0;

    double path_length = 0;

    for (const auto i : util::irange<std::size_t>(1, segments.size()))
    {
        path_length += segments[i].length;
        segment_length += segments[i].length;
        segment_duration += segments[i].duration;
        segments[segment_start_index].length = segment_length;
        segments[segment_start_index].duration = segment_duration;

        if (extractor::TurnInstruction::NoTurn != segments[i].turn_instruction)
        {
            BOOST_ASSERT(segments[i].necessary);
            segment_length = 0;
            segment_duration = 0;
            segment_start_index = i;
        }
    }

    total_distance = static_cast<std::uint32_t>(std::round(path_length));
    total_duration = static_cast<std::uint32_t>(std::round(
        (extract_alternative ? raw_route.alternative_path_length : raw_route.shortest_path_length) /
        10.));

    // Post-processing to remove empty or nearly empty path segments
    if (segments.size() > 2 && std::numeric_limits<float>::epsilon() > segments.back().length &&
        !(segments.end() - 2)->is_via_location)
    {
        segments.pop_back();
        segments.back().necessary = true;
        segments.back().turn_instruction = extractor::TurnInstruction::NoTurn;
    }

    if (segments.size() > 2 && std::numeric_limits<float>::epsilon() > segments.front().length &&
        !(segments.begin() + 1)->is_via_location)
    {
        segments.erase(segments.begin());
        segments.front().turn_instruction = extractor::TurnInstruction::HeadOn;
        segments.front().necessary = true;
    }

    if (allow_simplification)
    {
        douglasPeucker(segments, zoom_level);
    }

    std::uint32_t necessary_segments = 0; // a running index that counts the necessary pieces
    via_indices.push_back(0);
    const auto markNecessarySegments = [this, &necessary_segments](SegmentInformation &first,
                                                                   const SegmentInformation &second)
    {
        if (!first.necessary)
            return;

        // mark the end of a leg (of several segments)
        if (first.is_via_location)
            via_indices.push_back(necessary_segments);

        const double post_turn_bearing =
            util::coordinate_calculation::bearing(first.location, second.location);
        const double pre_turn_bearing =
            util::coordinate_calculation::bearing(second.location, first.location);
        first.post_turn_bearing = static_cast<short>(post_turn_bearing * 10);
        first.pre_turn_bearing = static_cast<short>(pre_turn_bearing * 10);

        ++necessary_segments;
    };

    // calculate which segments are necessary and update segments for bearings
    util::for_each_pair(segments, markNecessarySegments);
    via_indices.push_back(necessary_segments);

    BOOST_ASSERT(via_indices.size() >= 2);
}

template <typename DataFacadeT>
SegmentList<DataFacadeT> MakeSegmentList(const InternalRouteResult &raw_route,
                                         const bool extract_alternative,
                                         const unsigned zoom_level,
                                         const bool allow_simplification,
                                         const DataFacadeT *facade)
{
    return SegmentList<DataFacadeT>(raw_route, extract_alternative, zoom_level,
                                    allow_simplification, facade);
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
