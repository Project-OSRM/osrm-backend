#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/guidance/constants.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/log.hpp"

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cstddef>

using EdgeData = osrm::util::NodeBasedDynamicGraph::EdgeData;
using osrm::extractor::guidance::getTurnDirection;
using osrm::util::angularDeviation;

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
                                         const std::vector<util::Coordinate> &coordinates,
                                         const util::NameTable &name_table,
                                         const SuffixTable &street_name_suffix_table,
                                         const IntersectionGenerator &intersection_generator)
    : node_based_graph(node_based_graph), coordinates(coordinates), name_table(name_table),
      street_name_suffix_table(street_name_suffix_table),
      intersection_generator(intersection_generator),
      graph_walker(node_based_graph, intersection_generator)
{
}

// Inspects an intersection and a turn from via_edge onto road from the possible basic turn types
// (OnRamp, Continue, Turn) find the suitable turn type
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
                        coordinates[node_based_graph.GetTarget(via_edge)],
                        coordinates[node_based_graph.GetTarget(road.eid)]);
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
            if (in_mode != out_mode)
                return {TurnType::Notification, getTurnDirection(road.angle)};
            else
                return {num_roads == 2 ? TurnType::NoTurn : TurnType::Suppressed,
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
    const auto same_mode_left =
        in_data.travel_mode == node_based_graph.GetEdgeData(left.eid).travel_mode;
    const auto same_mode_right =
        in_data.travel_mode == node_based_graph.GetEdgeData(right.eid).travel_mode;
    const auto suppressed_left_type =
        same_mode_left ? TurnType::Suppressed : TurnType::Notification;
    const auto suppressed_right_type =
        same_mode_right ? TurnType::Suppressed : TurnType::Notification;
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
            left.instruction = {suppressed_left_type, DirectionModifier::Straight};
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
                right.instruction = {suppressed_right_type, DirectionModifier::Straight};
                left.instruction = {findBasicTurnType(via_edge, left),
                                    DirectionModifier::SlightLeft};
            }
        }
    }
    // left side of fork
    if (low_priority_right && !low_priority_left)
        left.instruction = {suppressed_left_type, DirectionModifier::SlightLeft};
    else
    {
        if (low_priority_left && !low_priority_right)
        {
            left.instruction = {TurnType::Turn, DirectionModifier::SlightLeft};
        }
        else
        {
            left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        }
    }

    // right side of fork
    if (low_priority_left && !low_priority_right)
        right.instruction = {suppressed_right_type, DirectionModifier::SlightLeft};
    else
    {
        if (low_priority_right && !low_priority_left)
        {
            right.instruction = {TurnType::Turn, DirectionModifier::SlightRight};
        }
        else
        {
            right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
        }
    }
}

void IntersectionHandler::assignFork(const EdgeID via_edge,
                                     ConnectedRoad &left,
                                     ConnectedRoad &center,
                                     ConnectedRoad &right) const
{
    // TODO handle low priority road classes in a reasonable way
    const auto suppressed_type = [&](const ConnectedRoad &road) {
        const auto in_mode = node_based_graph.GetEdgeData(via_edge).travel_mode;
        const auto out_mode = node_based_graph.GetEdgeData(road.eid).travel_mode;
        return in_mode == out_mode ? TurnType::Suppressed : TurnType::Notification;
    };

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
                center.instruction = {suppressed_type(center), DirectionModifier::Straight};
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
        {
            intersection[index].instruction = {findBasicTurnType(via_eid, intersection[index]),
                                               getTurnDirection(intersection[index].angle)};
        }
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

boost::optional<IntersectionHandler::IntersectionViewAndNode>
IntersectionHandler::getNextIntersection(const NodeID at, const EdgeID via) const
{
    // We use the intersection generator to jump over traffic signals, barriers. The intersection
    // generater takes a starting node and a corresponding edge starting at this node. It returns
    // the next non-artificial intersection writing as out param. the source node and the edge
    // for which the target is the next intersection.
    //
    // .            .
    // a . . tl . . c .
    // .            .
    //
    // e0 ^      ^ e1
    //
    // Starting at node `a` via edge `e0` the intersection generator returns the intersection at `c`
    // writing `tl` (traffic signal) node and the edge `e1` which has the intersection as target.

    const auto intersection_parameters = intersection_generator.SkipDegreeTwoNodes(at, via);
    // This should never happen, guard against nevertheless
    if (intersection_parameters.nid == SPECIAL_NODEID ||
        intersection_parameters.via_eid == SPECIAL_EDGEID)
    {
        return boost::none;
    }

    auto intersection =
        intersection_generator(intersection_parameters.nid, intersection_parameters.via_eid);
    auto intersection_node = node_based_graph.GetTarget(intersection_parameters.via_eid);

    if (intersection.size() <= 2 || intersection.isTrafficSignalOrBarrier())
    {
        return boost::none;
    }

    return boost::make_optional(
        IntersectionViewAndNode{std::move(intersection), intersection_node});
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
