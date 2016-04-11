#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/simple_logger.hpp"

#include <algorithm>

using EdgeData = osrm::util::NodeBasedDynamicGraph::EdgeData;

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
                                         const util::NameTable &name_table)
    : node_based_graph(node_based_graph), node_info_list(node_info_list), name_table(name_table)
{
}

IntersectionHandler::~IntersectionHandler() {}

std::size_t IntersectionHandler::countValid(const Intersection &intersection) const
{
    return std::count_if(intersection.begin(), intersection.end(),
                         [](const ConnectedRoad &road) { return road.entry_allowed; });
}

TurnType IntersectionHandler::findBasicTurnType(const EdgeID via_edge,
                                                const ConnectedRoad &road) const
{

    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);

    bool on_ramp = isRampClass(in_data.road_classification.road_class);

    bool onto_ramp = isRampClass(out_data.road_classification.road_class);

    if (!on_ramp && onto_ramp)
        return TurnType::Ramp;

    if (in_data.name_id == out_data.name_id && in_data.name_id != INVALID_NAME_ID)
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
    if (type == TurnType::Ramp)
    {
        return {TurnType::Ramp, getTurnDirection(road.turn.angle)};
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
                                  name_table.GetNameForID(out_data.name_id)))
        {
            // obvious turn onto a through street is a merge
            if (through_street)
            {
                return {TurnType::Merge, road.turn.angle > STRAIGHT_ANGLE
                                             ? DirectionModifier::SlightRight
                                             : DirectionModifier::SlightLeft};
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
    const bool low_priority_left = isLowPriorityRoadClass(
        node_based_graph.GetEdgeData(left.turn.eid).road_classification.road_class);
    const bool low_priority_right = isLowPriorityRoadClass(
        node_based_graph.GetEdgeData(right.turn.eid).road_classification.road_class);
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

void IntersectionHandler::assignCountingTurns(const EdgeID via_eid,
                                              Intersection &intersection,
                                              const std::size_t begin,
                                              const std::size_t end,
                                              const DirectionModifier modifier) const
{
    const constexpr TurnType turns[] = {TurnType::FirstTurn, TurnType::SecondTurn,
                                        TurnType::ThirdTurn, TurnType::FourthTurn};
    const constexpr TurnType ramps[] = {TurnType::FirstRamp, TurnType::SecondRamp,
                                        TurnType::ThirdRamp, TurnType::FourthRamp};

    const std::size_t length = end > begin ? end - begin : begin - end;
    if (length > 4)
    {
        util::SimpleLogger().Write(logDEBUG) << "Counting Turn assignment called for " << length
                                             << " turns. Supports at most four turns.";
    }

    // counting turns varies whether we consider left/right turns
    for (std::size_t index = begin, count = 0; index != end;
         count++, begin < end ? ++index : --index)
    {
        if (TurnType::Ramp == findBasicTurnType(via_eid, intersection[index]))
            intersection[index].turn.instruction = {ramps[count], modifier};
        else
            intersection[index].turn.instruction = {turns[count], modifier};
    }
}

bool IntersectionHandler::isThroughStreet(const std::size_t index,
                                          const Intersection &intersection) const
{
    if (node_based_graph.GetEdgeData(intersection[index].turn.eid).name_id == INVALID_NAME_ID)
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

} // namespace guidance
} // namespace extractor
} // namespace osrm
