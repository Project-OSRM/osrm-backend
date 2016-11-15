#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/guidance/toolkit.hpp"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iterator>
#include <limits>
#include <unordered_set>
#include <utility>

#include <boost/range/algorithm/count_if.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace
{
const constexpr bool USE_LOW_PRECISION_MODE = true;
// the inverse of use low precision mode
const constexpr bool USE_HIGH_PRECISION_MODE = !USE_LOW_PRECISION_MODE;
}

IntersectionGenerator::IntersectionGenerator(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const RestrictionMap &restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const std::vector<QueryNode> &node_info_list,
    const CompressedEdgeContainer &compressed_edge_container)
    : node_based_graph(node_based_graph), restriction_map(restriction_map),
      barrier_nodes(barrier_nodes), node_info_list(node_info_list),
      coordinate_extractor(node_based_graph, compressed_edge_container, node_info_list)
{
}

Intersection IntersectionGenerator::operator()(const NodeID from_node, const EdgeID via_eid) const
{
    return GetConnectedRoads(from_node, via_eid, USE_HIGH_PRECISION_MODE);
}

//                                               a
//                                               |
//                                               |
//                                               v
// For an intersection from_node --via_eid--> turn_node ----> c
//                                               ^
//                                               |
//                                               |
//                                               b
// This functions returns _all_ turns as if the graph was undirected.
// That means we not only get (from_node, turn_node, c) in the above example
// but also (from_node, turn_node, a), (from_node, turn_node, b). These turns are
// marked as invalid and only needed for intersection classification.
Intersection IntersectionGenerator::GetConnectedRoads(const NodeID from_node,
                                                      const EdgeID via_eid,
                                                      const bool use_low_precision_angles) const
{
    Intersection intersection;
    const NodeID turn_node = node_based_graph.GetTarget(via_eid);
    // reserve enough items (+ the possibly missing u-turn edge)
    intersection.reserve(node_based_graph.GetOutDegree(turn_node) + 1);
    const NodeID only_restriction_to_node = [&]() {
        // If only restrictions refer to invalid ways somewhere far away, we rather ignore the
        // restriction than to not route over the intersection at all.
        const auto only_restriction_to_node =
            restriction_map.CheckForEmanatingIsOnlyTurn(from_node, turn_node);
        if (only_restriction_to_node != SPECIAL_NODEID)
        {
            // check if we can find an edge in the edge-rage
            for (const auto onto_edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
                if (only_restriction_to_node == node_based_graph.GetTarget(onto_edge))
                    return only_restriction_to_node;
        }
        // Ignore broken only restrictions.
        return SPECIAL_NODEID;
    }();
    const bool is_barrier_node = barrier_nodes.find(turn_node) != barrier_nodes.end();

    bool has_uturn_edge = false;
    bool uturn_could_be_valid = false;
    const util::Coordinate turn_coordinate = node_info_list[turn_node];

    const auto intersection_lanes = getLaneCountAtIntersection(turn_node, node_based_graph);

    const auto extract_coordinate = [&](const NodeID from_node,
                                        const EdgeID via_eid,
                                        const bool traversed_in_reverse,
                                        const NodeID to_node) {
        return use_low_precision_angles
                   ? coordinate_extractor.GetCoordinateCloseToTurn(
                         from_node, via_eid, traversed_in_reverse, to_node)
                   : coordinate_extractor.GetCoordinateAlongRoad(
                         from_node, via_eid, traversed_in_reverse, to_node, intersection_lanes);
    };

    // The first coordinate (the origin) can depend on the number of lanes turning onto,
    // just as the target coordinate can. Here we compute the corrected coordinate for the
    // incoming edge

    // to compute the length along the path
    const auto in_segment_length = [&]() {
        const auto in_coordinates =
            coordinate_extractor.GetCoordinatesAlongRoad(from_node, via_eid, INVERT, turn_node);
        return util::coordinate_calculation::getLength(
            in_coordinates, util::coordinate_calculation::haversineDistance);
    }();
    const auto first_coordinate = extract_coordinate(from_node, via_eid, INVERT, turn_node);

    for (const EdgeID onto_edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
    {
        BOOST_ASSERT(onto_edge != SPECIAL_EDGEID);
        const NodeID to_node = node_based_graph.GetTarget(onto_edge);
        const auto &onto_data = node_based_graph.GetEdgeData(onto_edge);

        bool turn_is_valid =
            // reverse edges are never valid turns because the resulting turn would look like this:
            // from_node --via_edge--> turn_node <--onto_edge-- to_node
            // however we need this for capture intersection shape for incoming one-ways
            !onto_data.reversed &&
            // we are not turning over a barrier
            (!is_barrier_node || from_node == to_node) &&
            // We are at an only_-restriction but not at the right turn.
            (only_restriction_to_node == SPECIAL_NODEID || to_node == only_restriction_to_node) &&
            // the turn is not restricted
            !restriction_map.CheckIfTurnIsRestricted(from_node, turn_node, to_node);

        double bearing = 0., out_segment_length = 0., angle = 0.;
        if (from_node == to_node)
        {
            bearing = util::coordinate_calculation::bearing(turn_coordinate, first_coordinate);
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
            out_segment_length = in_segment_length;
            BOOST_ASSERT(angle >= 0. && angle < std::numeric_limits<double>::epsilon());
        }
        else
        {
            // the default distance we lookahead on a road. This distance prevents small mapping
            // errors to impact the turn angles.
            {
                // segment of out segment
                const auto out_coordinates = coordinate_extractor.GetCoordinatesAlongRoad(
                    turn_node, onto_edge, !INVERT, to_node);
                out_segment_length = util::coordinate_calculation::getLength(
                    out_coordinates, util::coordinate_calculation::haversineDistance);
            }
            const auto third_coordinate = extract_coordinate(turn_node, onto_edge, !INVERT, to_node);

            angle = util::coordinate_calculation::computeAngle(
                first_coordinate, turn_coordinate, third_coordinate);

            bearing = util::coordinate_calculation::bearing(turn_coordinate, third_coordinate);

            if (std::abs(angle) < std::numeric_limits<double>::epsilon())
                has_uturn_edge = true;
        }
        intersection.push_back(
            ConnectedRoad(TurnOperation{onto_edge,
                                        angle,
                                        bearing,
                                        {TurnType::Invalid, DirectionModifier::UTurn},
                                        INVALID_LANE_DATAID},
                          turn_is_valid,
                          out_segment_length));
    }

    // We hit the case of a street leading into nothing-ness. Since the code here assumes
    // that this
    // will never happen we add an artificial invalid uturn in this case.
    if (!has_uturn_edge)
    {
        const double bearing =
            util::coordinate_calculation::bearing(turn_coordinate, first_coordinate);

        intersection.push_back({TurnOperation{via_eid,
                                              0.,
                                              bearing,
                                              {TurnType::Invalid, DirectionModifier::UTurn},
                                              INVALID_LANE_DATAID},
                                false,
                                in_segment_length});
    }

    std::sort(std::begin(intersection),
              std::end(intersection),
              std::mem_fn(&ConnectedRoad::compareByAngle));

    BOOST_ASSERT(intersection[0].angle >= 0. &&
                 intersection[0].angle < std::numeric_limits<double>::epsilon());

    const auto valid_count =
        boost::count_if(intersection, [](const ConnectedRoad &road) { return road.entry_allowed; });
    if (0 == valid_count && uturn_could_be_valid)
    {
        // after intersections sorting by angles, find the u-turn with (from_node ==
        // to_node)
        // that was inserted together with setting uturn_could_be_valid flag
        std::size_t self_u_turn = 0;
        while (self_u_turn < intersection.size() &&
               intersection[self_u_turn].angle < std::numeric_limits<double>::epsilon() &&
               from_node != node_based_graph.GetTarget(intersection[self_u_turn].eid))
        {
            ++self_u_turn;
        }

        BOOST_ASSERT(from_node == node_based_graph.GetTarget(intersection[self_u_turn].eid));
        intersection[self_u_turn].entry_allowed = true;
    }
    return intersection;
}

Intersection
IntersectionGenerator::GetActualNextIntersection(const NodeID starting_node,
                                                 const EdgeID via_edge,
                                                 NodeID *resulting_from_node = nullptr,
                                                 EdgeID *resulting_via_edge = nullptr) const
{
    NodeID query_node = starting_node;
    EdgeID query_edge = via_edge;

    const auto get_next_edge = [this](const NodeID from, const EdgeID via) {
        const NodeID new_node = node_based_graph.GetTarget(via);
        BOOST_ASSERT(node_based_graph.GetOutDegree(new_node) == 2);
        const EdgeID begin_edges_new_node = node_based_graph.BeginEdges(new_node);
        return (node_based_graph.GetTarget(begin_edges_new_node) == from) ? begin_edges_new_node + 1
                                                                          : begin_edges_new_node;
    };

    std::unordered_set<NodeID> visited_nodes;
    // skip trivial nodes without generating the intersection in between, stop at the very first
    // intersection of degree > 2
    while (0 == visited_nodes.count(query_node) &&
           2 == node_based_graph.GetOutDegree(node_based_graph.GetTarget(query_edge)))
    {
        visited_nodes.insert(query_node);
        const auto next_node = node_based_graph.GetTarget(query_edge);
        const auto next_edge = get_next_edge(query_node, query_edge);
        if (!node_based_graph.GetEdgeData(query_edge)
                 .IsCompatibleTo(node_based_graph.GetEdgeData(next_edge)) ||
            node_based_graph.GetTarget(next_edge) == starting_node)
            break;

        query_node = next_node;
        query_edge = next_edge;
    }

    if (resulting_from_node)
        *resulting_from_node = query_node;
    if (resulting_via_edge)
        *resulting_via_edge = query_edge;

    return GetConnectedRoads(query_node, query_edge);
}

const CoordinateExtractor &IntersectionGenerator::GetCoordinateExtractor() const
{
    return coordinate_extractor;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
