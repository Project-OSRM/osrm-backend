#include "extractor/guidance/roundabout_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>
#include <cmath>

#include <boost/assert.hpp>

using osrm::util::guidance::getTurnDirection;

namespace osrm
{
namespace extractor
{
namespace guidance
{

RoundaboutHandler::RoundaboutHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                     const std::vector<QueryNode> &node_info_list,
                                     const CompressedEdgeContainer &compressed_edge_container,
                                     const util::NameTable &name_table,
                                     const SuffixTable &street_name_suffix_table,
                                     const ProfileProperties &profile_properties,
                                     const IntersectionGenerator &intersection_generator)
    : IntersectionHandler(node_based_graph,
                          node_info_list,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator),
      compressed_edge_container(compressed_edge_container), profile_properties(profile_properties),
      coordinate_extractor(node_based_graph, compressed_edge_container, node_info_list)
{
}

bool RoundaboutHandler::canProcess(const NodeID from_nid,
                                   const EdgeID via_eid,
                                   const Intersection &intersection) const
{
    const auto flags = getRoundaboutFlags(from_nid, via_eid, intersection);
    if (!flags.on_roundabout && !flags.can_enter)
        return false;

    const auto roundabout_type = getRoundaboutType(node_based_graph.GetTarget(via_eid));
    return roundabout_type != RoundaboutType::None;
}

Intersection RoundaboutHandler::
operator()(const NodeID from_nid, const EdgeID via_eid, Intersection intersection) const
{
    invalidateExitAgainstDirection(from_nid, via_eid, intersection);
    const auto flags = getRoundaboutFlags(from_nid, via_eid, intersection);
    const auto roundabout_type = getRoundaboutType(node_based_graph.GetTarget(via_eid));
    // find the radius of the roundabout
    return handleRoundabouts(roundabout_type,
                             via_eid,
                             flags.on_roundabout,
                             flags.can_exit_separately,
                             std::move(intersection));
}

detail::RoundaboutFlags RoundaboutHandler::getRoundaboutFlags(
    const NodeID from_nid, const EdgeID via_eid, const Intersection &intersection) const
{
    const auto &in_edge_data = node_based_graph.GetEdgeData(via_eid);
    bool on_roundabout = in_edge_data.roundabout;
    bool can_enter_roundabout = false;
    bool can_exit_roundabout_separately = false;

    const bool lhs = profile_properties.left_hand_driving;
    const int step = lhs ? -1 : 1;
    for (std::size_t cnt = 0, idx = lhs ? intersection.size() - 1 : 0; cnt < intersection.size();
         ++cnt, idx += step)
    {
        const auto &road = intersection[idx];
        const auto &edge_data = node_based_graph.GetEdgeData(road.turn.eid);
        // only check actual outgoing edges
        if (edge_data.reversed || !road.entry_allowed)
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
    if (in_edge_data.roundabout)
        return;

    bool past_roundabout_angle = false;
    const bool lhs = profile_properties.left_hand_driving;
    const int step = lhs ? -1 : 1;
    for (std::size_t cnt = 0, idx = lhs ? intersection.size() - 1 : 0; cnt < intersection.size();
         ++cnt, idx += step)
    {
        auto &road = intersection[idx];
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
        if (!edge_data.roundabout && node_based_graph.GetTarget(road.turn.eid) != from_nid &&
            past_roundabout_angle)
        {
            road.entry_allowed = false;
        }
    }
}

// If we want to see a roundabout as a turn, the exits have to be distinct enough to be seen a
// dedicated turns. We are limiting it to four-way intersections with well distinct bearings.
// All entry/roads and exit roads have to be simple. Not segregated roads.
// Processing segregated roads would technically require an angle of the turn to be available
// in postprocessing since we correct the turn-angle in turn-generaion.
bool RoundaboutHandler::qualifiesAsRoundaboutIntersection(
    const std::unordered_set<NodeID> &roundabout_nodes) const
{
    // translate a node ID into its respective coordinate stored in the node_info_list
    const auto getCoordinate = [this](const NodeID node) {
        return util::Coordinate(node_info_list[node].lon, node_info_list[node].lat);
    };
    const bool has_limited_size = roundabout_nodes.size() <= 4;
    if (!has_limited_size)
        return false;

    const bool simple_exits =
        roundabout_nodes.end() ==
        std::find_if(roundabout_nodes.begin(), roundabout_nodes.end(), [this](const NodeID node) {
            return (node_based_graph.GetOutDegree(node) > 3);
        });

    if (!simple_exits)
        return false;

    // Find all exit bearings. Only if they are well distinct (at least 60 degrees between
    // them), we allow a roundabout turn

    const auto exit_bearings = [this, &roundabout_nodes, getCoordinate]() {
        std::vector<double> result;
        for (const auto node : roundabout_nodes)
        {
            // given the reverse edge and the forward edge on a roundabout, a simple entry/exit
            // can only contain a single further road
            for (const auto edge : node_based_graph.GetAdjacentEdgeRange(node))
            {
                const auto edge_data = node_based_graph.GetEdgeData(edge);
                if (edge_data.roundabout)
                    continue;

                // there is a single non-roundabout edge
                const auto src_coordinate = getCoordinate(node);

                const auto next_coordinate = coordinate_extractor.GetCoordinateAlongRoad(
                    node,
                    edge,
                    edge_data.reversed,
                    node_based_graph.GetTarget(edge),
                    getLaneCountAtIntersection(node, node_based_graph));

                result.push_back(
                    util::coordinate_calculation::bearing(src_coordinate, next_coordinate));
                break;
            }
        }
        std::sort(result.begin(), result.end());
        return result;
    }();

    const bool well_distinct_bearings = [](const std::vector<double> &bearings) {
        for (std::size_t bearing_index = 0; bearing_index < bearings.size(); ++bearing_index)
        {
            const double difference =
                std::abs(bearings[(bearing_index + 1) % bearings.size()] - bearings[bearing_index]);
            // we assume non-narrow turns as well distinct
            if (difference <= NARROW_TURN_ANGLE)
                return false;
        }
        return true;
    }(exit_bearings);

    return well_distinct_bearings;
}

RoundaboutType RoundaboutHandler::getRoundaboutType(const NodeID nid) const
{
    // translate a node ID into its respective coordinate stored in the node_info_list
    const auto getCoordinate = [this](const NodeID node) {
        return util::Coordinate(node_info_list[node].lon, node_info_list[node].lat);
    };

    std::unordered_set<unsigned> roundabout_name_ids;
    std::unordered_set<unsigned> connected_names;

    const auto getNextOnRoundabout = [this, &roundabout_name_ids, &connected_names](
        const NodeID node) {
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

                if (EMPTY_NAMEID != edge_data.name_id)
                {

                    const auto announce = [&](unsigned id) {
                        return util::guidance::requiresNameAnnounced(
                            name_table.GetNameForID(id),
                            name_table.GetRefForID(id),
                            name_table.GetNameForID(edge_data.name_id),
                            name_table.GetRefForID(edge_data.name_id),
                            street_name_suffix_table);
                    };

                    if (std::all_of(begin(roundabout_name_ids), end(roundabout_name_ids), announce))
                        roundabout_name_ids.insert(edge_data.name_id);
                }

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

    // this value is a hard abort to deal with potential self-loops
    const auto countRoundaboutFlags = [&](const NodeID at_node) {
        // FIXME: this would be nicer as boost::count_if, but our integer range does not support
        // these range based handlers
        std::size_t count = 0;
        for (const auto edge : node_based_graph.GetAdjacentEdgeRange(at_node))
        {
            const auto &edge_data = node_based_graph.GetEdgeData(edge);
            if (edge_data.roundabout)
                count++;
        }
        return count;
    };

    const auto getEdgeLength = [&](const NodeID source_node, EdgeID eid) {
        double length = 0.;
        auto last_coord = getCoordinate(source_node);
        const auto &edge_bucket = compressed_edge_container.GetBucketReference(eid);
        for (const auto &compressed_edge : edge_bucket)
        {
            const auto next_coord = getCoordinate(compressed_edge.node_id);
            length += util::coordinate_calculation::haversineDistance(last_coord, next_coord);
            last_coord = next_coord;
        }
        return length;
    };

    // the roundabout radius has to be the same for all locations we look at it from
    // to guarantee this, we search the full roundabout for its vertices
    // and select the three smallest ids
    std::unordered_set<NodeID> roundabout_nodes;
    double roundabout_length = 0.;

    NodeID last_node = nid;
    while (0 == roundabout_nodes.count(last_node))
    {
        // only count exits/entry locations
        if (node_based_graph.GetOutDegree(last_node) > 2)
            roundabout_nodes.insert(last_node);

        // detect invalid/complex roundabout taggings
        if (countRoundaboutFlags(last_node) != 2)
            return RoundaboutType::None;

        const auto eid = getNextOnRoundabout(last_node);

        if (eid == SPECIAL_EDGEID)
        {
            return RoundaboutType::None;
        }

        roundabout_length += getEdgeLength(last_node, eid);

        last_node = node_based_graph.GetTarget(eid);

        if (last_node == nid)
            break;
    }

    // a roundabout that cannot be entered or exited should not get here
    if (roundabout_nodes.size() == 0)
        return RoundaboutType::None;

    // More a traffic loop than anything else, currently treated as roundabout turn
    if (roundabout_nodes.size() == 1)
    {
        return RoundaboutType::RoundaboutIntersection;
    }

    const double radius = roundabout_length / (2 * M_PI);

    // Looks like a rotary: large roundabout with dedicated name
    // do we have a dedicated name for the rotary, if not its a roundabout
    // This function can theoretically fail if the roundabout name is partly
    // used with a reference and without. This will be fixed automatically
    // when we handle references separately or if the useage is more consistent
    const auto is_rotary = 1 == roundabout_name_ids.size() &&                          //
                           0 == connected_names.count(*roundabout_name_ids.begin()) && //
                           radius > MAX_ROUNDABOUT_RADIUS;

    if (is_rotary)
        return RoundaboutType::Rotary;

    // Looks like an intersection: four ways and turn angles are easy to distinguish
    const auto is_roundabout_intersection = qualifiesAsRoundaboutIntersection(roundabout_nodes) &&
                                            radius < MAX_ROUNDABOUT_INTERSECTION_RADIUS;

    if (is_roundabout_intersection)
        return RoundaboutType::RoundaboutIntersection;

    // Not a special case, just a normal roundabout
    return RoundaboutType::Roundabout;
}

Intersection RoundaboutHandler::handleRoundabouts(const RoundaboutType roundabout_type,
                                                  const EdgeID via_eid,
                                                  const bool on_roundabout,
                                                  const bool can_exit_roundabout_separately,
                                                  Intersection intersection) const
{
    // detect via radius (get via circle through three vertices)
    NodeID node_v = node_based_graph.GetTarget(via_eid);

    const bool lhs = profile_properties.left_hand_driving;
    const int step = lhs ? -1 : 1;

    if (on_roundabout)
    {
        // Shoule hopefully have only a single exit and continue
        // at least for cars. How about bikes?
        for (std::size_t cnt = 0, idx = lhs ? intersection.size() - 1 : 0;
             cnt < intersection.size();
             ++cnt, idx += step)
        {
            auto &road = intersection[idx];
            auto &turn = road.turn;
            const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
            if (out_data.roundabout)
            {
                // TODO can forks happen in roundabouts? E.g. required lane changes
                if (1 == node_based_graph.GetDirectedOutDegree(node_v))
                {
                    // No turn possible.
                    if (intersection.size() == 2)
                        turn.instruction = TurnInstruction::NO_TURN();
                    else
                    {
                        turn.instruction.type =
                            TurnType::Suppressed; // make sure to report intersection
                        turn.instruction.direction_modifier = getTurnDirection(turn.angle);
                    }
                }
                else
                {
                    turn.instruction = TurnInstruction::REMAIN_ROUNDABOUT(
                        roundabout_type, getTurnDirection(turn.angle));
                }
            }
            else
            {
                turn.instruction =
                    TurnInstruction::EXIT_ROUNDABOUT(roundabout_type, getTurnDirection(turn.angle));
            }
        }
        return intersection;
    }
    else
    {
        for (std::size_t cnt = 0, idx = lhs ? intersection.size() - 1 : 0;
             cnt < intersection.size();
             ++cnt, idx += step)
        {
            auto &road = intersection[idx];
            if (!road.entry_allowed)
                continue;
            auto &turn = road.turn;
            const auto &out_data = node_based_graph.GetEdgeData(turn.eid);
            if (out_data.roundabout)
            {
                if (can_exit_roundabout_separately)
                    turn.instruction = TurnInstruction::ENTER_ROUNDABOUT_AT_EXIT(
                        roundabout_type, getTurnDirection(turn.angle));
                else
                    turn.instruction = TurnInstruction::ENTER_ROUNDABOUT(
                        roundabout_type, getTurnDirection(turn.angle));
            }
            else
            {
                turn.instruction = TurnInstruction::ENTER_AND_EXIT_ROUNDABOUT(
                    roundabout_type, getTurnDirection(turn.angle));
            }
        }
    }
    return intersection;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
