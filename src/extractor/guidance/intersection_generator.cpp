#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/guidance/toolkit.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <utility>

#include <boost/range/algorithm/count_if.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

IntersectionGenerator::IntersectionGenerator(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const RestrictionMap &restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const std::vector<QueryNode> &node_info_list,
    const CompressedEdgeContainer &compressed_edge_container)
    : node_based_graph(node_based_graph), restriction_map(restriction_map),
      barrier_nodes(barrier_nodes), node_info_list(node_info_list),
      compressed_edge_container(compressed_edge_container)
{
}

Intersection IntersectionGenerator::operator()(const NodeID from_node, const EdgeID via_eid) const
{
    auto intersection = getConnectedRoads(from_node, via_eid);
    return adjustForJoiningRoads(node_based_graph.GetTarget(via_eid),
                                 mergeSegregatedRoads(std::move(intersection)));
}

//                                               a
//                                               |
//                                               |
//                                               v
// For an intersection from_node --via_edi--> turn_node ----> c
//                                               ^
//                                               |
//                                               |
//                                               b
// This functions returns _all_ turns as if the graph was undirected.
// That means we not only get (from_node, turn_node, c) in the above example
// but also (from_node, turn_node, a), (from_node, turn_node, b). These turns are
// marked as invalid and only needed for intersection classification.
Intersection IntersectionGenerator::getConnectedRoads(const NodeID from_node,
                                                      const EdgeID via_eid) const
{
    Intersection intersection;
    const NodeID turn_node = node_based_graph.GetTarget(via_eid);
    const NodeID only_restriction_to_node =
        restriction_map.CheckForEmanatingIsOnlyTurn(from_node, turn_node);
    const bool is_barrier_node = barrier_nodes.find(turn_node) != barrier_nodes.end();

    bool has_uturn_edge = false;
    bool uturn_could_be_valid = false;
    for (const EdgeID onto_edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
    {
        BOOST_ASSERT(onto_edge != SPECIAL_EDGEID);
        const NodeID to_node = node_based_graph.GetTarget(onto_edge);

        bool turn_is_valid =
            // reverse edges are never valid turns because the resulting turn would look like this:
            // from_node --via_edge--> turn_node <--onto_edge-- to_node
            // however we need this for capture intersection shape for incoming one-ways
            !node_based_graph.GetEdgeData(onto_edge).reversed &&
            // we are not turning over a barrier
            (!is_barrier_node || from_node == to_node) &&
            // We are at an only_-restriction but not at the right turn.
            (only_restriction_to_node == SPECIAL_NODEID || to_node == only_restriction_to_node) &&
            // the turn is not restricted
            !restriction_map.CheckIfTurnIsRestricted(from_node, turn_node, to_node);

        auto angle = 0.;
        if (from_node == to_node)
        {
            uturn_could_be_valid = turn_is_valid;
            if (turn_is_valid && !is_barrier_node)
            {
                // we only add u-turns for dead-end streets.
                if (node_based_graph.GetOutDegree(turn_node) > 1)
                {
                    auto number_of_emmiting_bidirectional_edges = 0;
                    for (auto edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
                    {
                        auto target = node_based_graph.GetTarget(edge);
                        auto reverse_edge = node_based_graph.FindEdge(target, turn_node);
                        BOOST_ASSERT(reverse_edge != SPECIAL_EDGEID);
                        if (!node_based_graph.GetEdgeData(reverse_edge).reversed)
                        {
                            ++number_of_emmiting_bidirectional_edges;
                        }
                    }
                    // is a dead-end, only possible road is to go back
                    turn_is_valid = number_of_emmiting_bidirectional_edges <= 1;
                }
            }
            has_uturn_edge = true;
            BOOST_ASSERT(angle >= 0. && angle < std::numeric_limits<double>::epsilon());
        }
        else
        {
            // unpack first node of second segment if packed
            const auto first_coordinate = getRepresentativeCoordinate(
                from_node, turn_node, via_eid, INVERT, compressed_edge_container, node_info_list);
            const auto third_coordinate = getRepresentativeCoordinate(
                turn_node, to_node, onto_edge, !INVERT, compressed_edge_container, node_info_list);
            angle = util::coordinate_calculation::computeAngle(
                first_coordinate, node_info_list[turn_node], third_coordinate);
            if (std::abs(angle) < std::numeric_limits<double>::epsilon())
                has_uturn_edge = true;
        }

        intersection.push_back(
            ConnectedRoad(TurnOperation{onto_edge,
                                        angle,
                                        {TurnType::Invalid, DirectionModifier::UTurn},
                                        INVALID_LANE_DATAID},
                          turn_is_valid));
    }

    // We hit the case of a street leading into nothing-ness. Since the code here assumes that this
    // will never happen we add an artificial invalid uturn in this case.
    if (!has_uturn_edge)
    {
        intersection.push_back(
            {TurnOperation{
                 via_eid, 0., {TurnType::Invalid, DirectionModifier::UTurn}, INVALID_LANE_DATAID},
             false});
    }

    const auto ByAngle = [](const ConnectedRoad &first, const ConnectedRoad second) {
        return first.turn.angle < second.turn.angle;
    };
    std::sort(std::begin(intersection), std::end(intersection), ByAngle);

    BOOST_ASSERT(intersection[0].turn.angle >= 0. &&
                 intersection[0].turn.angle < std::numeric_limits<double>::epsilon());

    const auto valid_count =
        boost::count_if(intersection, [](const ConnectedRoad &road) { return road.entry_allowed; });
    if (0 == valid_count && uturn_could_be_valid)
    {
        // after intersections sorting by angles, find the u-turn with (from_node == to_node)
        // that was inserted together with setting uturn_could_be_valid flag
        std::size_t self_u_turn = 0;
        while (self_u_turn < intersection.size() &&
               intersection[self_u_turn].turn.angle < std::numeric_limits<double>::epsilon() &&
               from_node != node_based_graph.GetTarget(intersection[self_u_turn].turn.eid))
        {
            ++self_u_turn;
        }

        BOOST_ASSERT(from_node == node_based_graph.GetTarget(intersection[self_u_turn].turn.eid));
        intersection[self_u_turn].entry_allowed = true;
    }

    return intersection;
}

bool IntersectionGenerator::canMerge(const Intersection &intersection,
                                     std::size_t first_index,
                                     std::size_t second_index) const
{
    const auto &first_data = node_based_graph.GetEdgeData(intersection[first_index].turn.eid);
    const auto &second_data = node_based_graph.GetEdgeData(intersection[second_index].turn.eid);

    // only merge named ids
    if (first_data.name_id == EMPTY_NAMEID)
        return false;

    // need to be same name
    if (first_data.name_id != second_data.name_id)
        return false;

    // compatibility is required
    if (first_data.travel_mode != second_data.travel_mode)
        return false;
    if (first_data.road_classification != second_data.road_classification)
        return false;

    // may not be on a roundabout
    if (first_data.roundabout || second_data.roundabout)
        return false;

    // exactly one of them has to be reversed
    if (first_data.reversed == second_data.reversed)
        return false;

    // mergeable if the angle is not too big
    return angularDeviation(intersection[first_index].turn.angle,
                            intersection[second_index].turn.angle) < 60;
}

/*
 * Segregated Roads often merge onto a single intersection.
 * While technically representing different roads, they are
 * often looked at as a single road.
 * Due to the merging, turn Angles seem off, wenn we compute them from the
 * initial positions.
 *
 *         b<b<b<b(1)<b<b<b
 * aaaaa-b
 *         b>b>b>b(2)>b>b>b
 *
 * Would be seen as a slight turn going fro a to (2). A Sharp turn going from
 * (1) to (2).
 *
 * In cases like these, we megre this segregated roads into a single road to
 * end up with a case like:
 *
 * aaaaa-bbbbbb
 *
 * for the turn representation.
 * Anything containing the first u-turn in a merge affects all other angles
 * and is handled separately from all others.
 */
Intersection IntersectionGenerator::mergeSegregatedRoads(Intersection intersection) const
{
    const auto getRight = [&](std::size_t index) {
        return (index + intersection.size() - 1) % intersection.size();
    };

    const auto merge = [](const ConnectedRoad &first,
                          const ConnectedRoad &second) -> ConnectedRoad {
        if (!first.entry_allowed)
        {
            ConnectedRoad result = second;
            result.turn.angle = (first.turn.angle + second.turn.angle) / 2;
            if (first.turn.angle - second.turn.angle > 180)
                result.turn.angle += 180;
            if (result.turn.angle > 360)
                result.turn.angle -= 360;

            return result;
        }
        else
        {
            BOOST_ASSERT(!second.entry_allowed);
            ConnectedRoad result = first;
            result.turn.angle = (first.turn.angle + second.turn.angle) / 2;

            if (first.turn.angle - second.turn.angle > 180)
                result.turn.angle += 180;
            if (result.turn.angle > 360)
                result.turn.angle -= 360;

            return result;
        }
    };
    if (intersection.size() <= 1)
        return intersection;

    const bool is_connected_to_roundabout = [this, &intersection]() {
        for (const auto &road : intersection)
        {
            if (node_based_graph.GetEdgeData(road.turn.eid).roundabout)
                return true;
        }
        return false;
    }();

    // check for merges including the basic u-turn
    // these result in an adjustment of all other angles. This is due to how these angles are
    // perceived. Considering the following example:
    //
    //   c   b
    //     Y
    //     a
    //
    // coming from a to b (given a road that splits at the fork into two one-ways), the turn is not
    // considered as a turn but rather as going straight.
    // Now if we look at the situation merging:
    //
    //  a     b
    //    \ /
    // e - + - d
    //     |
    //     c
    //
    // With a,b representing the same road, the intersection itself represents a classif for way
    // intersection so we handle it like
    //
    //   (a),b
    //      |
    // e -  + - d
    //      |
    //      c
    //
    // To be able to consider this adjusted representation down the line, we merge some roads.
    // If the merge occurs at the u-turn edge, we need to adjust all angles, though, since they are
    // with respect to the now changed perceived location of a. If we move (a) to the left, we add
    // the difference to all angles. Otherwise we subtract it.
    bool merged_first = false;
    // these result in an adjustment of all other angles
    if (canMerge(intersection, 0, intersection.size() - 1))
    {
        merged_first = true;
        // moving `a` to the left
        const double correction_factor =
            (360 - intersection[intersection.size() - 1].turn.angle) / 2;
        for (std::size_t i = 1; i + 1 < intersection.size(); ++i)
            intersection[i].turn.angle += correction_factor;

        // FIXME if we have a left-sided country, we need to switch this off and enable it below
        intersection[0] = merge(intersection.front(), intersection.back());
        intersection[0].turn.angle = 0;

        intersection.pop_back();
    }
    else if (canMerge(intersection, 0, 1))
    {
        merged_first = true;
        // moving `a` to the right
        const double correction_factor = (intersection[1].turn.angle) / 2;
        for (std::size_t i = 2; i < intersection.size(); ++i)
            intersection[i].turn.angle -= correction_factor;
        intersection[0] = merge(intersection[0], intersection[1]);
        intersection[0].turn.angle = 0;
        intersection.erase(intersection.begin() + 1);
    }

    if (merged_first && is_connected_to_roundabout)
    {
        /*
         * We are merging a u-turn against the direction of a roundabout
         *
         *     -----------> roundabout
         *        /    \
         *     out      in
         *
         * These cases have to be disabled, even if they are not forbidden specifically by a
         * relation
         */
        intersection[0].entry_allowed = false;
    }

    // a merge including the first u-turn requres an adjustment of the turn angles
    // therefore these are handled prior to this step
    for (std::size_t index = 2; index < intersection.size(); ++index)
    {
        if (canMerge(intersection, index, getRight(index)))
        {
            intersection[getRight(index)] =
                merge(intersection[getRight(index)], intersection[index]);
            intersection.erase(intersection.begin() + index);
            --index;
        }
    }

    const auto ByAngle = [](const ConnectedRoad &first, const ConnectedRoad second) {
        return first.turn.angle < second.turn.angle;
    };
    std::sort(std::begin(intersection), std::end(intersection), ByAngle);
    return intersection;
}

// OSM can have some very steep angles for joining roads. Considering the following intersection:
//        x
//        |__________c
//       /
// a ---d
//       \ __________b
//
// with c->d as a oneway
// and d->b as a oneway, the turn von x->d is actually a turn from x->a. So when looking at the
// intersection coming from x, we want to interpret the situation as
//           x
// a __ d __ |__________c
//      |
//      |_______________b
//
// Where we see the turn to `d` as a right turn, rather than going straight.
// We do this by adjusting the local turn angle at `x` to turn onto `d` to be reflective of this
// situation.
Intersection IntersectionGenerator::adjustForJoiningRoads(const NodeID node_at_intersection,
                                                          Intersection intersection) const
{
    // nothing to do for dead ends
    if ( intersection.size() <= 1)
        return intersection;

    for (auto &road : intersection)
    {
        // to find out about the above situation, we need to look at the next intersection (at d in
        // the example). If the initial road can be merged to the left/right, we are about to adjust
        // the angle.
        const auto next_intersection_along_road =
            getConnectedRoads(node_at_intersection, road.turn.eid);
        if (next_intersection_along_road.size() <= 1)
            continue;

        const auto adjustAngle = [](double angle, double offset) {
            angle += offset;
            if (angle > 360)
                return angle - 360.;
            else if (angle < 0)
                return angle + 360.;
            return angle;
        };

        if (canMerge(next_intersection_along_road, 0, 1))
        {
            const auto offset = 0.5 * angularDeviation(next_intersection_along_road[0].turn.angle,
                                                       next_intersection_along_road[1].turn.angle);
            // at the target intersection, we merge to the right, so we need to shift the current
            // angle to the left
            road.turn.angle = adjustAngle(road.turn.angle, offset);
        }
        else if (canMerge(next_intersection_along_road, 0, next_intersection_along_road.size() - 1))
        {
            const auto offset =
                0.5 * angularDeviation(
                          next_intersection_along_road[0].turn.angle,
                          next_intersection_along_road[next_intersection_along_road.size() - 1]
                              .turn.angle);
            // at the target intersection, we merge to the left, so we need to shift the current
            // angle to the right
            road.turn.angle = adjustAngle(road.turn.angle, -offset);
        }
    }
    std::cout << std::flush;
    return intersection;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
