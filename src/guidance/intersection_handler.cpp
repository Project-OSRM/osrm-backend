#include "guidance/intersection_handler.hpp"
#include "extractor/intersection/intersection_analysis.hpp"
#include "guidance/constants.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/log.hpp"

#include "util/bearing.hpp"

#include <algorithm>
#include <cstddef>

using EdgeData = osrm::util::NodeBasedDynamicGraph::EdgeData;
using osrm::util::angularDeviation;

namespace osrm::guidance
{

namespace detail
{
// TODO check flags!
inline bool requiresAnnouncement(const util::NodeBasedDynamicGraph &node_based_graph,
                                 const extractor::EdgeBasedNodeDataContainer &node_data_container,
                                 const EdgeID from,
                                 const EdgeID to)
{
    const auto &from_edge = node_based_graph.GetEdgeData(from);
    const auto &to_edge = node_based_graph.GetEdgeData(to);

    if (from_edge.reversed != to_edge.reversed)
        return true;

    if (!(from_edge.flags == to_edge.flags))
        return true;

    const auto &annotation_from = node_data_container.GetAnnotation(from_edge.annotation_data);
    const auto &annotation_to = node_data_container.GetAnnotation(to_edge.annotation_data);
    return !annotation_from.CanCombineWith(annotation_to);
}
} // namespace detail

IntersectionHandler::IntersectionHandler(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const extractor::EdgeBasedNodeDataContainer &node_data_container,
    const std::vector<util::Coordinate> &node_coordinates,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const extractor::RestrictionMap &node_restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const extractor::TurnLanesIndexedArray &turn_lanes_data,
    const extractor::NameTable &name_table,
    const extractor::SuffixTable &street_name_suffix_table)
    : node_based_graph(node_based_graph), node_data_container(node_data_container),
      node_coordinates(node_coordinates), compressed_geometries(compressed_geometries),
      node_restriction_map(node_restriction_map), barrier_nodes(barrier_nodes),
      turn_lanes_data(turn_lanes_data), name_table(name_table),
      street_name_suffix_table(street_name_suffix_table), graph_walker(node_based_graph,
                                                                       node_data_container,
                                                                       node_coordinates,
                                                                       compressed_geometries,
                                                                       node_restriction_map,
                                                                       barrier_nodes,
                                                                       turn_lanes_data)
{
}

// Inspects an intersection and a turn from via_edge onto road from the possible basic turn types
// (OnRamp, Continue, Turn) find the suitable turn type
TurnType::Enum IntersectionHandler::findBasicTurnType(const EdgeID via_edge,
                                                      const ConnectedRoad &road) const
{

    bool on_ramp = node_based_graph.GetEdgeData(via_edge).flags.road_classification.IsRampClass();
    bool onto_ramp = node_based_graph.GetEdgeData(road.eid).flags.road_classification.IsRampClass();

    if (!on_ramp && onto_ramp)
        return TurnType::OnRamp;

    const auto &in_name_id =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(via_edge).annotation_data)
            .name_id;
    const auto &out_name_id =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
            .name_id;
    const auto &in_name_empty = name_table.GetNameForID(in_name_id).empty();
    const auto &out_name_empty = name_table.GetNameForID(out_name_id).empty();

    const auto same_name = !util::guidance::requiresNameAnnounced(
        in_name_id, out_name_id, name_table, street_name_suffix_table);

    if (!in_name_empty && !out_name_empty && same_name)
    {
        return TurnType::Continue;
    }

    return TurnType::Turn;
}

TurnType::Enum IntersectionHandler::areSameClasses(const EdgeID via_edge,
                                                   const ConnectedRoad &road) const
{
    const auto &in_classes =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(via_edge).annotation_data)
            .classes;
    const auto &out_classes =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
            .classes;

    return in_classes == out_classes;
}

TurnInstruction IntersectionHandler::getInstructionForObvious(const std::size_t num_roads,
                                                              const EdgeID via_edge,
                                                              const bool through_street,
                                                              const ConnectedRoad &road) const
{
    const auto type = findBasicTurnType(via_edge, road);
    if (type == TurnType::OnRamp)
    {
        return {TurnType::OnRamp, getTurnDirection(road.angle)};
    }

    if (angularDeviation(road.angle, 0) < 0.01)
    {
        return {TurnType::Continue, DirectionModifier::UTurn};
    }

    // handle travel modes:
    const auto in_mode =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(via_edge).annotation_data)
            .travel_mode;
    const auto out_mode =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
            .travel_mode;
    const auto needs_notification = in_mode != out_mode;

    if (type == TurnType::Turn)
    {
        const auto &in_classification = node_based_graph.GetEdgeData(via_edge).flags;
        const auto &in_data = node_data_container.GetAnnotation(
            node_based_graph.GetEdgeData(via_edge).annotation_data);
        const auto &out_classification = node_based_graph.GetEdgeData(road.eid).flags;
        const auto &out_data = node_data_container.GetAnnotation(
            node_based_graph.GetEdgeData(road.eid).annotation_data);

        if (util::guidance::requiresNameAnnounced(
                in_data.name_id, out_data.name_id, name_table, street_name_suffix_table))
        {
            // obvious turn onto a through street is a merge
            if (through_street)
            {
                // We reserve merges for motorway types. All others are considered for simply going
                // straight onto a road. This avoids confusion about merge directions on streets
                // that could potentially also offer different choices
                if (out_classification.road_classification.IsMotorwayClass())
                    return {TurnType::Merge,
                            road.angle > STRAIGHT_ANGLE ? DirectionModifier::SlightRight
                                                        : DirectionModifier::SlightLeft};
                else if (in_classification.road_classification.IsRampClass() &&
                         out_classification.road_classification.IsRampClass())
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
                    // is obvious, the choice is arguably better. We need the road to continue for a
                    // bit though, until we assume this is safe to do. In addition, the angle cannot
                    // get too wide, so we only allow narrow turn angles to begin with.

                    // FIXME this requires https://github.com/Project-OSRM/osrm-backend/pull/2399,
                    // since `distance` does not refer to an actual distance but rather to the
                    // duration/weight of the traversal. We can only approximate the distance here
                    // or actually follow the full road. When 2399 lands, we can exchange here for a
                    // precalculated distance value.
                    const auto distance = util::coordinate_calculation::greatCircleDistance(
                        node_coordinates[node_based_graph.GetTarget(via_edge)],
                        node_coordinates[node_based_graph.GetTarget(road.eid)]);

                    return {TurnType::Turn,
                            (angularDeviation(road.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
                             distance > 2 * MAX_COLLAPSE_DISTANCE)
                                ? DirectionModifier::Straight
                                : getTurnDirection(road.angle)};
                }
            }
            else
            {
                return {needs_notification ? TurnType::Notification : TurnType::NewName,
                        getTurnDirection(road.angle)};
            }
        }
        // name has not changed, suppress a turn here or indicate mode change
        else
        {
            if (needs_notification)
                return {TurnType::Notification, getTurnDirection(road.angle)};
            else
                return {num_roads == 2 && areSameClasses(via_edge, road) ? TurnType::NoTurn
                                                                         : TurnType::Suppressed,
                        getTurnDirection(road.angle)};
        }
    }
    BOOST_ASSERT(type == TurnType::Continue);
    if (needs_notification)
    {
        return {TurnType::Notification, getTurnDirection(road.angle)};
    }
    if (num_roads > 2 || !areSameClasses(via_edge, road))
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
    const auto &in_data =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(via_edge).annotation_data);
    const auto &lhs_classification =
        node_based_graph.GetEdgeData(left.eid).flags.road_classification;
    const auto &lhs_data =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(left.eid).annotation_data);
    const auto &rhs_classification =
        node_based_graph.GetEdgeData(right.eid).flags.road_classification;
    const auto &rhs_data =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(right.eid).annotation_data);
    const bool low_priority_left = lhs_classification.IsLowPriorityRoadClass();
    const bool low_priority_right = rhs_classification.IsLowPriorityRoadClass();
    const auto same_mode_left = in_data.travel_mode == lhs_data.travel_mode;
    const auto same_mode_right = in_data.travel_mode == rhs_data.travel_mode;
    const auto suppressed_left_type =
        same_mode_left ? TurnType::Suppressed : TurnType::Notification;
    const auto suppressed_right_type =
        same_mode_right ? TurnType::Suppressed : TurnType::Notification;
    if ((angularDeviation(left.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
         angularDeviation(right.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE))
    {
        // left side is actually straight
        if (detail::requiresAnnouncement(node_based_graph, node_data_container, via_edge, left.eid))
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
        if (angularDeviation(right.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            angularDeviation(left.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
        {
            if (detail::requiresAnnouncement(
                    node_based_graph, node_data_container, via_edge, right.eid))
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
        right.instruction = {suppressed_right_type, DirectionModifier::SlightRight};
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
    const auto suppressed_type = [&](const ConnectedRoad &road)
    {
        const auto in_mode =
            node_data_container
                .GetAnnotation(node_based_graph.GetEdgeData(via_edge).annotation_data)
                .travel_mode;
        const auto out_mode =
            node_data_container
                .GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
                .travel_mode;
        return in_mode == out_mode ? TurnType::Suppressed : TurnType::Notification;
    };

    if (left.entry_allowed && center.entry_allowed && right.entry_allowed)
    {
        left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        if (angularDeviation(center.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        {
            if (detail::requiresAnnouncement(
                    node_based_graph, node_data_container, via_edge, center.eid))
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

std::optional<IntersectionHandler::IntersectionViewAndNode>
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

    const auto intersection_parameters =
        extractor::intersection::skipDegreeTwoNodes(node_based_graph, {at, via});
    // This should never happen, guard against nevertheless
    if (intersection_parameters.node == SPECIAL_NODEID ||
        intersection_parameters.edge == SPECIAL_EDGEID)
    {
        return std::nullopt;
    }

    auto intersection = extractor::intersection::getConnectedRoads<false>(node_based_graph,
                                                                          node_data_container,
                                                                          node_coordinates,
                                                                          compressed_geometries,
                                                                          node_restriction_map,
                                                                          barrier_nodes,
                                                                          turn_lanes_data,
                                                                          intersection_parameters);
    auto intersection_node = node_based_graph.GetTarget(intersection_parameters.edge);

    if (intersection.size() <= 2 || intersection.isTrafficSignalOrBarrier())
    {
        return std::nullopt;
    }

    return std::make_optional(IntersectionViewAndNode{std::move(intersection), intersection_node});
}

bool IntersectionHandler::isSameName(const EdgeID source_edge_id, const EdgeID target_edge_id) const
{
    const auto &source_edge_data = node_data_container.GetAnnotation(
        node_based_graph.GetEdgeData(source_edge_id).annotation_data);
    const auto &target_edge_data = node_data_container.GetAnnotation(
        node_based_graph.GetEdgeData(target_edge_id).annotation_data);

    return !name_table.GetNameForID(source_edge_data.name_id).empty() && //
           !name_table.GetNameForID(target_edge_data.name_id).empty() && //
           !util::guidance::requiresNameAnnounced(source_edge_data.name_id,
                                                  target_edge_data.name_id,
                                                  name_table,
                                                  street_name_suffix_table); //
}

} // namespace osrm::guidance
