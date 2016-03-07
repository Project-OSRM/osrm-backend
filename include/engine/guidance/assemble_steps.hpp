#ifndef ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_
#define ENGINE_GUIDANCE_ASSEMBLE_STEPS_HPP_

#include "engine/guidance/route_step.hpp"
#include "engine/guidance/step_maneuver.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/toolkit.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/coordinate.hpp"
#include "util/bearing.hpp"
#include "extractor/travel_mode.hpp"

#include <vector>
#include <boost/optional.hpp>

namespace osrm
{
namespace engine
{
namespace guidance
{
namespace detail
{
StepManeuver stepManeuverFromGeometry(extractor::guidance::TurnInstruction instruction,
                                      const LegGeometry &leg_geometry,
                                      const std::size_t segment_index,
                                      const unsigned exit);
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
    const auto source_duration =
        (source_traversed_in_reverse ? source_node.GetReverseWeightPlusOffset()
                                     : source_node.GetForwardWeightPlusOffset()) /
        10.;
    const auto source_mode = source_traversed_in_reverse ? source_node.backward_travel_mode
                                                         : source_node.forward_travel_mode;

    const auto target_duration =
        (target_traversed_in_reverse ? target_node.GetReverseWeightPlusOffset()
                                     : target_node.GetForwardWeightPlusOffset()) /
        10.;
    const auto target_mode = target_traversed_in_reverse ? target_node.backward_travel_mode
                                                         : target_node.forward_travel_mode;

    const auto number_of_segments = leg_geometry.GetNumberOfSegments();

    std::vector<RouteStep> steps;
    steps.reserve(number_of_segments);

    std::size_t segment_index = 0;
    const auto initial_modifier =
        leg_geometry.locations.size() >= 3
            ? angleToDirectionModifier(util::coordinate_calculation::computeAngle(
                  leg_geometry.locations[0],
                  leg_geometry.locations[1],
                  leg_geometry.locations[2]))
            : extractor::guidance::DirectionModifier::UTurn;

    if (leg_data.size() > 0)
    {

        StepManeuver maneuver = detail::stepManeuverFromGeometry(
            extractor::guidance::TurnInstruction{extractor::guidance::TurnType::Location,
                                                 initial_modifier},
            leg_geometry, segment_index, INVALID_EXIT_NR);

        // PathData saves the information we need of the segment _before_ the turn,
        // but a RouteStep is with regard to the segment after the turn.
        // We need to skip the first segment because it is already covered by the
        // initial start of a route
        for (const auto &path_point : leg_data)
        {
            if (path_point.turn_instruction != extractor::guidance::TurnInstruction::NO_TURN())
            {
                const auto name = facade.get_name_for_id(path_point.name_id);
                const auto distance = leg_geometry.segment_distances[segment_index];
                steps.push_back(RouteStep{
                    path_point.name_id, name, path_point.duration_until_turn / 10.0, distance,
                    path_point.travel_mode, maneuver, leg_geometry.FrontIndex(segment_index),
                    leg_geometry.BackIndex(segment_index) + 1});
                maneuver = detail::stepManeuverFromGeometry(
                    path_point.turn_instruction, leg_geometry, segment_index, path_point.exit);
                segment_index++;
            }
        }
        const auto distance = leg_geometry.segment_distances[segment_index];
        steps.push_back(RouteStep{target_node.name_id, facade.get_name_for_id(target_node.name_id),
                                  target_duration, distance, target_mode, maneuver,
                                  leg_geometry.FrontIndex(segment_index),
                                  leg_geometry.BackIndex(segment_index) + 1});
    }
    else
    {
        //
        // |-----s source_duration
        // |-------------t target_duration
        // x---*---*---*---z compressed edge
        //       |-------| duration
        StepManeuver maneuver = {source_node.location, 0., 0.,
                                 extractor::guidance::TurnInstruction{
                                     extractor::guidance::TurnType::Location, initial_modifier},
                                 INVALID_EXIT_NR};

        steps.push_back(RouteStep{source_node.name_id, facade.get_name_for_id(source_node.name_id),
                                  target_duration - source_duration,
                                  leg_geometry.segment_distances[segment_index], source_mode,
                                  std::move(maneuver), leg_geometry.FrontIndex(segment_index),
                                  leg_geometry.BackIndex(segment_index) + 1});
    }

    BOOST_ASSERT(segment_index == number_of_segments - 1);
    const auto final_modifier =
        leg_geometry.locations.size() >= 3
            ? angleToDirectionModifier(util::coordinate_calculation::computeAngle(
                  leg_geometry.locations[leg_geometry.locations.size() - 3],
                  leg_geometry.locations[leg_geometry.locations.size() - 2],
                  leg_geometry.locations[leg_geometry.locations.size() - 1]))
            : extractor::guidance::DirectionModifier::UTurn;
    // This step has length zero, the only reason we need it is the target location
    steps.push_back(RouteStep{
        target_node.name_id, facade.get_name_for_id(target_node.name_id), 0., 0., target_mode,
        StepManeuver{target_node.location, 0., 0.,
                     extractor::guidance::TurnInstruction{extractor::guidance::TurnType::Location,
                                                          final_modifier},
                     INVALID_EXIT_NR},
        leg_geometry.locations.size(), leg_geometry.locations.size()});

    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_SEGMENT_LIST_HPP_
