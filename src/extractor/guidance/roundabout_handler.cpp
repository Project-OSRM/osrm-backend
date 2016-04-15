#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/roundabout_handler.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/simple_logger.hpp"

#include <cmath>
#include <set>
#include <unordered_set>

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

RoundaboutHandler::RoundaboutHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                     const std::vector<QueryNode> &node_info_list,
                                     const util::NameTable &name_table)
    : IntersectionHandler(node_based_graph, node_info_list, name_table)
{
}

RoundaboutHandler::~RoundaboutHandler() {}

bool RoundaboutHandler::canProcess(const NodeID from_nid,
                                   const EdgeID via_eid,
                                   const Intersection &intersection) const
{
    const auto flags = getRoundaboutFlags(from_nid, via_eid, intersection);
    return flags.on_roundabout || flags.can_enter;
}

Intersection RoundaboutHandler::
operator()(const NodeID from_nid, const EdgeID via_eid, Intersection intersection) const
{
    invalidateExitAgainstDirection(from_nid, via_eid, intersection);
    const auto flags = getRoundaboutFlags(from_nid, via_eid, intersection);
    const bool is_rotary = isRotary(node_based_graph.GetTarget(via_eid));
    // find the radius of the roundabout
    return handleRoundabouts(is_rotary, via_eid, flags.on_roundabout, flags.can_exit_separately,
                             std::move(intersection));
}

detail::RoundaboutFlags RoundaboutHandler::getRoundaboutFlags(
    const NodeID from_nid, const EdgeID via_eid, const Intersection &intersection) const
{
    const auto &in_edge_data = node_based_graph.GetEdgeData(via_eid);
    bool on_roundabout = in_edge_data.roundabout;
    bool can_enter_roundabout = false;
    bool can_exit_roundabout_separately = false;
    for (const auto &road : intersection)
    {
        const auto &edge_data = node_based_graph.GetEdgeData(road.turn.eid);
        // only check actual outgoing edges
        if (edge_data.reversed || !road.entry_allowed )
            continue;

        if (edge_data.roundabout)
        {
            can_enter_roundabout = true;
        }
        // Exiting roundabouts at an entry point is technically a data-modelling issue.
        // This workaround handles cases in which an exit follows the entry.
        // To correctly represent perceived exits, we only count exits leading to a
        // separate vertex than the one we are coming from that are in the direction of
        // the roundabout.
        // The sorting of the angles represents a problem for left-sided driving, though.
        // FIXME in case of left-sided driving, we have to check whether we can enter the
        // roundabout later in the cycle, rather than prior.
        // FIXME requires consideration of crossing the roundabout
        else if (node_based_graph.GetTarget(road.turn.eid) != from_nid && !can_enter_roundabout)
        {
            can_exit_roundabout_separately = true;
        }
    }
    return {on_roundabout, can_enter_roundabout, can_exit_roundabout_separately};
}

void RoundaboutHandler::invalidateExitAgainstDirection(const NodeID from_nid,
                                                       const EdgeID via_eid,
                                                       Intersection &intersection) const
{
    const auto &in_edge_data = node_based_graph.GetEdgeData(via_eid);
    if( in_edge_data.roundabout )
        return;

    bool past_roundabout_angle = false;
    for (auto &road : intersection)
    {
        const auto &edge_data = node_based_graph.GetEdgeData(road.turn.eid);
        // only check actual outgoing edges
        if (edge_data.reversed)
        {
            // remember whether we have seen the roundabout in-part
            if (edge_data.roundabout)
                past_roundabout_angle = true;

            continue;
        }

        // Exiting roundabouts at an entry point is technically a data-modelling issue.
        // This workaround handles cases in which an exit precedes and entry. The resulting
        // u-turn against the roundabout direction is invalidated.
        // The sorting of the angles represents a problem for left-sided driving, though.
        // FIXME in case of left-sided driving, we have to check whether we can enter the
        // roundabout later in the cycle, rather than prior.
        if (!edge_data.roundabout && node_based_graph.GetTarget(road.turn.eid) != from_nid &&
            past_roundabout_angle)
        {
            road.entry_allowed = false;
        }
    }
}

bool RoundaboutHandler::isRotary(const NodeID nid) const
{
    // translate a node ID into its respective coordinate stored in the node_info_list
    const auto getCoordinate = [this](const NodeID node) {
        return util::Coordinate(node_info_list[node].lon, node_info_list[node].lat);
    };

    unsigned roundabout_name_id = 0;
    std::unordered_set<unsigned> connected_names;

    const auto getNextOnRoundabout = [this, &roundabout_name_id,
                                      &connected_names](const NodeID node) {
        EdgeID continue_edge = SPECIAL_EDGEID;
        for (const auto edge : node_based_graph.GetAdjacentEdgeRange(node))
        {
            const auto &edge_data = node_based_graph.GetEdgeData(edge);
            if (!edge_data.reversed && edge_data.roundabout)
            {
                if (SPECIAL_EDGEID != continue_edge)
                {
                    // fork in roundabout
                    return SPECIAL_EDGEID;
                }
                // roundabout does not keep its name
                if (roundabout_name_id != 0 && roundabout_name_id != edge_data.name_id &&
                    requiresNameAnnounced(name_table.GetNameForID(roundabout_name_id),
                                          name_table.GetNameForID(edge_data.name_id)))
                {
                    return SPECIAL_EDGEID;
                }

                roundabout_name_id = edge_data.name_id;

                continue_edge = edge;
            }
            else if (!edge_data.roundabout)
            {
                // remember all connected road names
                connected_names.insert(edge_data.name_id);
            }
        }
        return continue_edge;
    };
    // the roundabout radius has to be the same for all locations we look at it from
    // to guarantee this, we search the full roundabout for its vertices
    // and select the three smalles ids
    std::set<NodeID> roundabout_nodes; // needs to be sorted

    // this value is a hard abort to deal with potential self-loops
    NodeID last_node = nid;
    while (0 == roundabout_nodes.count(last_node))
    {
        roundabout_nodes.insert(last_node);
        const auto eid = getNextOnRoundabout(last_node);

        if (eid == SPECIAL_EDGEID)
        {
            util::SimpleLogger().Write(logDEBUG) << "Non-Loop Roundabout found.";
            return false;
        }

        last_node = node_based_graph.GetTarget(eid);
    }

    // do we have a dedicated name for the rotary, if not its a roundabout
    // This function can theoretically fail if the roundabout name is partly
    // used with a reference and without. This will be fixed automatically
    // when we handle references separately or if the useage is more consistent
    if (roundabout_name_id == 0 || connected_names.count(roundabout_name_id))
    {
        return false;
    }

    if (roundabout_nodes.size() <= 1)
    {
        return false;
    }
    // calculate the radius of the roundabout/rotary. For two coordinates, we assume a minimal
    // circle
    // with both vertices right at the other side (so half their distance in meters).
    // Otherwise, we construct a circle through the first tree vertices.
    const auto getRadius = [&roundabout_nodes, &getCoordinate]() {
        auto node_itr = roundabout_nodes.begin();
        if (roundabout_nodes.size() == 2)
        {
            const auto first = getCoordinate(*node_itr++), second = getCoordinate(*node_itr++);
            return 0.5 * util::coordinate_calculation::haversineDistance(first, second);
        }
        else
        {
            const auto first = getCoordinate(*node_itr++), second = getCoordinate(*node_itr++),
                       third = getCoordinate(*node_itr++);
            return util::coordinate_calculation::circleRadius(first, second, third);
        }
    };
    const double radius = getRadius();

    // check whether the circle computation has gone wrong
    // The radius computation can result in infinity, if the three coordinates are non-distinct.
    // To stay on the safe side, we say its not a rotary
    if (std::isinf(radius))
        return false;

    return radius > MAX_ROUNDABOUT_RADIUS;
}

Intersection RoundaboutHandler::handleRoundabouts(const bool is_rotary,
                                                  const EdgeID via_eid,
                                                  const bool on_roundabout,
                                                  const bool can_exit_roundabout_separately,
                                                  Intersection intersection) const
{
    // detect via radius (get via circle through three vertices)
    NodeID node_v = node_based_graph.GetTarget(via_eid);
    if (on_roundabout)
    {
        // Shoule hopefully have only a single exit and continue
        // at least for cars. How about bikes?
        for (auto &road : intersection)
        {
            auto &turn = road.turn;
            const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
            if (out_data.roundabout)
            {
                // TODO can forks happen in roundabouts? E.g. required lane changes
                if (1 == node_based_graph.GetDirectedOutDegree(node_v))
                {
                    // No turn possible.
                    turn.instruction = TurnInstruction::NO_TURN();
                }
                else
                {
                    turn.instruction =
                        TurnInstruction::REMAIN_ROUNDABOUT(is_rotary, getTurnDirection(turn.angle));
                }
            }
            else
            {
                turn.instruction =
                    TurnInstruction::EXIT_ROUNDABOUT(is_rotary, getTurnDirection(turn.angle));
            }
        }
        return intersection;
    }
    else
        for (auto &road : intersection)
        {
            if (!road.entry_allowed)
                continue;
            auto &turn = road.turn;
            const auto &out_data = node_based_graph.GetEdgeData(turn.eid);
            if (out_data.roundabout)
            {
                turn.instruction =
                    TurnInstruction::ENTER_ROUNDABOUT(is_rotary, getTurnDirection(turn.angle));
                if (can_exit_roundabout_separately)
                {
                    if (turn.instruction.type == TurnType::EnterRotary)
                        turn.instruction.type = TurnType::EnterRotaryAtExit;
                    if (turn.instruction.type == TurnType::EnterRoundabout)
                        turn.instruction.type = TurnType::EnterRoundaboutAtExit;
                }
            }
            else
            {
                turn.instruction = TurnInstruction::ENTER_AND_EXIT_ROUNDABOUT(
                    is_rotary, getTurnDirection(turn.angle));
            }
        }
    return intersection;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
