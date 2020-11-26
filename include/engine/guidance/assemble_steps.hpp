#ifndef ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_
#define ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_

#include "extractor/travel_mode.hpp"
#include "extractor/turn_lane_types.hpp"
#include "guidance/turn_instruction.hpp"
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
std::pair<short, short> getDepartBearings(const LegGeometry &leg_geometry,
                                          const PhantomNode &source_node,
                                          const bool traversed_in_reverse);
std::pair<short, short> getArriveBearings(const LegGeometry &leg_geometry,
                                          const PhantomNode &target_node,
                                          const bool traversed_in_reverse);
} // namespace detail

inline std::vector<RouteStep> assembleSteps(const datafacade::BaseDataFacade &facade,
                                            const std::vector<PathData> &leg_data,
                                            const LegGeometry &leg_geometry,
                                            const PhantomNode &source_node,
                                            const PhantomNode &target_node,
                                            const bool source_traversed_in_reverse,
                                            const bool target_traversed_in_reverse)
{
    const double weight_multiplier = facade.GetWeightMultiplier();

    const double constexpr ZERO_DURATION = 0., ZERO_DISTANCE = 0., ZERO_WEIGHT = 0;
    const constexpr char *NO_ROTARY_NAME = "";
    const EdgeWeight source_weight =
        source_traversed_in_reverse ? source_node.reverse_weight : source_node.forward_weight;
    const EdgeWeight source_duration =
        source_traversed_in_reverse ? source_node.reverse_duration : source_node.forward_duration;
    const auto source_node_id = source_traversed_in_reverse ? source_node.reverse_segment_id.id
                                                            : source_node.forward_segment_id.id;
    const auto source_name_id = facade.GetNameIndex(source_node_id);
    bool is_segregated = facade.IsSegregated(source_node_id);
    const auto source_mode = facade.GetTravelMode(source_node_id);
    auto source_classes = facade.GetClasses(facade.GetClassData(source_node_id));

    const EdgeWeight target_duration =
        target_traversed_in_reverse ? target_node.reverse_duration : target_node.forward_duration;
    const EdgeWeight target_weight =
        target_traversed_in_reverse ? target_node.reverse_weight : target_node.forward_weight;
    const auto target_node_id = target_traversed_in_reverse ? target_node.reverse_segment_id.id
                                                            : target_node.forward_segment_id.id;
    const auto target_name_id = facade.GetNameIndex(target_node_id);
    const auto target_mode = facade.GetTravelMode(target_node_id);

    const auto number_of_segments = leg_geometry.GetNumberOfSegments();

    std::vector<RouteStep> steps;
    steps.reserve(number_of_segments);

    std::size_t segment_index = 0;
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);

    auto bearings =
        detail::getDepartBearings(leg_geometry, source_node, source_traversed_in_reverse);

    StepManeuver maneuver{source_node.location,
                          bearings.first,
                          bearings.second,
                          osrm::guidance::TurnInstruction::NO_TURN(),
                          WaypointType::Depart,
                          0};

    IntermediateIntersection intersection{source_node.location,
                                          std::vector<short>({bearings.second}),
                                          std::vector<bool>({true}),
                                          IntermediateIntersection::NO_INDEX,
                                          0,
                                          util::guidance::LaneTuple(),
                                          {},
                                          source_classes};

    if (leg_data.size() > 0)
    {
        // PathData saves the information we need of the segment _before_ the turn,
        // but a RouteStep is with regard to the segment after the turn.
        // We need to skip the first segment because it is already covered by the
        // initial start of a route
        EdgeWeight segment_duration = 0;
        EdgeWeight segment_weight = 0;

        // some name changes are not announced in our processing. For these, we have to keep the
        // first name on the segment
        auto step_name_id = source_name_id;
        for (std::size_t leg_data_index = 0; leg_data_index < leg_data.size(); ++leg_data_index)
        {
            const auto &path_point = leg_data[leg_data_index];
            segment_duration += path_point.duration_until_turn;
            segment_weight += path_point.weight_until_turn;

            // all changes to this check have to be matched with assemble_geometry
            if (path_point.turn_instruction.type != osrm::guidance::TurnType::NoTurn)
            {
                BOOST_ASSERT(segment_weight >= 0);
                const auto name = facade.GetNameForID(step_name_id);
                const auto ref = facade.GetRefForID(step_name_id);
                const auto pronunciation = facade.GetPronunciationForID(step_name_id);
                const auto destinations = facade.GetDestinationsForID(step_name_id);
                const auto exits = facade.GetExitsForID(step_name_id);
                const auto distance = leg_geometry.segment_distances[segment_index];
                // intersections contain the classes of exiting road
                intersection.classes = facade.GetClasses(path_point.classes);

                steps.push_back(RouteStep{path_point.from_edge_based_node,
                                          step_name_id,
                                          is_segregated,
                                          name.to_string(),
                                          ref.to_string(),
                                          pronunciation.to_string(),
                                          destinations.to_string(),
                                          exits.to_string(),
                                          NO_ROTARY_NAME,
                                          NO_ROTARY_NAME,
                                          segment_duration / 10.,
                                          distance,
                                          segment_weight / weight_multiplier,
                                          path_point.travel_mode,
                                          maneuver,
                                          leg_geometry.FrontIndex(segment_index),
                                          leg_geometry.BackIndex(segment_index) + 1,
                                          {intersection},
                                          path_point.is_left_hand_driving});

                if (leg_data_index + 1 < leg_data.size())
                {
                    step_name_id = leg_data[leg_data_index + 1].name_id;
                    is_segregated = leg_data[leg_data_index + 1].is_segregated;
                }
                else
                {
                    step_name_id = facade.GetNameIndex(target_node_id);
                    is_segregated = facade.IsSegregated(target_node_id);
                }

                // extract bearings
                bearings = std::make_pair<std::uint16_t, std::uint16_t>(
                    path_point.pre_turn_bearing.Get(), path_point.post_turn_bearing.Get());
                const auto bearing_class = facade.GetBearingClass(path_point.turn_via_node);
                auto bearing_data = bearing_class.getAvailableBearings();
                intersection.in = bearing_class.findMatchingBearing(bearings.first);
                intersection.out = bearing_class.findMatchingBearing(bearings.second);
                intersection.location = facade.GetCoordinateOfNode(path_point.turn_via_node);
                intersection.bearings.clear();
                intersection.bearings.reserve(bearing_data.size());
                intersection.lanes = path_point.lane_data.first;
                intersection.lane_description =
                    path_point.lane_data.second != INVALID_LANE_DESCRIPTIONID
                        ? facade.GetTurnDescription(path_point.lane_data.second)
                        : extractor::TurnLaneDescription();

                // Lanes in turn are bound by total number of lanes at the location
                BOOST_ASSERT(intersection.lanes.lanes_in_turn <=
                             intersection.lane_description.size());
                // No lanes at location and no turn lane or lanes at location and lanes in turn
                BOOST_ASSERT((intersection.lane_description.empty() &&
                              intersection.lanes.lanes_in_turn == 0) ||
                             (!intersection.lane_description.empty() &&
                              intersection.lanes.lanes_in_turn != 0));

                std::copy(bearing_data.begin(),
                          bearing_data.end(),
                          std::back_inserter(intersection.bearings));
                intersection.entry.clear();
                for (auto idx : util::irange<std::size_t>(0, intersection.bearings.size()))
                {
                    intersection.entry.push_back(path_point.entry_class.allowsEntry(idx));
                }
                std::int16_t bearing_in_driving_direction =
                    util::bearing::reverse(std::round(bearings.first));
                maneuver = {intersection.location,
                            bearing_in_driving_direction,
                            bearings.second,
                            path_point.turn_instruction,
                            WaypointType::None,
                            0};
                segment_index++;
                segment_duration = 0;
                segment_weight = 0;
            }
        }
        const auto distance = leg_geometry.segment_distances[segment_index];
        const EdgeWeight duration = segment_duration + target_duration;
        const EdgeWeight weight = segment_weight + target_weight;
        // intersections contain the classes of exiting road
        intersection.classes = facade.GetClasses(facade.GetClassData(target_node_id));
        BOOST_ASSERT(duration >= 0);
        steps.push_back(RouteStep{leg_data[leg_data.size() - 1].from_edge_based_node,
                                  step_name_id,
                                  is_segregated,
                                  facade.GetNameForID(step_name_id).to_string(),
                                  facade.GetRefForID(step_name_id).to_string(),
                                  facade.GetPronunciationForID(step_name_id).to_string(),
                                  facade.GetDestinationsForID(step_name_id).to_string(),
                                  facade.GetExitsForID(step_name_id).to_string(),
                                  NO_ROTARY_NAME,
                                  NO_ROTARY_NAME,
                                  duration / 10.,
                                  distance,
                                  weight / weight_multiplier,
                                  target_mode,
                                  maneuver,
                                  leg_geometry.FrontIndex(segment_index),
                                  leg_geometry.BackIndex(segment_index) + 1,
                                  {intersection},
                                  facade.IsLeftHandDriving(target_node_id)});
    }
    // In this case the source + target are on the same edge segment
    else
    {
        BOOST_ASSERT(source_node.fwd_segment_position == target_node.fwd_segment_position);
        BOOST_ASSERT(source_traversed_in_reverse == target_traversed_in_reverse);

        // The difference (target-source) should handle
        // all variants for similar directions u-v and s-t (and opposite)
        //    s(t)  t(s)   source_traversed_in_reverse = target_traversed_in_reverse = false
        // u-------------v
        // |---|           source_weight
        // |---------|     target_weight

        //    s(t)  t(s)   source_traversed_in_reverse = target_traversed_in_reverse = true
        // u-------------v
        // |   |---------| source_weight
        // |         |---| target_weight
        BOOST_ASSERT(target_weight >= source_weight);
        const EdgeWeight weight = target_weight - source_weight;

        // use rectified linear unit function to avoid negative duration values
        // due to flooring errors in phantom snapping
        BOOST_ASSERT(target_duration >= source_duration || weight == 0);
        const EdgeWeight duration = std::max(0, target_duration - source_duration);

        steps.push_back(RouteStep{source_node_id,
                                  source_name_id,
                                  is_segregated,
                                  facade.GetNameForID(source_name_id).to_string(),
                                  facade.GetRefForID(source_name_id).to_string(),
                                  facade.GetPronunciationForID(source_name_id).to_string(),
                                  facade.GetDestinationsForID(source_name_id).to_string(),
                                  facade.GetExitsForID(source_name_id).to_string(),
                                  NO_ROTARY_NAME,
                                  NO_ROTARY_NAME,
                                  duration / 10.,
                                  leg_geometry.segment_distances[segment_index],
                                  weight / weight_multiplier,
                                  source_mode,
                                  std::move(maneuver),
                                  leg_geometry.FrontIndex(segment_index),
                                  leg_geometry.BackIndex(segment_index) + 1,
                                  {intersection},
                                  facade.IsLeftHandDriving(source_node_id)});
    }

    BOOST_ASSERT(segment_index == number_of_segments - 1);
    bearings = detail::getArriveBearings(leg_geometry, target_node, target_traversed_in_reverse);

    intersection = {
        target_node.location,
        std::vector<short>({static_cast<short>(util::bearing::reverse(bearings.first))}),
        std::vector<bool>({true}),
        0,
        IntermediateIntersection::NO_INDEX,
        util::guidance::LaneTuple(),
        {},
        {}};

    // This step has length zero, the only reason we need it is the target location
    maneuver = {intersection.location,
                bearings.first,
                bearings.second,
                osrm::guidance::TurnInstruction::NO_TURN(),
                WaypointType::Arrive,
                0};

    BOOST_ASSERT(!leg_geometry.locations.empty());
    steps.push_back(RouteStep{target_node_id,
                              target_name_id,
                              facade.IsSegregated(target_node_id),
                              facade.GetNameForID(target_name_id).to_string(),
                              facade.GetRefForID(target_name_id).to_string(),
                              facade.GetPronunciationForID(target_name_id).to_string(),
                              facade.GetDestinationsForID(target_name_id).to_string(),
                              facade.GetExitsForID(target_name_id).to_string(),
                              NO_ROTARY_NAME,
                              NO_ROTARY_NAME,
                              ZERO_DURATION,
                              ZERO_DISTANCE,
                              ZERO_WEIGHT,
                              target_mode,
                              std::move(maneuver),
                              leg_geometry.locations.size() - 1,
                              leg_geometry.locations.size(),
                              {intersection},
                              facade.IsLeftHandDriving(target_node_id)});

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
    // depart and arrive need to be trivial
    BOOST_ASSERT(steps.front().maneuver.exit == 0 && steps.back().maneuver.exit == 0);
    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
