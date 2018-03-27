#include "guidance/roundabout_handler.hpp"
#include "guidance/constants.hpp"

#include "util/assert.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/log.hpp"
#include "util/to_osm_link.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>

#include <boost/assert.hpp>

using osrm::guidance::getTurnDirection;

namespace osrm
{
namespace guidance
{

RoundaboutHandler::RoundaboutHandler(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const extractor::EdgeBasedNodeDataContainer &node_data_container,
    const std::vector<util::Coordinate> &coordinates,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const extractor::RestrictionMap &node_restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const extractor::TurnLanesIndexedArray &turn_lanes_data,
    const extractor::NameTable &name_table,
    const extractor::SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          node_data_container,
                          coordinates,
                          compressed_geometries,
                          node_restriction_map,
                          barrier_nodes,
                          turn_lanes_data,
                          name_table,
                          street_name_suffix_table),
      coordinate_extractor(node_based_graph, compressed_geometries, coordinates)
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
    const auto &in_edge_class = in_edge_data.flags;
    bool on_roundabout = in_edge_class.roundabout || in_edge_class.circular;
    bool can_enter_roundabout = false;
    bool can_exit_roundabout_separately = false;

    const bool lhs =
        node_data_container.GetAnnotation(in_edge_data.annotation_data).is_left_hand_driving;
    const int step = lhs ? -1 : 1;
    for (std::size_t cnt = 0, idx = lhs ? intersection.size() - 1 : 0; cnt < intersection.size();
         ++cnt, idx += step)
    {
        const auto &road = intersection[idx];
        const auto &edge = node_based_graph.GetEdgeData(road.eid);
        // only check actual outgoing edges
        if (edge.reversed || !road.entry_allowed)
            continue;

        if (edge.flags.roundabout || edge.flags.circular)
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
        else if (node_based_graph.GetTarget(road.eid) != from_nid && !can_enter_roundabout)
        {
            can_exit_roundabout_separately = true;
        }
    }
    return {on_roundabout, can_enter_roundabout, can_exit_roundabout_separately};
}

// If we want to see a roundabout as a turn, the exits have to be distinct enough to be seen a
// dedicated turns. We are limiting it to four-way intersections with well distinct bearings.
// All entry/roads and exit roads have to be simple. Not segregated roads.
// Processing segregated roads would technically require an angle of the turn to be available
// in postprocessing since we correct the turn-angle in turn-generaion.
bool RoundaboutHandler::qualifiesAsRoundaboutIntersection(
    const std::unordered_set<NodeID> &roundabout_nodes) const
{
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

    const auto exit_bearings = [this, &roundabout_nodes]() {
        std::vector<double> result;
        for (const auto node : roundabout_nodes)
        {
            // given the reverse edge and the forward edge on a roundabout, a simple entry/exit
            // can only contain a single further road
            for (const auto edge : node_based_graph.GetAdjacentEdgeRange(node))
            {
                const auto &edge_data = node_based_graph.GetEdgeData(edge);
                if (edge_data.flags.roundabout || edge_data.flags.circular)
                    continue;

                // there is a single non-roundabout edge
                const auto src_coordinate = node_coordinates[node];

                const auto edge_range = node_based_graph.GetAdjacentEdgeRange(node);
                const auto number_of_lanes_at_intersection = std::accumulate(
                    edge_range.begin(),
                    edge_range.end(),
                    std::uint8_t{0},
                    [this](const auto current_max, const auto current_eid) {
                        return std::max(current_max,
                                        node_based_graph.GetEdgeData(current_eid)
                                            .flags.road_classification.GetNumberOfLanes());
                    });

                const auto next_coordinate =
                    coordinate_extractor.GetCoordinateAlongRoad(node,
                                                                edge,
                                                                false,
                                                                node_based_graph.GetTarget(edge),
                                                                number_of_lanes_at_intersection);

                result.push_back(
                    util::coordinate_calculation::bearing(src_coordinate, next_coordinate));

                // OSM data sometimes contains duplicated nodes with identical coordinates, or
                // because of coordinate precision rounding, end up at the same coordinate.
                // It's impossible to calculate a bearing between these, so we log a warning
                // that the data should be checked.
                // The bearing calculation should return 0 in these cases, which may not be correct,
                // but is at least not random.
                if (src_coordinate == next_coordinate)
                {
                    util::Log(logDEBUG) << "Zero length segment at " << toOSMLink(next_coordinate)
                                        << " could cause invalid roundabout exit bearings";
                    BOOST_ASSERT(std::abs(result.back()) <= 0.1);
                }

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
    std::unordered_set<unsigned> roundabout_name_ids;
    std::unordered_set<unsigned> connected_names;

    const auto getNextOnRoundabout = [this, &roundabout_name_ids, &connected_names](
        const NodeID node, const bool roundabout, const bool circular) {
        BOOST_ASSERT(roundabout != circular);
        EdgeID continue_edge = SPECIAL_EDGEID;
        for (const auto edge_id : node_based_graph.GetAdjacentEdgeRange(node))
        {
            const auto &edge = node_based_graph.GetEdgeData(edge_id);
            const auto &edge_data = node_data_container.GetAnnotation(edge.annotation_data);
            if (!edge.reversed && (edge.flags.circular == circular) &&
                (edge.flags.roundabout == roundabout))
            {
                if (SPECIAL_EDGEID != continue_edge)
                {
                    // fork in roundabout
                    return SPECIAL_EDGEID;
                }

                const auto &edge_name_empty = name_table.GetNameForID(edge_data.name_id).empty();
                if (!edge_name_empty)
                {

                    const auto announce = [&](unsigned id) {
                        return util::guidance::requiresNameAnnounced(
                            id, edge_data.name_id, name_table, street_name_suffix_table);
                    };

                    if (std::all_of(begin(roundabout_name_ids), end(roundabout_name_ids), announce))
                        roundabout_name_ids.insert(edge_data.name_id);
                }
                continue_edge = edge_id;
            }
            else if (!edge.flags.roundabout && !edge.flags.circular)
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
            if (edge_data.flags.roundabout || edge_data.flags.circular)
                count++;
        }
        return count;
    };

    const auto getEdgeLength = [&](const NodeID source_node, EdgeID eid) {
        double length = 0.;
        auto last_coord = node_coordinates[source_node];
        const auto &edge_bucket = compressed_geometries.GetBucketReference(eid);
        for (const auto &compressed_edge : edge_bucket)
        {
            const auto next_coord = node_coordinates[compressed_edge.node_id];
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

    // cannot be find_if, as long as adjacent edge range does not work.
    bool roundabout = false, circular = false;
    for (const auto eid : node_based_graph.GetAdjacentEdgeRange(nid))
    {
        const auto data = node_based_graph.GetEdgeData(eid).flags;
        roundabout |= data.roundabout;
        circular |= data.circular;
    }

    if (roundabout == circular)
        return RoundaboutType::None;

    while (0 == roundabout_nodes.count(last_node))
    {
        // only count exits/entry locations
        if (node_based_graph.GetOutDegree(last_node) > 2)
            roundabout_nodes.insert(last_node);

        // detect invalid/complex roundabout taggings
        if (countRoundaboutFlags(last_node) != 2)
            return RoundaboutType::None;

        const auto eid = getNextOnRoundabout(last_node, roundabout, circular);

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
    if (roundabout_nodes.size() == 1 && roundabout)
    {
        return RoundaboutType::RoundaboutIntersection;
    }

    const double radius = roundabout_length / (2 * M_PI);

    // Looks like a rotary: large roundabout with dedicated name
    // do we have a dedicated name for the rotary, if not its a roundabout
    // This function can theoretically fail if the roundabout name is partly
    // used with a reference and without. This will be fixed automatically
    // when we handle references separately or if the useage is more consistent
    const auto is_rotary = (1 == roundabout_name_ids.size()) &&
                           (circular ||                                                    //
                            ((0 == connected_names.count(*roundabout_name_ids.begin())) && //
                             (radius > MAX_ROUNDABOUT_RADIUS)));

    if (is_rotary)
        return RoundaboutType::Rotary;

    // circular intersections need to be rotaries
    if (circular)
        return RoundaboutType::None;

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
    NodeID node_at_center_of_intersection = node_based_graph.GetTarget(via_eid);
    const auto &in_edge_data = node_based_graph.GetEdgeData(via_eid);

    const bool lhs =
        node_data_container.GetAnnotation(in_edge_data.annotation_data).is_left_hand_driving;
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
            auto &turn = road;
            const auto &out_data = node_based_graph.GetEdgeData(road.eid).flags;
            ;
            if (out_data.roundabout || out_data.circular)
            {
                // TODO can forks happen in roundabouts? E.g. required lane changes
                if (1 == node_based_graph.GetDirectedOutDegree(node_at_center_of_intersection))
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
                    // Check if there is a non-service exit
                    const auto has_non_ignorable_exit = [&]() {
                        for (const auto eid :
                             node_based_graph.GetAdjacentEdgeRange(node_at_center_of_intersection))
                        {
                            const auto &leaving_edge = node_based_graph.GetEdgeData(eid);
                            if (!leaving_edge.reversed && !leaving_edge.flags.roundabout &&
                                !leaving_edge.flags.circular &&
                                !leaving_edge.flags.road_classification.IsLowPriorityRoadClass())
                                return true;
                        }
                        return false;
                    };

                    // Count normal exits and service roads, if the roundabout is a service road
                    // itself
                    if (out_data.road_classification.IsLowPriorityRoadClass() ||
                        has_non_ignorable_exit())
                    {
                        turn.instruction = TurnInstruction::REMAIN_ROUNDABOUT(
                            roundabout_type, getTurnDirection(turn.angle));
                    }
                    else
                    { // Suppress exit instructions from normal roundabouts to service roads
                        turn.instruction = {TurnType::Suppressed, getTurnDirection(turn.angle)};
                    }
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
        bool crossing_roundabout = false;
        for (std::size_t cnt = 0, idx = lhs ? intersection.size() - 1 : 0;
             cnt < intersection.size();
             ++cnt, idx += step)
        {
            auto &turn = intersection[idx];
            const auto &out_data = node_based_graph.GetEdgeData(turn.eid).flags;

            // A roundabout consists of exactly two roads at an intersection. by toggeling this
            // flag, we can switch between roads crossing the roundabout and roads that are on the
            // same side as via_eid.
            if (out_data.roundabout || out_data.circular)
                crossing_roundabout = !crossing_roundabout;

            if (!turn.entry_allowed)
                continue;

            if (out_data.roundabout || out_data.circular)
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
                // Distinguish between throughabouts and entering a roundabout to directly exit: In
                // case of a throughabout, both enter and exit do not show roundabout tags (as we
                // already have checked, when arriving here) and the enter/exit are nearly straight
                // and on different sides of the roundabouts
                if (util::angularDeviation(turn.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE &&
                    crossing_roundabout)
                {
                    turn.instruction =
                        getInstructionForObvious(intersection.size(),
                                                 via_eid,
                                                 isThroughStreet(idx,
                                                                 intersection,
                                                                 node_based_graph,
                                                                 node_data_container,
                                                                 name_table,
                                                                 street_name_suffix_table),
                                                 turn);
                }
                else
                {
                    turn.instruction = TurnInstruction::ENTER_AND_EXIT_ROUNDABOUT(
                        roundabout_type, getTurnDirection(turn.angle));
                }
            }
        }
    }
    return intersection;
}

} // namespace guidance
} // namespace osrm
