#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>

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
    return !from.IsCompatibleTo(to);
}
}

IntersectionHandler::IntersectionHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                         const std::vector<QueryNode> &node_info_list,
                                         const util::NameTable &name_table,
                                         const SuffixTable &street_name_suffix_table)
    : node_based_graph(node_based_graph), node_info_list(node_info_list), name_table(name_table),
      street_name_suffix_table(street_name_suffix_table)
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
            requiresNameAnnounced(name_table.GetNameForID(in_data.name_id),
                                  name_table.GetNameForID(out_data.name_id),
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
    for (const auto &road : intersection)
    {
        // a through street cannot start at our own position
        if (road.turn.angle < std::numeric_limits<double>::epsilon())
            continue;
        if (angularDeviation(road.turn.angle, intersection[index].turn.angle) >
                (STRAIGHT_ANGLE - NARROW_TURN_ANGLE) &&
            node_based_graph.GetEdgeData(road.turn.eid).name_id ==
                node_based_graph.GetEdgeData(intersection[index].turn.eid).name_id)
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
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (intersection[i].entry_allowed && deviation < best_deviation)
        {
            best_deviation = deviation;
            best = i;
        }

        const auto out_data = node_based_graph.GetEdgeData(intersection[i].turn.eid);
        auto continue_class =
            node_based_graph.GetEdgeData(intersection[best_continue].turn.eid).road_classification;
        if (intersection[i].entry_allowed && out_data.name_id == in_data.name_id &&
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
    }

    if (best == 0)
        return 0;

    if (best_deviation >= 2 * NARROW_TURN_ANGLE)
        return 0;
    // has no obvious continued road
    if (best_continue == 0 || best_continue_deviation >= 2 * NARROW_TURN_ANGLE ||
        (node_based_graph.GetEdgeData(intersection[best_continue].turn.eid).road_classification ==
             node_based_graph.GetEdgeData(intersection[best].turn.eid).road_classification &&
         std::abs(best_continue_deviation) > 1 && best_deviation / best_continue_deviation < 0.75))
    {
        // Find left/right deviation
        const double left_deviation = angularDeviation(
            intersection[(best + 1) % intersection.size()].turn.angle, STRAIGHT_ANGLE);
        const double right_deviation =
            angularDeviation(intersection[best - 1].turn.angle, STRAIGHT_ANGLE);

        if (best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            std::min(left_deviation, right_deviation) > FUZZY_ANGLE_DIFFERENCE)
            return best;

        // other narrow turns?
        if (angularDeviation(intersection[best - 1].turn.angle, STRAIGHT_ANGLE) <=
            FUZZY_ANGLE_DIFFERENCE)
            return 0;
        if (angularDeviation(intersection[(best + 1) % intersection.size()].turn.angle,
                             STRAIGHT_ANGLE) <= FUZZY_ANGLE_DIFFERENCE)
            return 0;

        // Well distinct turn that is nearly straight
        if ((left_deviation / best_deviation >= DISTINCTION_RATIO ||
             (left_deviation > best_deviation &&
              !intersection[(best + 1) % intersection.size()].entry_allowed)) &&
            (right_deviation / best_deviation >= DISTINCTION_RATIO ||
             (right_deviation > best_deviation && !intersection[best - 1].entry_allowed)))
        {
            return best;
        }
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

            if (angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE) / deviation < 1.1 &&
                continue_data.road_classification ==
                    node_based_graph.GetEdgeData(intersection[i].turn.eid).road_classification)
                return 0;
        }
        return best_continue; // no obvious turn
    }

    return 0;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
