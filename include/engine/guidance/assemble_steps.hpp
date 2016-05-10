#ifndef ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_
#define ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_

#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/guidance/step_maneuver.hpp"
#include "engine/guidance/toolkit.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/guidance/entry_class.hpp"
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

Intersection intersectionFromGeometry(const WaypointType waypoint_type,
                                      const double segment_duration,
                                      const LegGeometry &leg_geometry,
                                      const std::size_t segment_index);

} // ns detail

template <typename DataFacadeT>
std::vector<RouteStep> assembleSteps(const DataFacadeT &facade,
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

    if (leg_data.size() > 0)
    {
        StepManeuver maneuver = {extractor::guidance::TurnInstruction::NO_TURN(),
                                 WaypointType::Depart, 0};

        auto intersection =
            detail::intersectionFromGeometry(WaypointType::Depart, 0, leg_geometry, 0);
        intersection = util::guidance::setIntersectionClasses(std::move(intersection), source_node);
        // maneuver.location = source_node.location;

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
                const auto distance = leg_geometry.segment_distances[segment_index];
                std::vector<Intersection> intersections(1, intersection);
                intersection = detail::intersectionFromGeometry(
                    WaypointType::None, segment_duration / 10., leg_geometry, segment_index);
                intersection.entry_class = facade.GetEntryClass(path_point.entry_classid);
                intersection.bearing_class =
                    facade.GetBearingClass(facade.GetBearingClassID(path_point.turn_via_node));
                steps.push_back(RouteStep{
                    step_name_id, name, NO_ROTARY_NAME, segment_duration / 10.0, distance,
                    path_point.travel_mode, maneuver, leg_geometry.FrontIndex(segment_index),
                    leg_geometry.BackIndex(segment_index) + 1, std::move(intersections)});
                if (leg_data_index + 1 < leg_data.size())
                {
                    step_name_id = leg_data[leg_data_index + 1].name_id;
                }
                else
                {
                    step_name_id = target_node.name_id;
                }
                maneuver = {path_point.turn_instruction, WaypointType::None, 0};
                segment_index++;
                segment_duration = 0;
            }
        }
        const auto distance = leg_geometry.segment_distances[segment_index];
        const int duration = segment_duration + target_duration;
        BOOST_ASSERT(duration >= 0);
        steps.push_back(RouteStep{
            step_name_id, facade.GetNameForID(step_name_id), NO_ROTARY_NAME, duration / 10.,
            distance, target_mode, maneuver, leg_geometry.FrontIndex(segment_index),
            leg_geometry.BackIndex(segment_index) + 1, std::vector<Intersection>(1, intersection)});
    }
    // In this case the source + target are on the same edge segment
    else
    {
        BOOST_ASSERT(source_node.fwd_segment_position == target_node.fwd_segment_position);
        //     s     t
        // u-------------v
        // |---| source_duration
        // |---------| target_duration

        StepManeuver maneuver = {extractor::guidance::TurnInstruction::NO_TURN(),
                                 WaypointType::Depart, 0};
        int duration = target_duration - source_duration;
        BOOST_ASSERT(duration >= 0);

        auto intersection = detail::intersectionFromGeometry(WaypointType::Depart, duration / 10.,
                                                             leg_geometry, segment_index);
        intersection = util::guidance::setIntersectionClasses(std::move(intersection), source_node);

        steps.push_back(RouteStep{
            source_node.name_id, facade.GetNameForID(source_node.name_id), NO_ROTARY_NAME,
            duration / 10., leg_geometry.segment_distances[segment_index], source_mode,
            std::move(maneuver), leg_geometry.FrontIndex(segment_index),
            leg_geometry.BackIndex(segment_index) + 1, std::vector<Intersection>(1, intersection)});
    }

    BOOST_ASSERT(segment_index == number_of_segments - 1);
    // This step has length zero, the only reason we need it is the target location
    StepManeuver final_maneuver = {extractor::guidance::TurnInstruction::NO_TURN(),
                                   WaypointType::Arrive, 0};

    auto intersection =
        detail::intersectionFromGeometry(WaypointType::Arrive, 0, leg_geometry, segment_index);
    intersection = util::guidance::setIntersectionClasses(std::move(intersection), target_node);

    BOOST_ASSERT(!leg_geometry.locations.empty());
    steps.push_back(RouteStep{target_node.name_id, facade.GetNameForID(target_node.name_id),
                              NO_ROTARY_NAME, ZERO_DURATION, ZERO_DISTANCE, target_mode,
                              std::move(final_maneuver), leg_geometry.locations.size() - 1,
                              leg_geometry.locations.size(),
                              std::vector<Intersection>(1, intersection)});

    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
