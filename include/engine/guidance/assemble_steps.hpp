#ifndef ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_
#define ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/travel_mode.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/guidance/step_maneuver.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/typedefs.hpp"

#include <boost/optional.hpp>
#include <cstddef>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{
namespace detail
{
std::pair<short, short> getDepartBearings(const LegGeometry &leg_geometry);
std::pair<short, short> getArriveBearings(const LegGeometry &leg_geometry);
} // ns detail

inline std::vector<RouteStep> assembleSteps(const datafacade::BaseDataFacade &facade,
                                            const std::vector<PathData> &leg_data,
                                            const LegGeometry &leg_geometry,
                                            const PhantomNode &source_node,
                                            const PhantomNode &target_node,
                                            const bool source_traversed_in_reverse,
                                            const bool target_traversed_in_reverse)
{
    const double constexpr ZERO_DURATION = 0., ZERO_DISTANCE = 0.;
    const constexpr char *NO_ROTARY_NAME = "";
    const EdgeWeight source_duration =
        source_traversed_in_reverse ? source_node.reverse_weight : source_node.forward_weight;
    const auto source_mode = source_traversed_in_reverse ? source_node.backward_travel_mode
                                                         : source_node.forward_travel_mode;

    const EdgeWeight target_duration =
        target_traversed_in_reverse ? target_node.reverse_weight : target_node.forward_weight;
    const auto target_mode = target_traversed_in_reverse ? target_node.backward_travel_mode
                                                         : target_node.forward_travel_mode;

    const auto number_of_segments = leg_geometry.GetNumberOfSegments();

    std::vector<RouteStep> steps;
    steps.reserve(number_of_segments);

    std::size_t segment_index = 0;
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);

    auto bearings = detail::getDepartBearings(leg_geometry);

    StepManeuver maneuver{source_node.location,
                          bearings.first,
                          bearings.second,
                          extractor::guidance::TurnInstruction::NO_TURN(),
                          WaypointType::Depart,
                          0};

    Intersection intersection{source_node.location,
                              std::vector<short>({bearings.second}),
                              std::vector<bool>({true}),
                              Intersection::NO_INDEX,
                              0,
                              util::guidance::LaneTuple(),
                              {}};

    if (leg_data.size() > 0)
    {
        // PathData saves the information we need of the segment _before_ the turn,
        // but a RouteStep is with regard to the segment after the turn.
        // We need to skip the first segment because it is already covered by the
        // initial start of a route
        int segment_duration = 0;

        // some name changes are not announced in our processing. For these, we have to keep the
        // first name on the segment
        auto step_name_id = source_node.name_id;
        for (std::size_t leg_data_index = 0; leg_data_index < leg_data.size(); ++leg_data_index)
        {
            const auto &path_point = leg_data[leg_data_index];
            segment_duration += path_point.duration_until_turn;

            // all changes to this check have to be matched with assemble_geometry
            if (path_point.turn_instruction.type != extractor::guidance::TurnType::NoTurn)
            {
                BOOST_ASSERT(segment_duration >= 0);
                const auto name = facade.GetNameForID(step_name_id);
                const auto ref = facade.GetRefForID(step_name_id);
                const auto pronunciation = facade.GetPronunciationForID(step_name_id);
                const auto destinations = facade.GetDestinationsForID(step_name_id);
                const auto distance = leg_geometry.segment_distances[segment_index];

                steps.push_back(RouteStep{step_name_id,
                                          std::move(name),
                                          std::move(ref),
                                          std::move(pronunciation),
                                          std::move(destinations),
                                          NO_ROTARY_NAME,
                                          NO_ROTARY_NAME,
                                          segment_duration / 10.0,
                                          distance,
                                          path_point.travel_mode,
                                          maneuver,
                                          leg_geometry.FrontIndex(segment_index),
                                          leg_geometry.BackIndex(segment_index) + 1,
                                          {intersection}});

                if (leg_data_index + 1 < leg_data.size())
                {
                    step_name_id = leg_data[leg_data_index + 1].name_id;
                }
                else
                {
                    step_name_id = target_node.name_id;
                }

                // extract bearings
                bearings = std::make_pair<std::uint16_t, std::uint16_t>(
                    path_point.pre_turn_bearing.Get(), path_point.post_turn_bearing.Get());
                const auto entry_class = facade.GetEntryClass(path_point.entry_classid);
                const auto bearing_class =
                    facade.GetBearingClass(facade.GetBearingClassID(path_point.turn_via_node));
                auto bearing_data = bearing_class.getAvailableBearings();
                intersection.in = bearing_class.findMatchingBearing(bearings.first);
                intersection.out = bearing_class.findMatchingBearing(bearings.second);
                intersection.location = facade.GetCoordinateOfNode(path_point.turn_via_node);
                intersection.bearings.clear();
                intersection.bearings.reserve(bearing_class.getAvailableBearings().size());
                intersection.lanes = path_point.lane_data.first;
                intersection.lane_description =
                    path_point.lane_data.second != INVALID_LANE_DESCRIPTIONID
                        ? facade.GetTurnDescription(path_point.lane_data.second)
                        : extractor::guidance::TurnLaneDescription();
                std::copy(bearing_class.getAvailableBearings().begin(),
                          bearing_class.getAvailableBearings().end(),
                          std::back_inserter(intersection.bearings));
                intersection.entry.clear();
                for (auto idx : util::irange<std::size_t>(0, intersection.bearings.size()))
                {
                    intersection.entry.push_back(entry_class.allowsEntry(idx));
                }
                std::int16_t bearing_in_driving_direction =
                    util::reverseBearing(std::round(bearings.first));
                maneuver = {intersection.location,
                            bearing_in_driving_direction,
                            bearings.second,
                            path_point.turn_instruction,
                            WaypointType::None,
                            0};
                segment_index++;
                segment_duration = 0;
            }
        }
        const auto distance = leg_geometry.segment_distances[segment_index];
        const int duration = segment_duration + target_duration;
        BOOST_ASSERT(duration >= 0);
        steps.push_back(RouteStep{step_name_id,
                                  facade.GetNameForID(step_name_id),
                                  facade.GetRefForID(step_name_id),
                                  facade.GetPronunciationForID(step_name_id),
                                  facade.GetDestinationsForID(step_name_id),
                                  NO_ROTARY_NAME,
                                  NO_ROTARY_NAME,
                                  duration / 10.,
                                  distance,
                                  target_mode,
                                  maneuver,
                                  leg_geometry.FrontIndex(segment_index),
                                  leg_geometry.BackIndex(segment_index) + 1,
                                  {intersection}});
    }
    // In this case the source + target are on the same edge segment
    else
    {
        BOOST_ASSERT(source_node.fwd_segment_position == target_node.fwd_segment_position);
        //     s     t
        // u-------------v
        // |---| source_duration
        // |---------| target_duration

        int duration = target_duration - source_duration;
        BOOST_ASSERT(duration >= 0);

        steps.push_back(RouteStep{source_node.name_id,
                                  facade.GetNameForID(source_node.name_id),
                                  facade.GetRefForID(source_node.name_id),
                                  facade.GetPronunciationForID(source_node.name_id),
                                  facade.GetDestinationsForID(source_node.name_id),
                                  NO_ROTARY_NAME,
                                  NO_ROTARY_NAME,
                                  duration / 10.,
                                  leg_geometry.segment_distances[segment_index],
                                  source_mode,
                                  std::move(maneuver),
                                  leg_geometry.FrontIndex(segment_index),
                                  leg_geometry.BackIndex(segment_index) + 1,
                                  {intersection}});
    }

    BOOST_ASSERT(segment_index == number_of_segments - 1);
    bearings = detail::getArriveBearings(leg_geometry);

    intersection = {target_node.location,
                    std::vector<short>({static_cast<short>(util::reverseBearing(bearings.first))}),
                    std::vector<bool>({true}),
                    0,
                    Intersection::NO_INDEX,
                    util::guidance::LaneTuple(),
                    {}};

    // This step has length zero, the only reason we need it is the target location
    maneuver = {intersection.location,
                bearings.first,
                bearings.second,
                extractor::guidance::TurnInstruction::NO_TURN(),
                WaypointType::Arrive,
                0};

    BOOST_ASSERT(!leg_geometry.locations.empty());
    steps.push_back(RouteStep{target_node.name_id,
                              facade.GetNameForID(target_node.name_id),
                              facade.GetRefForID(target_node.name_id),
                              facade.GetPronunciationForID(target_node.name_id),
                              facade.GetDestinationsForID(target_node.name_id),
                              NO_ROTARY_NAME,
                              NO_ROTARY_NAME,
                              ZERO_DURATION,
                              ZERO_DISTANCE,
                              target_mode,
                              std::move(maneuver),
                              leg_geometry.locations.size() - 1,
                              leg_geometry.locations.size(),
                              {intersection}});

    BOOST_ASSERT(steps.front().intersections.size() == 1);
    BOOST_ASSERT(steps.front().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.front().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.front().maneuver.waypoint_type == WaypointType::Depart);

    BOOST_ASSERT(steps.back().intersections.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.back().maneuver.waypoint_type == WaypointType::Arrive);
    BOOST_ASSERT(steps.back().intersections.front().lanes.lanes_in_turn == 0);
    BOOST_ASSERT(steps.back().intersections.front().lanes.first_lane_from_the_right ==
                 INVALID_LANEID);
    BOOST_ASSERT(steps.back().intersections.front().lane_description.empty());
    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
