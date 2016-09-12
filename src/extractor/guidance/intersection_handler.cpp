#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>
#include <cstddef>

using EdgeData = osrm::util::NodeBasedDynamicGraph::EdgeData;
using osrm::util::guidance::getTurnDirection;

namespace osrm
{
namespace extractor
{
namespace guidance
{

namespace detail
{
inline bool requiresAnnouncement(const EdgeData &from, const EdgeData &to)
{
    return !from.CanCombineWith(to);
}
}

IntersectionHandler::IntersectionHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                         const std::vector<QueryNode> &node_info_list,
                                         const util::NameTable &name_table,
                                         const SuffixTable &street_name_suffix_table,
                                         const IntersectionGenerator &intersection_generator)
    : node_based_graph(node_based_graph), node_info_list(node_info_list), name_table(name_table),
      street_name_suffix_table(street_name_suffix_table),
      intersection_generator(intersection_generator)
{
}

std::size_t IntersectionHandler::countValid(const Intersection &intersection) const
{
    return std::count_if(intersection.begin(), intersection.end(), [](const ConnectedRoad &road) {
        return road.entry_allowed;
    });
}

TurnType::Enum IntersectionHandler::findBasicTurnType(const EdgeID via_edge,
                                                      const ConnectedRoad &road) const
{

    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);

    bool on_ramp = in_data.road_classification.IsRampClass();

    bool onto_ramp = out_data.road_classification.IsRampClass();

    if (!on_ramp && onto_ramp)
        return TurnType::OnRamp;

    if (in_data.name_id == out_data.name_id && in_data.name_id != EMPTY_NAMEID)
    {
        return TurnType::Continue;
    }

    return TurnType::Turn;
}

TurnInstruction IntersectionHandler::getInstructionForObvious(const std::size_t num_roads,
                                                              const EdgeID via_edge,
                                                              const bool through_street,
                                                              const ConnectedRoad &road) const
{
    const auto type = findBasicTurnType(via_edge, road);
    // handle travel modes:
    const auto in_mode = node_based_graph.GetEdgeData(via_edge).travel_mode;
    const auto out_mode = node_based_graph.GetEdgeData(road.turn.eid).travel_mode;
    if (type == TurnType::OnRamp)
    {
        return {TurnType::OnRamp, getTurnDirection(road.turn.angle)};
    }

    if (angularDeviation(road.turn.angle, 0) < 0.01)
    {
        return {TurnType::Turn, DirectionModifier::UTurn};
    }
    if (type == TurnType::Turn)
    {
        const auto &in_data = node_based_graph.GetEdgeData(via_edge);
        const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
        if (in_data.name_id != out_data.name_id &&
            util::guidance::requiresNameAnnounced(name_table.GetNameForID(in_data.name_id),
                                                  name_table.GetRefForID(in_data.name_id),
                                                  name_table.GetNameForID(out_data.name_id),
                                                  name_table.GetRefForID(out_data.name_id),
                                                  street_name_suffix_table))
        {
            // obvious turn onto a through street is a merge
            if (through_street)
            {
                // We reserve merges for motorway types. All others are considered for simply going
                // straight onto a road. This avoids confusion about merge directions on streets
                // that could potentially also offer different choices
                if (out_data.road_classification.IsMotorwayClass())
                    return {TurnType::Merge,
                            road.turn.angle > STRAIGHT_ANGLE ? DirectionModifier::SlightRight
                                                             : DirectionModifier::SlightLeft};
                else if (in_data.road_classification.IsRampClass() &&
                         out_data.road_classification.IsRampClass())
                {
                    if (in_mode == out_mode)
                        return {TurnType::Suppressed, getTurnDirection(road.turn.angle)};
                    else
                        return {TurnType::Notification, getTurnDirection(road.turn.angle)};
                }
                else
                {
                    const double constexpr MAX_COLLAPSE_DISTANCE = 30;
                    // in normal road condidtions, we check if the turn is nearly straight.
                    // Doing so, we widen the angle that a turn is considered straight, but since it
                    // is obvious, the choice is arguably better.

                    // FIXME this requires https://github.com/Project-OSRM/osrm-backend/pull/2399,
                    // since `distance` does not refer to an actual distance but rather to the
                    // duration/weight of the traversal. We can only approximate the distance here
                    // or actually follow the full road. When 2399 lands, we can exchange here for a
                    // precalculated distance value.
                    const auto distance = util::coordinate_calculation::haversineDistance(
                        node_info_list[node_based_graph.GetTarget(via_edge)],
                        node_info_list[node_based_graph.GetTarget(road.turn.eid)]);
                    return {TurnType::Turn,
                            (angularDeviation(road.turn.angle, STRAIGHT_ANGLE) <
                                 FUZZY_ANGLE_DIFFERENCE ||
                             distance > 2 * MAX_COLLAPSE_DISTANCE)
                                ? DirectionModifier::Straight
                                : getTurnDirection(road.turn.angle)};
                }
            }
            else
            {
                return {TurnType::NewName, getTurnDirection(road.turn.angle)};
            }
        }
        else
        {
            if (in_mode == out_mode)
                return {TurnType::Suppressed, getTurnDirection(road.turn.angle)};
            else
                return {TurnType::Notification, getTurnDirection(road.turn.angle)};
        }
    }
    BOOST_ASSERT(type == TurnType::Continue);
    if (in_mode != out_mode)
    {
        return {TurnType::Notification, getTurnDirection(road.turn.angle)};
    }
    if (num_roads > 2)
    {
        return {TurnType::Suppressed, getTurnDirection(road.turn.angle)};
    }
    else
    {
        return {TurnType::NoTurn, getTurnDirection(road.turn.angle)};
    }
}

void IntersectionHandler::assignFork(const EdgeID via_edge,
                                     ConnectedRoad &left,
                                     ConnectedRoad &right) const
{
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const bool low_priority_left =
        node_based_graph.GetEdgeData(left.turn.eid).road_classification.IsLowPriorityRoadClass();
    const bool low_priority_right =
        node_based_graph.GetEdgeData(right.turn.eid).road_classification.IsLowPriorityRoadClass();
    if ((angularDeviation(left.turn.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
         angularDeviation(right.turn.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE))
    {
        // left side is actually straight
        const auto &out_data = node_based_graph.GetEdgeData(left.turn.eid);
        if (detail::requiresAnnouncement(in_data, out_data))
        {
            if (low_priority_right && !low_priority_left)
            {
                left.turn.instruction = getInstructionForObvious(3, via_edge, false, left);
                right.turn.instruction = {findBasicTurnType(via_edge, right),
                                          DirectionModifier::SlightRight};
            }
            else
            {
                if (low_priority_left && !low_priority_right)
                {
                    left.turn.instruction = {findBasicTurnType(via_edge, left),
                                             DirectionModifier::SlightLeft};
                    right.turn.instruction = {findBasicTurnType(via_edge, right),
                                              DirectionModifier::SlightRight};
                }
                else
                {
                    left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
                    right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
                }
            }
        }
        else
        {
            left.turn.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      DirectionModifier::SlightRight};
        }
    }
    else if (angularDeviation(right.turn.angle, STRAIGHT_ANGLE) <
                 MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
             angularDeviation(left.turn.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
    {
        // right side is actually straight
        const auto &out_data = node_based_graph.GetEdgeData(right.turn.eid);
        if (angularDeviation(right.turn.angle, STRAIGHT_ANGLE) <
                MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            angularDeviation(left.turn.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
        {
            if (detail::requiresAnnouncement(in_data, out_data))
            {
                if (low_priority_left && !low_priority_right)
                {
                    left.turn.instruction = {findBasicTurnType(via_edge, left),
                                             DirectionModifier::SlightLeft};
                    right.turn.instruction = getInstructionForObvious(3, via_edge, false, right);
                }
                else
                {
                    if (low_priority_right && !low_priority_left)
                    {
                        left.turn.instruction = {findBasicTurnType(via_edge, left),
                                                 DirectionModifier::SlightLeft};
                        right.turn.instruction = {findBasicTurnType(via_edge, right),
                                                  DirectionModifier::SlightRight};
                    }
                    else
                    {
                        right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
                        left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
                    }
                }
            }
            else
            {
                right.turn.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
                left.turn.instruction = {findBasicTurnType(via_edge, left),
                                         DirectionModifier::SlightLeft};
            }
        }
    }
    // left side of fork
    if (low_priority_right && !low_priority_left)
        left.turn.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
    else
    {
        if (low_priority_left && !low_priority_right)
            left.turn.instruction = {TurnType::Turn, DirectionModifier::SlightLeft};
        else
            left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
    }

    // right side of fork
    if (low_priority_left && !low_priority_right)
        right.turn.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
    else
    {
        if (low_priority_right && !low_priority_left)
            right.turn.instruction = {TurnType::Turn, DirectionModifier::SlightRight};
        else
            right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
    }
}

void IntersectionHandler::assignFork(const EdgeID via_edge,
                                     ConnectedRoad &left,
                                     ConnectedRoad &center,
                                     ConnectedRoad &right) const
{
    // TODO handle low priority road classes in a reasonable way
    if (left.entry_allowed && center.entry_allowed && right.entry_allowed)
    {
        left.turn.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        if (angularDeviation(center.turn.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        {
            const auto &in_data = node_based_graph.GetEdgeData(via_edge);
            const auto &out_data = node_based_graph.GetEdgeData(center.turn.eid);
            if (detail::requiresAnnouncement(in_data, out_data))
            {
                center.turn.instruction = {TurnType::Fork, DirectionModifier::Straight};
            }
            else
            {
                center.turn.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            }
        }
        else
        {
            center.turn.instruction = {TurnType::Fork, DirectionModifier::Straight};
        }
        right.turn.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
    }
    else if (left.entry_allowed)
    {
        if (right.entry_allowed)
            assignFork(via_edge, left, right);
        else if (center.entry_allowed)
            assignFork(via_edge, left, center);
        else
            left.turn.instruction = {findBasicTurnType(via_edge, left),
                                     getTurnDirection(left.turn.angle)};
    }
    else if (right.entry_allowed)
    {
        if (center.entry_allowed)
            assignFork(via_edge, center, right);
        else
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      getTurnDirection(right.turn.angle)};
    }
    else
    {
        if (center.entry_allowed)
            center.turn.instruction = {findBasicTurnType(via_edge, center),
                                       getTurnDirection(center.turn.angle)};
    }
}

void IntersectionHandler::assignTrivialTurns(const EdgeID via_eid,
                                             Intersection &intersection,
                                             const std::size_t begin,
                                             const std::size_t end) const
{
    for (std::size_t index = begin; index != end; ++index)
        if (intersection[index].entry_allowed)
            intersection[index].turn.instruction = {
                findBasicTurnType(via_eid, intersection[index]),
                getTurnDirection(intersection[index].turn.angle)};
}

bool IntersectionHandler::isThroughStreet(const std::size_t index,
                                          const Intersection &intersection) const
{
    if (node_based_graph.GetEdgeData(intersection[index].turn.eid).name_id == EMPTY_NAMEID)
        return false;

    const auto &data_at_index = node_based_graph.GetEdgeData(intersection[index].turn.eid);

    // a through street cannot start at our own position -> index 1
    for (std::size_t road_index = 1; road_index < intersection.size(); ++road_index)
    {
        if (road_index == index)
            continue;

        const auto &road = intersection[road_index];
        const auto &road_data = node_based_graph.GetEdgeData(road.turn.eid);

        // roads have a near straight angle (180 degree)
        const bool is_nearly_straight =
            angularDeviation(road.turn.angle, intersection[index].turn.angle) >
            (STRAIGHT_ANGLE - FUZZY_ANGLE_DIFFERENCE);

        const bool have_same_name = data_at_index.name_id == road_data.name_id;
        const bool have_same_category =
            data_at_index.road_classification == road_data.road_classification;

        if (is_nearly_straight && have_same_name && have_same_category)
            return true;
    }
    return false;
}

std::size_t IntersectionHandler::findObviousTurn(const EdgeID via_edge,
                                                 const Intersection &intersection) const
{
    // no obvious road
    if (intersection.size() == 1)
        return 0;

    // a single non u-turn is obvious
    if (intersection.size() == 2)
        return 1;

    // at least three roads
    std::size_t best = 0;
    double best_deviation = 180;

    std::size_t best_continue = 0;
    double best_continue_deviation = 180;

    const EdgeData &in_data = node_based_graph.GetEdgeData(via_edge);
    const auto in_classification = in_data.road_classification;

    const auto obvious_by_road_class = [](const RoadClassification in_classification,
                                          const RoadClassification obvious_candidate,
                                          const RoadClassification compare_candidate) {
        const bool has_high_priority =
            PRIORITY_DISTINCTION_FACTOR * obvious_candidate.GetPriority() <
            compare_candidate.GetPriority();
        const bool continues_on_same_class = in_classification == obvious_candidate;
        return (has_high_priority && continues_on_same_class) ||
               (!obvious_candidate.IsLowPriorityRoadClass() &&
                compare_candidate.IsLowPriorityRoadClass());
    };

    for (std::size_t i = 1; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (!intersection[i].entry_allowed)
            continue;

        const auto out_data = node_based_graph.GetEdgeData(intersection[i].turn.eid);
        const auto continue_class =
            node_based_graph.GetEdgeData(intersection[best_continue].turn.eid).road_classification;

        if (out_data.name_id == in_data.name_id &&
            (best_continue == 0 ||
             (continue_class.GetPriority() > out_data.road_classification.GetPriority() &&
              in_classification != continue_class) ||
             (deviation < best_continue_deviation &&
              out_data.road_classification == continue_class) ||
             (continue_class != in_classification &&
              out_data.road_classification == continue_class)))
        {
            best_continue_deviation = deviation;
            best_continue = i;
        }

        const auto current_best_class =
            node_based_graph.GetEdgeData(intersection[best_continue].turn.eid).road_classification;

        // don't prefer low priority classes
        if (out_data.road_classification.IsLowPriorityRoadClass() &&
            !current_best_class.IsLowPriorityRoadClass())
            continue;

        const bool is_better_choice_by_priority = obvious_by_road_class(
            in_data.road_classification, out_data.road_classification, current_best_class);

        const bool other_is_better_choice_by_priority = obvious_by_road_class(
            in_data.road_classification, current_best_class, out_data.road_classification);

        if ((!other_is_better_choice_by_priority && deviation < best_deviation) ||
            is_better_choice_by_priority)
        {
            best_deviation = deviation;
            best = i;
        }
    }

    // We don't consider empty names a valid continue feature. This distinguishes between missing
    // names and actual continuing roads.
    if (in_data.name_id == EMPTY_NAMEID)
        best_continue = 0;

    if (best == 0)
        return 0;

    const std::pair<std::int64_t, std::int64_t> num_continue_names = [&]() {
        std::int64_t count = 0, count_valid = 0;
        if (in_data.name_id != EMPTY_NAMEID)
        {
            for (std::size_t i = 1; i < intersection.size(); ++i)
            {
                const auto &road = intersection[i];
                if ((in_data.name_id == node_based_graph.GetEdgeData(road.turn.eid).name_id))
                {
                    ++count;
                    if (road.entry_allowed)
                        ++count_valid;
                }
            }
        }
        return std::make_pair(count, count_valid);
    }();

    if (0 != best_continue && best != best_continue &&
        angularDeviation(intersection[best].turn.angle, STRAIGHT_ANGLE) <
            MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
        node_based_graph.GetEdgeData(intersection[best_continue].turn.eid).road_classification ==
            node_based_graph.GetEdgeData(intersection[best].turn.eid).road_classification)
    {
        // if the best angle is going straight but the road is turning, we don't name anything
        // obvious
        return 0;
    }

    const bool all_continues_are_narrow = [&]() {
        if (in_data.name_id == EMPTY_NAMEID)
            return false;

        return std::count_if(
                   intersection.begin() + 1, intersection.end(), [&](const ConnectedRoad &road) {
                       return (in_data.name_id ==
                               node_based_graph.GetEdgeData(road.turn.eid).name_id) &&
                              angularDeviation(road.turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE;
                   }) == num_continue_names.first;
    }();

    // has no obvious continued road
    const auto &best_data = node_based_graph.GetEdgeData(intersection[best].turn.eid);
    if (best_continue == 0 || (!all_continues_are_narrow &&
                               (num_continue_names.first >= 2 && intersection.size() >= 4)) ||
        (num_continue_names.second >= 2 && best_continue_deviation >= 2 * NARROW_TURN_ANGLE) ||
        (best_deviation != best_continue_deviation && best_deviation < FUZZY_ANGLE_DIFFERENCE &&
         !best_data.road_classification.IsRampClass()))
    {
        // Find left/right deviation
        const double left_deviation = angularDeviation(
            intersection[(best + 1) % intersection.size()].turn.angle, STRAIGHT_ANGLE);
        const double right_deviation =
            angularDeviation(intersection[best - 1].turn.angle, STRAIGHT_ANGLE);

        if (best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            std::min(left_deviation, right_deviation) > FUZZY_ANGLE_DIFFERENCE)
            return best;

        const auto left_index = (best + 1) % intersection.size();
        const auto right_index = best - 1;
        const auto &left_data = node_based_graph.GetEdgeData(intersection[left_index].turn.eid);
        const auto &right_data = node_based_graph.GetEdgeData(intersection[right_index].turn.eid);

        const bool obvious_to_left =
            left_index == 0 || obvious_by_road_class(in_data.road_classification,
                                                     best_data.road_classification,
                                                     left_data.road_classification);
        const bool obvious_to_right =
            right_index == 0 || obvious_by_road_class(in_data.road_classification,
                                                      best_data.road_classification,
                                                      right_data.road_classification);

        // other narrow turns?
        if (angularDeviation(intersection[right_index].turn.angle, STRAIGHT_ANGLE) <=
                FUZZY_ANGLE_DIFFERENCE &&
            !obvious_to_right)
            return 0;

        if (angularDeviation(intersection[left_index].turn.angle, STRAIGHT_ANGLE) <=
                FUZZY_ANGLE_DIFFERENCE &&
            !obvious_to_left)
            return 0;

        const bool distinct_to_left =
            left_deviation / best_deviation >= DISTINCTION_RATIO ||
            (left_deviation > best_deviation &&
             (!intersection[left_index].entry_allowed && in_data.distance > 30));
        const bool distinct_to_right =
            right_deviation / best_deviation >= DISTINCTION_RATIO ||
            (right_deviation > best_deviation &&
             (!intersection[right_index].entry_allowed && in_data.distance > 30));

        // Well distinct turn that is nearly straight
        if ((distinct_to_left || obvious_to_left) && (distinct_to_right || obvious_to_right))
            return best;
    }
    else
    {
        const double deviation =
            angularDeviation(intersection[best_continue].turn.angle, STRAIGHT_ANGLE);
        const auto &continue_data =
            node_based_graph.GetEdgeData(intersection[best_continue].turn.eid);
        if (std::abs(deviation) < 1)
            return best_continue;

        // check if any other similar best continues exist
        for (std::size_t i = 1; i < intersection.size(); ++i)
        {
            if (i == best_continue || !intersection[i].entry_allowed)
                continue;

            const auto &turn_data = node_based_graph.GetEdgeData(intersection[i].turn.eid);
            const bool is_obvious_by_road_class =
                obvious_by_road_class(in_data.road_classification,
                                      continue_data.road_classification,
                                      turn_data.road_classification);

            // if the main road is obvious by class, we ignore the current road as a potential
            // prevention of obviousness
            if (is_obvious_by_road_class)
                continue;

            // continuation could be grouped with a straight turn and the turning road is a ramp
            if (turn_data.road_classification.IsRampClass() && deviation < GROUP_ANGLE)
                continue;

            // perfectly straight turns prevent obviousness
            const auto turn_deviation =
                angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
            if (turn_deviation < FUZZY_ANGLE_DIFFERENCE)
                return 0;

            const auto deviation_ratio = turn_deviation / deviation;

            // in comparison to normal devitions, a continue road can offer a smaller distinction
            // ratio. Other roads close to the turn angle are not as obvious, if one road continues.
            if (deviation_ratio < DISTINCTION_RATIO / 1.5)
                return 0;

            /* in comparison to another continuing road, we need a better distinction. This prevents
               situations where the turn is probably less obvious. An example are places that have a
               road with the same name entering/exiting:

                       d
                      /
                     /
               a -- b
                     \
                      \
                       c
            */

            if (turn_data.name_id == continue_data.name_id &&
                deviation_ratio < 1.5 * DISTINCTION_RATIO)
                return 0;
        }

        // Segregated intersections can result in us finding an obvious turn, even though its only
        // obvious due to a very short segment in between. So if the segment coming in is very
        // short, we check the previous intersection for other continues in the opposite bearing.
        const auto node_at_intersection = node_based_graph.GetTarget(via_edge);
        const util::Coordinate coordinate_at_intersection = node_info_list[node_at_intersection];

        const auto node_at_u_turn = node_based_graph.GetTarget(intersection[0].turn.eid);
        const util::Coordinate coordinate_at_u_turn = node_info_list[node_at_u_turn];

        const double constexpr MAX_COLLAPSE_DISTANCE = 30;
        if (util::coordinate_calculation::haversineDistance(
                coordinate_at_intersection, coordinate_at_u_turn) < MAX_COLLAPSE_DISTANCE)
        {
            // this request here actually goes against the direction of the ingoing edgeid. This can
            // even reverse the direction. Since we don't want to compute actual turns but simply
            // try to find whether there is a turn going to the opposite direction of our obvious
            // turn, this should be alright.
            const auto previous_intersection = intersection_generator.GetActualNextIntersection(
                node_at_intersection, intersection[0].turn.eid, nullptr, nullptr);

            const auto continue_road = intersection[best_continue];
            for (const auto &comparison_road : previous_intersection)
            {
                // since we look at the intersection in the wrong direction, a similar angle
                // actually represents a near 180 degree different in bearings between the two
                // roads. So if there is a road that is enterable in the opposite direction just
                // prior, a turn is not obvious
                const auto &turn_data = node_based_graph.GetEdgeData(comparison_road.turn.eid);
                if (angularDeviation(comparison_road.turn.angle, STRAIGHT_ANGLE) > GROUP_ANGLE &&
                    angularDeviation(comparison_road.turn.angle, continue_road.turn.angle) <
                        FUZZY_ANGLE_DIFFERENCE &&
                    !turn_data.reversed && continue_data.CanCombineWith(turn_data))
                    return 0;
            }
        }

        return best_continue;
    }

    return 0;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
