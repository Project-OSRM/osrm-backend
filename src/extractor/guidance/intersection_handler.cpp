#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/coordinate_calculation.hpp"
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
    const auto &out_data = node_based_graph.GetEdgeData(road.eid);

    bool on_ramp = in_data.road_classification.IsRampClass();

    bool onto_ramp = out_data.road_classification.IsRampClass();

    if (!on_ramp && onto_ramp)
        return TurnType::OnRamp;

    const auto same_name = !util::guidance::requiresNameAnnounced(
        in_data.name_id, out_data.name_id, name_table, street_name_suffix_table);

    if (in_data.name_id != EMPTY_NAMEID && out_data.name_id != EMPTY_NAMEID && same_name)
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
    const auto out_mode = node_based_graph.GetEdgeData(road.eid).travel_mode;
    if (type == TurnType::OnRamp)
    {
        return {TurnType::OnRamp, getTurnDirection(road.angle)};
    }

    if (angularDeviation(road.angle, 0) < 0.01)
    {
        return {TurnType::Turn, DirectionModifier::UTurn};
    }
    if (type == TurnType::Turn)
    {
        const auto &in_data = node_based_graph.GetEdgeData(via_edge);
        const auto &out_data = node_based_graph.GetEdgeData(road.eid);

        if (util::guidance::requiresNameAnnounced(
                in_data.name_id, out_data.name_id, name_table, street_name_suffix_table))
        {
            // obvious turn onto a through street is a merge
            if (through_street)
            {
                // We reserve merges for motorway types. All others are considered for simply going
                // straight onto a road. This avoids confusion about merge directions on streets
                // that could potentially also offer different choices
                if (out_data.road_classification.IsMotorwayClass())
                    return {TurnType::Merge,
                            road.angle > STRAIGHT_ANGLE ? DirectionModifier::SlightRight
                                                        : DirectionModifier::SlightLeft};
                else if (in_data.road_classification.IsRampClass() &&
                         out_data.road_classification.IsRampClass())
                {
                    // This check is more a precaution than anything else. Our current travel modes
                    // cannot reach this, since all ramps are exposing the same travel type. But we
                    // could see toll-type at some point.
                    return {in_mode == out_mode ? TurnType::Suppressed : TurnType::Notification,
                            getTurnDirection(road.angle)};
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
                        node_info_list[node_based_graph.GetTarget(road.eid)]);
                    return {
                        TurnType::Turn,
                        (angularDeviation(road.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE ||
                         distance > 2 * MAX_COLLAPSE_DISTANCE)
                            ? DirectionModifier::Straight
                            : getTurnDirection(road.angle)};
                }
            }
            else
            {
                return {in_mode == out_mode ? TurnType::NewName : TurnType::Notification,
                        getTurnDirection(road.angle)};
            }
        }
        // name has not changed, suppress a turn here or indicate mode change
        else
        {
            return {in_mode == out_mode ? TurnType::Suppressed : TurnType::Notification,
                    getTurnDirection(road.angle)};
        }
    }
    BOOST_ASSERT(type == TurnType::Continue);
    if (in_mode != out_mode)
    {
        return {TurnType::Notification, getTurnDirection(road.angle)};
    }
    if (num_roads > 2)
    {
        return {TurnType::Suppressed, getTurnDirection(road.angle)};
    }
    else
    {
        return {TurnType::NoTurn, getTurnDirection(road.angle)};
    }
}

void IntersectionHandler::assignFork(const EdgeID via_edge,
                                     ConnectedRoad &left,
                                     ConnectedRoad &right) const
{
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const bool low_priority_left =
        node_based_graph.GetEdgeData(left.eid).road_classification.IsLowPriorityRoadClass();
    const bool low_priority_right =
        node_based_graph.GetEdgeData(right.eid).road_classification.IsLowPriorityRoadClass();
    if ((angularDeviation(left.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
         angularDeviation(right.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE))
    {
        // left side is actually straight
        const auto &out_data = node_based_graph.GetEdgeData(left.eid);
        if (detail::requiresAnnouncement(in_data, out_data))
        {
            if (low_priority_right && !low_priority_left)
            {
                left.instruction = getInstructionForObvious(3, via_edge, false, left);
                right.instruction = {findBasicTurnType(via_edge, right),
                                     DirectionModifier::SlightRight};
            }
            else
            {
                if (low_priority_left && !low_priority_right)
                {
                    left.instruction = {findBasicTurnType(via_edge, left),
                                        DirectionModifier::SlightLeft};
                    right.instruction = {findBasicTurnType(via_edge, right),
                                         DirectionModifier::SlightRight};
                }
                else
                {
                    left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
                    right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
                }
            }
        }
        else
        {
            left.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            right.instruction = {findBasicTurnType(via_edge, right),
                                 DirectionModifier::SlightRight};
        }
    }
    else if (angularDeviation(right.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
             angularDeviation(left.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
    {
        // right side is actually straight
        const auto &out_data = node_based_graph.GetEdgeData(right.eid);
        if (angularDeviation(right.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            angularDeviation(left.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
        {
            if (detail::requiresAnnouncement(in_data, out_data))
            {
                if (low_priority_left && !low_priority_right)
                {
                    left.instruction = {findBasicTurnType(via_edge, left),
                                        DirectionModifier::SlightLeft};
                    right.instruction = getInstructionForObvious(3, via_edge, false, right);
                }
                else
                {
                    if (low_priority_right && !low_priority_left)
                    {
                        left.instruction = {findBasicTurnType(via_edge, left),
                                            DirectionModifier::SlightLeft};
                        right.instruction = {findBasicTurnType(via_edge, right),
                                             DirectionModifier::SlightRight};
                    }
                    else
                    {
                        right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
                        left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
                    }
                }
            }
            else
            {
                right.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
                left.instruction = {findBasicTurnType(via_edge, left),
                                    DirectionModifier::SlightLeft};
            }
        }
    }
    // left side of fork
    if (low_priority_right && !low_priority_left)
        left.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
    else
    {
        if (low_priority_left && !low_priority_right)
            left.instruction = {TurnType::Turn, DirectionModifier::SlightLeft};
        else
            left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
    }

    // right side of fork
    if (low_priority_left && !low_priority_right)
        right.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
    else
    {
        if (low_priority_right && !low_priority_left)
            right.instruction = {TurnType::Turn, DirectionModifier::SlightRight};
        else
            right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
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
        left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        if (angularDeviation(center.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        {
            const auto &in_data = node_based_graph.GetEdgeData(via_edge);
            const auto &out_data = node_based_graph.GetEdgeData(center.eid);
            if (detail::requiresAnnouncement(in_data, out_data))
            {
                center.instruction = {TurnType::Fork, DirectionModifier::Straight};
            }
            else
            {
                center.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            }
        }
        else
        {
            center.instruction = {TurnType::Fork, DirectionModifier::Straight};
        }
        right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
    }
    else if (left.entry_allowed)
    {
        if (right.entry_allowed)
            assignFork(via_edge, left, right);
        else if (center.entry_allowed)
            assignFork(via_edge, left, center);
        else
            left.instruction = {findBasicTurnType(via_edge, left), getTurnDirection(left.angle)};
    }
    else if (right.entry_allowed)
    {
        if (center.entry_allowed)
            assignFork(via_edge, center, right);
        else
            right.instruction = {findBasicTurnType(via_edge, right), getTurnDirection(right.angle)};
    }
    else
    {
        if (center.entry_allowed)
            center.instruction = {findBasicTurnType(via_edge, center),
                                  getTurnDirection(center.angle)};
    }
}

void IntersectionHandler::assignTrivialTurns(const EdgeID via_eid,
                                             Intersection &intersection,
                                             const std::size_t begin,
                                             const std::size_t end) const
{
    for (std::size_t index = begin; index != end; ++index)
        if (intersection[index].entry_allowed)
            intersection[index].instruction = {findBasicTurnType(via_eid, intersection[index]),
                                               getTurnDirection(intersection[index].angle)};
}

bool IntersectionHandler::isThroughStreet(const std::size_t index,
                                          const Intersection &intersection) const
{
    const auto &data_at_index = node_based_graph.GetEdgeData(intersection[index].eid);

    if (data_at_index.name_id == EMPTY_NAMEID)
        return false;

    // a through street cannot start at our own position -> index 1
    for (std::size_t road_index = 1; road_index < intersection.size(); ++road_index)
    {
        if (road_index == index)
            continue;

        const auto &road = intersection[road_index];
        const auto &road_data = node_based_graph.GetEdgeData(road.eid);

        // roads have a near straight angle (180 degree)
        const bool is_nearly_straight = angularDeviation(road.angle, intersection[index].angle) >
                                        (STRAIGHT_ANGLE - FUZZY_ANGLE_DIFFERENCE);

        const bool have_same_name =
            road_data.name_id != EMPTY_NAMEID &&
            !util::guidance::requiresNameAnnounced(
                data_at_index.name_id, road_data.name_id, name_table, street_name_suffix_table);

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

    for (std::size_t i = 1; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].angle, STRAIGHT_ANGLE);
        if (!intersection[i].entry_allowed)
            continue;

        const auto out_data = node_based_graph.GetEdgeData(intersection[i].eid);
        const auto continue_class =
            node_based_graph.GetEdgeData(intersection[best_continue].eid).road_classification;

        const auto same_name = !util::guidance::requiresNameAnnounced(
            in_data.name_id, out_data.name_id, name_table, street_name_suffix_table);

        if (same_name && (best_continue == 0 || (continue_class.GetPriority() >
                                                     out_data.road_classification.GetPriority() &&
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
            node_based_graph.GetEdgeData(intersection[best_continue].eid).road_classification;

        // don't prefer low priority classes
        if (best != 0 && out_data.road_classification.IsLowPriorityRoadClass() &&
            !current_best_class.IsLowPriorityRoadClass())
            continue;

        const bool is_better_choice_by_priority =
            best == 0 || obviousByRoadClass(in_data.road_classification,
                                            out_data.road_classification,
                                            current_best_class);

        const bool other_is_better_choice_by_priority =
            best != 0 && obviousByRoadClass(in_data.road_classification,
                                            current_best_class,
                                            out_data.road_classification);

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
                const auto &road_data = node_based_graph.GetEdgeData(road.eid);

                const auto same_name = !util::guidance::requiresNameAnnounced(
                    in_data.name_id, road_data.name_id, name_table, street_name_suffix_table);

                if (same_name)
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
        angularDeviation(intersection[best].angle, STRAIGHT_ANGLE) <
            MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
        node_based_graph.GetEdgeData(intersection[best_continue].eid).road_classification ==
            node_based_graph.GetEdgeData(intersection[best].eid).road_classification)
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
                       const auto &road_data = node_based_graph.GetEdgeData(road.eid);
                       const auto same_name =
                           !util::guidance::requiresNameAnnounced(in_data.name_id,
                                                                  road_data.name_id,
                                                                  name_table,
                                                                  street_name_suffix_table);

                       return same_name &&
                              angularDeviation(road.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE;
                   }) == num_continue_names.first;
    }();

    // has no obvious continued road
    const auto &best_data = node_based_graph.GetEdgeData(intersection[best].eid);

    const auto check_non_continue = [&]() {
        // no continue road exists
        if (best_continue == 0)
            return true;

        // we have multiple continues and not all are narrow (treat all the same)
        if (!all_continues_are_narrow &&
            (num_continue_names.first >= 2 && intersection.size() >= 4))
            return true;

        // if the best continue is not narrow and we also have at least 2 possible choices, the
        // intersection size does not matter anymore
        if (num_continue_names.second >= 2 && best_continue_deviation >= 2 * NARROW_TURN_ANGLE)
            return true;

        // continue data now most certainly exists
        const auto &continue_data = node_based_graph.GetEdgeData(intersection[best_continue].eid);

        if (obviousByRoadClass(in_data.road_classification,
                               continue_data.road_classification,
                               best_data.road_classification))
            return false;

        if (obviousByRoadClass(in_data.road_classification,
                               best_data.road_classification,
                               continue_data.road_classification))
            return true;

        // the best deviation is very straight and not a ramp
        if (best_deviation < best_continue_deviation && best_deviation < FUZZY_ANGLE_DIFFERENCE &&
            !best_data.road_classification.IsRampClass())
            return true;

        // the continue road is of a lower priority, while the road continues on the same priority
        // with a better angle
        if (best_deviation < best_continue_deviation &&
            in_data.road_classification == best_data.road_classification &&
            continue_data.road_classification.GetPriority() >
                best_data.road_classification.GetPriority())
            return true;

        return false;
    }();

    if (check_non_continue)
    {
        // Find left/right deviation
        // skipping over service roads
        const std::size_t left_index = [&]() {
            const auto index_candidate = (best + 1) % intersection.size();
            if (index_candidate == 0)
                return index_candidate;
            const auto &candidate_data =
                node_based_graph.GetEdgeData(intersection[index_candidate].eid);
            if (obviousByRoadClass(in_data.road_classification,
                                   best_data.road_classification,
                                   candidate_data.road_classification))
                return (index_candidate + 1) % intersection.size();
            else
                return index_candidate;

        }();
        const auto right_index = [&]() {
            BOOST_ASSERT(best > 0);
            const auto index_candidate = best - 1;
            if (index_candidate == 0)
                return index_candidate;
            const auto candidate_data =
                node_based_graph.GetEdgeData(intersection[index_candidate].eid);
            if (obviousByRoadClass(in_data.road_classification,
                                   best_data.road_classification,
                                   candidate_data.road_classification))
                return index_candidate - 1;
            else
                return index_candidate;
        }();

        const double left_deviation =
            angularDeviation(intersection[left_index].angle, STRAIGHT_ANGLE);
        const double right_deviation =
            angularDeviation(intersection[right_index].angle, STRAIGHT_ANGLE);

        if (best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            std::min(left_deviation, right_deviation) > FUZZY_ANGLE_DIFFERENCE)
            return best;

        const auto &left_data = node_based_graph.GetEdgeData(intersection[left_index].eid);
        const auto &right_data = node_based_graph.GetEdgeData(intersection[right_index].eid);

        const bool obvious_to_left =
            left_index == 0 || obviousByRoadClass(in_data.road_classification,
                                                  best_data.road_classification,
                                                  left_data.road_classification);
        const bool obvious_to_right =
            right_index == 0 || obviousByRoadClass(in_data.road_classification,
                                                   best_data.road_classification,
                                                   right_data.road_classification);

        // if the best turn isn't narrow, but there is a nearly straight turn, we don't consider the
        // turn obvious
        const auto check_narrow = [&intersection, best_deviation](const std::size_t index) {
            return angularDeviation(intersection[index].angle, STRAIGHT_ANGLE) <=
                       FUZZY_ANGLE_DIFFERENCE &&
                   (best_deviation > NARROW_TURN_ANGLE || intersection[index].entry_allowed);
        };

        // other narrow turns?
        if (check_narrow(right_index) && !obvious_to_right)
            return 0;

        if (check_narrow(left_index) && !obvious_to_left)
            return 0;

        // check if a turn is distinct enough
        const auto isDistinct = [&](const std::size_t index, const double deviation) {
            /*
               If the neighbor is not possible to enter, we allow for a lower
               distinction rate. If the road category is smaller, its also adjusted. Only
               roads of the same priority require the full distinction ratio.
             */
            const auto adjusted_distinction_ratio = [&]() {
                // not allowed competitors are easily distinct
                if (!intersection[index].entry_allowed)
                    return 0.7 * DISTINCTION_RATIO;
                // a bit less obvious are road classes
                else if (in_data.road_classification == best_data.road_classification &&
                         best_data.road_classification.GetPriority() <
                             node_based_graph.GetEdgeData(intersection[index].eid)
                                 .road_classification.GetPriority())
                    return 0.8 * DISTINCTION_RATIO;
                // if road classes are the same, we use the full ratio
                else
                    return DISTINCTION_RATIO;
            }();
            return index == 0 || deviation / best_deviation >= adjusted_distinction_ratio ||
                   (deviation <= NARROW_TURN_ANGLE && !intersection[index].entry_allowed);
        };

        const bool distinct_to_left = isDistinct(left_index, left_deviation);
        const bool distinct_to_right = isDistinct(right_index, right_deviation);
        // Well distinct turn that is nearly straight
        if ((distinct_to_left || obvious_to_left) && (distinct_to_right || obvious_to_right))
            return best;
    }
    else
    {
        const double deviation =
            angularDeviation(intersection[best_continue].angle, STRAIGHT_ANGLE);
        const auto &continue_data = node_based_graph.GetEdgeData(intersection[best_continue].eid);
        if (std::abs(deviation) < 1)
            return best_continue;

        // check if any other similar best continues exist
        for (std::size_t i = 1; i < intersection.size(); ++i)
        {
            if (i == best_continue || !intersection[i].entry_allowed)
                continue;

            const auto &turn_data = node_based_graph.GetEdgeData(intersection[i].eid);
            const bool is_obvious_by_road_class =
                obviousByRoadClass(in_data.road_classification,
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
            const auto turn_deviation = angularDeviation(intersection[i].angle, STRAIGHT_ANGLE);
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

            const auto same_name = !util::guidance::requiresNameAnnounced(
                turn_data.name_id, continue_data.name_id, name_table, street_name_suffix_table);

            if (same_name && deviation_ratio < 1.5 * DISTINCTION_RATIO)
                return 0;
        }

        // Segregated intersections can result in us finding an obvious turn, even though its only
        // obvious due to a very short segment in between. So if the segment coming in is very
        // short, we check the previous intersection for other continues in the opposite bearing.
        const auto node_at_intersection = node_based_graph.GetTarget(via_edge);
        const util::Coordinate coordinate_at_intersection = node_info_list[node_at_intersection];

        const auto node_at_u_turn = node_based_graph.GetTarget(intersection[0].eid);
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
                node_at_intersection, intersection[0].eid, nullptr, nullptr);

            const auto continue_road = intersection[best_continue];
            for (const auto &comparison_road : previous_intersection)
            {
                // since we look at the intersection in the wrong direction, a similar angle
                // actually represents a near 180 degree different in bearings between the two
                // roads. So if there is a road that is enterable in the opposite direction just
                // prior, a turn is not obvious
                const auto &turn_data = node_based_graph.GetEdgeData(comparison_road.eid);
                if (angularDeviation(comparison_road.angle, STRAIGHT_ANGLE) > GROUP_ANGLE &&
                    angularDeviation(comparison_road.angle, continue_road.angle) <
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
