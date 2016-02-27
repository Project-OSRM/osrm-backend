#ifndef ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
#define ENGINE_GUIDANCE_SEGMENT_LIST_HPP_

#include "osrm/coordinate.hpp"

#include "engine/segment_information.hpp"
#include "engine/douglas_peucker.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/segment_information.hpp"
#include "util/integer_range.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/for_each_pair.hpp"
#include "util/bearing.hpp"
#include "guidance/turn_instruction.hpp"
#include "guidance/guidance_toolkit.hpp"

#include <boost/assert.hpp>

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <vector>

// transfers the internal edge based data structures to a more useable format
namespace osrm
{
namespace guidance
{

template <typename DataFacadeT> class SegmentList
{
  public:
    using DataFacade = DataFacadeT;
    SegmentList(const engine::InternalRouteResult &raw_route,
                const bool extract_alternative,
                int zoom_level,
                bool allow_simplification,
                const DataFacade *facade,
                const std::vector<FixedPointCoordinate> &query_coordinates);

    const std::vector<std::uint32_t> &GetViaIndices() const;
    std::uint32_t GetDistance() const;
    std::uint32_t GetDuration() const;

    const std::vector<engine::SegmentInformation> &Get() const;

  private:
    void InitRoute(const engine::PhantomNode &phantom_node, const bool traversed_in_reverse);
    void AddLeg(const std::vector<engine::PathData> &leg_data,
                const engine::PhantomNode &target_node,
                const bool traversed_in_reverse,
                const bool is_via_leg,
                const DataFacade *facade,
                const FixedPointCoordinate queried_coordinate);

    void AppendSegment(const FixedPointCoordinate coordinate, const engine::PathData &path_point);
    void Finalize(const bool extract_alternative,
                  const engine::InternalRouteResult &raw_route,
                  const int zoom_level,
                  const bool allow_simplification);

    // journey length in tenth of a second
    std::uint32_t total_distance;
    // journey distance in meter (TODO: verify)
    std::uint32_t total_duration;

    // segments that are required to keep
    std::vector<std::uint32_t> via_indices;

    // a list of node based segments

    std::vector<engine::SegmentInformation> segments;
};

template <typename DataFacadeT>
SegmentList<DataFacadeT>::SegmentList(const engine::InternalRouteResult &raw_route,
                                      const bool extract_alternative,
                                      const int zoom_level,
                                      const bool allow_simplification,
                                      const DataFacade *facade,
                                      const std::vector<FixedPointCoordinate> &query_coordinates)
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
               raw_route.alt_source_traversed_in_reverse.back(), false, facade,
               query_coordinates.back());
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
                   raw_route.is_via_leg(raw_index), facade, query_coordinates[raw_index + 1]);
            /* TODO check why this is here in the first place
            if (raw_route.is_via_leg(raw_index))
            {
                const auto &source_phantom =
                    raw_route.segment_end_coordinates[raw_index].target_phantom;
                if (raw_route.target_traversed_in_reverse[raw_index] !=
                    raw_route.source_traversed_in_reverse[raw_index + 1])
                {
                    bool traversed_in_reverse = raw_route.target_traversed_in_reverse[raw_index];
                    const extractor::TravelMode travel_mode =
                        (traversed_in_reverse ? source_phantom.backward_travel_mode
                                              : source_phantom.forward_travel_mode);
                    const bool constexpr IS_NECESSARY = true;
                    const bool constexpr IS_VIA_LOCATION = true;
                    // add a modifier if the destination is at least some distance away
                    const auto dist_to_queried_location =
                        util::coordinate_calculation::haversineDistance(
                            source_phantom.location, query_coordinates[raw_index + 1]);
                    const auto bearing = util::coordinate_calculation::bearing(
                        query_coordinates[raw_index + 1], source_phantom.location);
                    const DirectionModifier modifier =
                        (5 < dist_to_queried_location && dist_to_queried_location < 200)
                            ? angleToDirectionModifier(bearing)
                            : DirectionModifier::UTurn;
                    segments.emplace_back(source_phantom.location, source_phantom.name_id, 0, 0.f,
                                          TurnInstruction(TurnType::Location, modifier),
                                          IS_NECESSARY, IS_VIA_LOCATION, travel_mode);
                }
            }
            */
        }
    }

    // set direction of last coordinate
    const auto &target_node = raw_route.segment_end_coordinates.back().target_phantom;
    const auto dist_to_queried_location = util::coordinate_calculation::haversineDistance(
        target_node.location, query_coordinates.back());
    const auto getPrevCoordinate = [&](const FixedPointCoordinate coordinate)
    {
        for (auto itr = segments.rbegin(); itr != segments.rend(); ++itr)
        {
            const auto dist =
                util::coordinate_calculation::haversineDistance(coordinate, itr->location);
            if (dist > 0)
            {
                return itr->location;
            }
        }
        return segments.back().location;
    };

    Finalize(extract_alternative, raw_route, zoom_level, allow_simplification);

    const auto bearing = util::coordinate_calculation::computeAngle(
        getPrevCoordinate(target_node.location), target_node.location, query_coordinates.back());
    const DirectionModifier modifier =
        (5 < dist_to_queried_location && dist_to_queried_location < 200)
            ? angleToDirectionModifier(bearing)
            : DirectionModifier::UTurn;
    segments.back().turn_instruction.direction_modifier = modifier;

    std::cout << "[all segments] " << std::endl;
    for (auto s : segments)
        std::cout << "\t" << (int)s.turn_instruction.type << " "
                  << (int)s.turn_instruction.direction_modifier << " Name: " << s.name_id
                  << " Required: " << s.necessary << " Duration: " << s.duration
                  << " Length: " << s.length << std::endl;
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::InitRoute(const engine::PhantomNode &node,
                                         const bool traversed_in_reverse)
{
    const auto segment_duration =
        (traversed_in_reverse ? node.reverse_weight : node.forward_weight);
    const auto travel_mode =
        (traversed_in_reverse ? node.backward_travel_mode : node.forward_travel_mode);

    AppendSegment(node.location,
                  engine::PathData(0, node.name_id,
                                   TurnInstruction(TurnType::Location, DirectionModifier::Straight),
                                   segment_duration, travel_mode));
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::AddLeg(const std::vector<engine::PathData> &leg_data,
                                      const engine::PhantomNode &target_node,
                                      const bool traversed_in_reverse,
                                      const bool is_via_leg,
                                      const DataFacade *facade,
                                      const FixedPointCoordinate queried_coordinate)
{
    for (const auto &path_data : leg_data)
    {
        AppendSegment(facade->GetCoordinateOfNode(path_data.node), path_data);
    }

    const EdgeWeight segment_duration =
        (traversed_in_reverse ? target_node.reverse_weight : target_node.forward_weight);
    const extractor::TravelMode travel_mode =
        (traversed_in_reverse ? target_node.backward_travel_mode : target_node.forward_travel_mode);
    const bool constexpr IS_NECESSARY = true;
    const bool constexpr IS_VIA_LOCATION = true;

    const auto dist_to_queried_location =
        util::coordinate_calculation::haversineDistance(target_node.location, queried_coordinate);

    const auto getPrevCoordinate = [&](const FixedPointCoordinate coordinate)
    {
        for (auto itr = segments.rbegin(); itr != segments.rend(); ++itr)
        {
            const auto dist =
                util::coordinate_calculation::haversineDistance(coordinate, itr->location);
            if (dist > 0)
            {
                return itr->location;
            }
        }
        return segments.back().location;
    };
    const auto bearing = util::coordinate_calculation::computeAngle(
        getPrevCoordinate(target_node.location), target_node.location, queried_coordinate);
    const DirectionModifier modifier =
        (5 < dist_to_queried_location && dist_to_queried_location < 200)
            ? angleToDirectionModifier(bearing)
            : DirectionModifier::UTurn;
    segments.emplace_back(target_node.location, target_node.name_id, segment_duration, 0.f,
                          is_via_leg ? TurnInstruction(TurnType::Location, modifier)
                                     : TurnInstruction::NO_TURN(),
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
std::vector<engine::SegmentInformation> const &SegmentList<DataFacadeT>::Get() const
{
    return segments;
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::AppendSegment(const FixedPointCoordinate coordinate,
                                             const engine::PathData &path_point)
{
    auto turn = path_point.turn_instruction;

    const auto prevInstruction = [this]() -> engine::SegmentInformation &
    {
        for (auto itr = segments.rbegin(); itr != segments.rend(); ++itr)
            if (itr->turn_instruction.type != TurnType::NoTurn)
                return *itr;

        // this point is unreachable, since we have a valid start instructions
        BOOST_ASSERT(false);
        return *segments.rend();
    };

    if (!segments.empty() && turn.type == TurnType::EndOfRoad)
    {
        const auto prev = prevInstruction().turn_instruction;
        if (prev.type != TurnType::Suppressed)
            turn.type = TurnType::Turn;
    }

    /*
    if (segments.size() > 1 && (turn.type == TurnType::Suppressed || turn.type == TurnType::Turn) &&
        isLeftRight(turn.direction_modifier) && path_point.name_id == segments.back().name_id)
    {
        auto &prev = segments.back();
        const auto distance =
            util::coordinate_calculation::greatCircleDistance(coordinate, prev.location);
        std::cout << "Distance: " << distance << std::endl;
        if (distance < 30)
        {
            turn.type = TurnType::Turn;
            if( isSlightLeftRight(prev.turn_instruction.direction_modifier) || prev.turn_instruction.direction_modifier == DirectionModifier::Straight )
              prev.turn_instruction.type = TurnType::Suppressed;
        }
    }
    */
    /*

        auto &previous = prevInstruction();

        if (path_point.turn_instruction.type == TurnType::NewName &&
            previous.turn_instruction.type == TurnType::Ramp && previous.name_id == 0)
        {
            previous.name_id = path_point.name_id;
            turn.type = TurnType::Continue;
        }
    */
    segments.emplace_back(coordinate, path_point.name_id, path_point.segment_duration, 0.f, turn,
                          path_point.travel_mode);
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::Finalize(const bool extract_alternative,
                                        const engine::InternalRouteResult &raw_route,
                                        const int zoom_level,
                                        const bool allow_simplification)
{
    if (segments.empty())
        return;

    // check if first two segments can be merged
    BOOST_ASSERT(segments.size() >= 2);
    if (segments[0].location == segments[1].location &&
        segments[1].turn_instruction.type == TurnType::NoTurn)
    {
        segments[0].travel_mode = segments[1].travel_mode;
        segments[0].name_id = segments[1].name_id;
        // Other data??
        segments.erase(segments.begin() + 1);
    }

    // announce mode changes
    for (std::size_t i = 0; i + 1 < segments.size(); ++i)
    {
        auto &segment = segments[i];
        const auto next_mode = segments[i + 1].travel_mode;
        if (segment.travel_mode != next_mode && isTurnNecessary(segment.turn_instruction))
        {
            segment.turn_instruction.type = TurnType::Notification;
            segment.necessary = true;
        }
    }

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

        if (isTurnNecessary(segments[i].turn_instruction))
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

    // TODO check: are the next to if's ever used???
    // Post-processing to remove empty or nearly empty path segments
    if (segments.size() > 2 && std::numeric_limits<float>::epsilon() > segments.back().length &&
        !(segments.end() - 2)->is_via_location)
    {
        segments.pop_back();
        segments.back().necessary = true;
        segments.back().turn_instruction.type = TurnType::NoTurn;
    }

    if (segments.size() > 2 && std::numeric_limits<float>::epsilon() > segments.front().length &&
        !(segments.begin() + 1)->is_via_location)
    {
        segments.erase(segments.begin());
        segments.front().turn_instruction.type = TurnType::Location;
        segments.front().turn_instruction.direction_modifier = DirectionModifier::Straight;
        segments.front().necessary = true;
    }

    if (allow_simplification)
    {
        douglasPeucker(segments, zoom_level);
    }

    std::uint32_t necessary_segments = 0; // a running index that counts the necessary pieces
    via_indices.push_back(0);
    const auto markNecessarySegments = [this, &necessary_segments](
        engine::SegmentInformation &first, const engine::SegmentInformation &second)
    {
        if (!first.necessary)
            return;

        // mark the end of a leg (of several segments)
        if (first.is_via_location)
            via_indices.push_back(necessary_segments);

        // TODO this is completely buggy, if segments are very short
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
    segments[0].turn_instruction.direction_modifier =
        bearingToDirectionModifier(util::bearing::get(segments[0].post_turn_bearing / 10));
    via_indices.push_back(necessary_segments);

    BOOST_ASSERT(via_indices.size() >= 2);
}

template <typename DataFacadeT>
SegmentList<DataFacadeT> MakeSegmentList(const engine::InternalRouteResult &raw_route,
                                         const bool extract_alternative,
                                         const DataFacadeT *facade)
{
    return SegmentList<DataFacadeT>(raw_route, extract_alternative, facade);
}

} // namespace guidance
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
