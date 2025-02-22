#include "extractor/intersection/mergable_road_detector.hpp"
#include "extractor/intersection/intersection_analysis.hpp"
#include "extractor/intersection/node_based_graph_walker.hpp"
#include "extractor/name_table.hpp"
#include "extractor/query_node.hpp"
#include "extractor/suffix_table.hpp"
#include "guidance/constants.hpp"

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"

using osrm::util::angularDeviation;

namespace osrm::extractor::intersection
{

namespace
{
// check a connected road for equality of a name
// returns 'true' if no equality because this is used as a filter elsewhere, i.e. filter if fn
// returns 'true'
inline auto makeCheckRoadForName(const NameID name_id,
                                 const util::NodeBasedDynamicGraph &node_based_graph,
                                 const EdgeBasedNodeDataContainer &node_data_container,
                                 const NameTable &name_table,
                                 const SuffixTable &suffix_table)
{
    return [name_id, &node_based_graph, &node_data_container, &name_table, &suffix_table](
               const MergableRoadDetector::MergableRoadData &road)
    {
        // since we filter here, we don't want any other name than the one we are looking for
        const auto road_name_id =
            node_data_container
                .GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
                .name_id;
        const auto road_name_empty = name_table.GetNameForID(road_name_id).empty();
        const auto in_name_empty = name_table.GetNameForID(name_id).empty();
        if (in_name_empty || road_name_empty)
            return true;
        const auto requires_announcement =
            util::guidance::requiresNameAnnounced(
                name_id, road_name_id, name_table, suffix_table) ||
            util::guidance::requiresNameAnnounced(road_name_id, name_id, name_table, suffix_table);

        return requires_announcement;
    };
}
} // namespace

MergableRoadDetector::MergableRoadDetector(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const EdgeBasedNodeDataContainer &node_data_container,
    const std::vector<util::Coordinate> &node_coordinates,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const RestrictionMap &node_restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const extractor::TurnLanesIndexedArray &turn_lanes_data,
    const NameTable &name_table,
    const SuffixTable &street_name_suffix_table)
    : node_based_graph(node_based_graph), node_data_container(node_data_container),
      node_coordinates(node_coordinates), compressed_geometries(compressed_geometries),
      node_restriction_map(node_restriction_map), barrier_nodes(barrier_nodes),
      turn_lanes_data(turn_lanes_data), name_table(name_table),
      street_name_suffix_table(street_name_suffix_table),
      coordinate_extractor(node_based_graph, compressed_geometries, node_coordinates)
{
}

bool MergableRoadDetector::CanMergeRoad(const NodeID intersection_node,
                                        const IntersectionEdgeGeometry &lhs,
                                        const IntersectionEdgeGeometry &rhs) const
{
    // roads should be somewhat close
    if (angularDeviation(lhs.perceived_bearing, rhs.perceived_bearing) > MERGABLE_ANGLE_DIFFERENCE)
        return false;

    const auto &lhs_edge = node_based_graph.GetEdgeData(lhs.eid);
    const auto &rhs_edge = node_based_graph.GetEdgeData(rhs.eid);
    const auto &lhs_edge_data = node_data_container.GetAnnotation(lhs_edge.annotation_data);
    const auto &rhs_edge_data = node_data_container.GetAnnotation(rhs_edge.annotation_data);

    // and they need to describe the same road
    if ((lhs_edge.reversed == rhs_edge.reversed) ||
        !EdgeDataSupportsMerge(lhs_edge.flags, rhs_edge.flags, lhs_edge_data, rhs_edge_data))
        return false;

    /* don't use any circular links, since they mess up detection we jump out early.
     *
     *          / -- \
     * a ---- b - - /
     */
    const auto road_target = [this](const MergableRoadData &road)
    { return node_based_graph.GetTarget(road.eid); };

    // TODO might have to skip over trivial intersections
    if (road_target(lhs) == intersection_node || road_target(rhs) == intersection_node)
        return false;

    // Don't merge turning circles/traffic loops
    if (IsTrafficLoop(intersection_node, lhs) || IsTrafficLoop(intersection_node, rhs))
        return false;

    // needs to be checked prior to link roads, since connections can seem like links
    if (IsTrafficIsland(intersection_node, lhs, rhs))
        return true;

    // Don't merge link roads
    if (IsLinkRoad(intersection_node, lhs) || IsLinkRoad(intersection_node, rhs))
        return false;

    // check if we simply split up prior to an intersection
    if (IsNarrowTriangle(intersection_node, lhs, rhs))
        return true;

    // finally check if two roads describe the direction
    return HaveSameDirection(intersection_node, lhs, rhs) &&
           !IsCircularShape(intersection_node, lhs, rhs);
}

bool MergableRoadDetector::IsDistinctFrom(const MergableRoadData &lhs,
                                          const MergableRoadData &rhs) const
{
    // needs to be far away
    if (angularDeviation(lhs.perceived_bearing, rhs.perceived_bearing) > MERGABLE_ANGLE_DIFFERENCE)
        return true;
    else // or it cannot have the same name
        return !HaveIdenticalNames(
            node_data_container.GetAnnotation(node_based_graph.GetEdgeData(lhs.eid).annotation_data)
                .name_id,
            node_data_container.GetAnnotation(node_based_graph.GetEdgeData(rhs.eid).annotation_data)
                .name_id,
            name_table,
            street_name_suffix_table);
}

bool MergableRoadDetector::EdgeDataSupportsMerge(
    const NodeBasedEdgeClassification &lhs_flags,
    const NodeBasedEdgeClassification &rhs_flags,
    const NodeBasedEdgeAnnotation &lhs_annotation,
    const NodeBasedEdgeAnnotation &rhs_annotation) const
{
    // roundabouts are special, simply don't hurt them. We might not want to bear the
    // consequences
    if (lhs_flags.roundabout || rhs_flags.roundabout)
        return false;

    /* The travel mode should be the same for both roads. If we were to merge different travel
     * modes, we would hide information/run the risk of loosing valid choices (e.g. short period
     * of pushing)
     */
    if (lhs_annotation.travel_mode != rhs_annotation.travel_mode)
        return false;

    // we require valid names
    if (!HaveIdenticalNames(
            lhs_annotation.name_id, rhs_annotation.name_id, name_table, street_name_suffix_table))
        return false;

    return lhs_flags.road_classification == rhs_flags.road_classification;
}

bool MergableRoadDetector::IsTrafficLoop(const NodeID intersection_node,
                                         const MergableRoadData &road) const
{
    const auto connection =
        intersection::skipDegreeTwoNodes(node_based_graph, {intersection_node, road.eid});
    return intersection_node == node_based_graph.GetTarget(connection.edge);
}

bool MergableRoadDetector::IsNarrowTriangle(const NodeID intersection_node,
                                            const MergableRoadData &lhs,
                                            const MergableRoadData &rhs) const
{
    // selection data to the right and left
    const auto constexpr SMALL_RANDOM_HOPLIMIT = 5;
    IntersectionFinderAccumulator left_accumulator(SMALL_RANDOM_HOPLIMIT,
                                                   node_based_graph,
                                                   node_data_container,
                                                   node_coordinates,
                                                   compressed_geometries,
                                                   node_restriction_map,
                                                   barrier_nodes,
                                                   turn_lanes_data),
        right_accumulator(SMALL_RANDOM_HOPLIMIT,
                          node_based_graph,
                          node_data_container,
                          node_coordinates,
                          compressed_geometries,
                          node_restriction_map,
                          barrier_nodes,
                          turn_lanes_data);

    /* Standard following the straightmost road
     * Since both items have the same id, we can `select` based on any setup
     */
    SelectStraightmostRoadByNameAndOnlyChoice selector(
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(lhs.eid).annotation_data)
            .name_id,
        lhs.perceived_bearing,
        /*requires entry=*/false,
        false);

    NodeBasedGraphWalker graph_walker(node_based_graph,
                                      node_data_container,
                                      node_coordinates,
                                      compressed_geometries,
                                      node_restriction_map,
                                      barrier_nodes,
                                      turn_lanes_data);
    graph_walker.TraverseRoad(intersection_node, lhs.eid, left_accumulator, selector);
    /* if the intersection does not have a right turn, we continue onto the next one once
     * (skipping over a single small side street)
     */
    if (angularDeviation(left_accumulator.intersection.findClosestTurn(ORTHOGONAL_ANGLE)->angle,
                         ORTHOGONAL_ANGLE) > NARROW_TURN_ANGLE)
    {
        graph_walker.TraverseRoad(
            node_based_graph.GetTarget(left_accumulator.via_edge_id),
            left_accumulator.intersection.findClosestTurn(STRAIGHT_ANGLE)->eid,
            left_accumulator,
            selector);
    }
    const auto distance_to_triangle = util::coordinate_calculation::greatCircleDistance(
        node_coordinates[intersection_node],
        node_coordinates[node_based_graph.GetTarget(left_accumulator.via_edge_id)]);

    // don't move too far down the road
    const constexpr auto RANGE_TO_TRIANGLE_LIMIT = 80;
    if (distance_to_triangle > RANGE_TO_TRIANGLE_LIMIT)
        return false;

    graph_walker.TraverseRoad(intersection_node, rhs.eid, right_accumulator, selector);
    if (angularDeviation(right_accumulator.intersection.findClosestTurn(270)->angle, 270) >
        NARROW_TURN_ANGLE)
    {
        graph_walker.TraverseRoad(
            node_based_graph.GetTarget(right_accumulator.via_edge_id),
            right_accumulator.intersection.findClosestTurn(STRAIGHT_ANGLE)->eid,
            right_accumulator,
            selector);
    }

    BOOST_ASSERT(!left_accumulator.intersection.empty() && !right_accumulator.intersection.empty());

    // find the closes resembling a right turn
    const auto connector_turn = left_accumulator.intersection.findClosestTurn(ORTHOGONAL_ANGLE);
    /* check if that right turn connects to the right_accumulator intersection (i.e. we have a
     * triangle)
     * a connection should be somewhat to the right, when looking at the left side of the
     * triangle
     *
     *    b ..... c
     *     \     /
     *      \   /
     *       \ /
     *        a
     *
     * e.g. here when looking at `a,b`, a narrow triangle should offer a turn to the right, when
     * we want to connect to c
     */
    if (angularDeviation(connector_turn->angle, ORTHOGONAL_ANGLE) > NARROW_TURN_ANGLE)
        return false;

    const auto num_lanes = [this](const MergableRoadData &road)
    {
        return std::max<std::uint8_t>(
            node_based_graph.GetEdgeData(road.eid).flags.road_classification.GetNumberOfLanes(), 1);
    };

    // the width we can bridge at the intersection
    const auto assumed_road_width = (num_lanes(lhs) + num_lanes(rhs)) * ASSUMED_LANE_WIDTH;
    const constexpr auto MAXIMAL_ALLOWED_TRAFFIC_ISLAND_WIDTH = 10;
    const auto distance_between_triangle_corners =
        util::coordinate_calculation::greatCircleDistance(
            node_coordinates[node_based_graph.GetTarget(left_accumulator.via_edge_id)],
            node_coordinates[node_based_graph.GetTarget(right_accumulator.via_edge_id)]);
    if (distance_between_triangle_corners >
        (assumed_road_width + MAXIMAL_ALLOWED_TRAFFIC_ISLAND_WIDTH))
        return false;

    // check if both intersections are connected
    IntersectionFinderAccumulator connect_accumulator(SMALL_RANDOM_HOPLIMIT,
                                                      node_based_graph,
                                                      node_data_container,
                                                      node_coordinates,
                                                      compressed_geometries,
                                                      node_restriction_map,
                                                      barrier_nodes,
                                                      turn_lanes_data);
    graph_walker.TraverseRoad(node_based_graph.GetTarget(left_accumulator.via_edge_id),
                              connector_turn->eid,
                              connect_accumulator,
                              selector);

    // the if both items are connected
    return node_based_graph.GetTarget(connect_accumulator.via_edge_id) ==
           node_based_graph.GetTarget(right_accumulator.via_edge_id);
}

bool MergableRoadDetector::IsCircularShape(const NodeID intersection_node,
                                           const MergableRoadData &lhs,
                                           const MergableRoadData &rhs) const
{
    NodeBasedGraphWalker graph_walker(node_based_graph,
                                      node_data_container,
                                      node_coordinates,
                                      compressed_geometries,
                                      node_restriction_map,
                                      barrier_nodes,
                                      turn_lanes_data);
    const auto getCoordinatesAlongWay = [&](const EdgeID edge_id, const double max_length)
    {
        LengthLimitedCoordinateAccumulator accumulator(coordinate_extractor, max_length);
        SelectStraightmostRoadByNameAndOnlyChoice selector(
            node_data_container.GetAnnotation(node_based_graph.GetEdgeData(edge_id).annotation_data)
                .name_id,
            lhs.perceived_bearing,
            /*requires_entry=*/false,
            false);
        graph_walker.TraverseRoad(intersection_node, edge_id, accumulator, selector);

        return std::make_pair(accumulator.accumulated_length, accumulator.coordinates);
    };

    std::vector<util::Coordinate> coordinates_to_the_left, coordinates_to_the_right;
    double distance_traversed_to_the_left, distance_traversed_to_the_right;

    std::tie(distance_traversed_to_the_left, coordinates_to_the_left) =
        getCoordinatesAlongWay(lhs.eid, distance_to_extract);

    std::tie(distance_traversed_to_the_right, coordinates_to_the_right) =
        getCoordinatesAlongWay(rhs.eid, distance_to_extract);

    const auto connect_again = (coordinates_to_the_left.back() == coordinates_to_the_right.back());

    // Tuning parameter to detect and don't merge roads close to circular shapes
    // if the area to squared circumference ratio is between the lower bound and 1/(4π)
    // that correspond to isoperimetric inequality 4πA ≤ L² or lower bound ≤ A/L² ≤ 1/(4π).
    // The lower bound must be larger enough to allow merging of square-shaped intersections
    // with A/L² = 1/16 or 78.6%
    // The condition suppresses roads merging for intersections like
    //             .  .
    //           .      .
    //       ----        ----
    //           .      .
    //             .  .
    // but will allow roads merging for intersections like
    //           -------
    //          /       \ 
    //      ----         ----
    //          \       /
    //           -------
    const auto constexpr CIRCULAR_POLYGON_ISOPERIMETRIC_LOWER_BOUND = 0.85 / (4 * std::numbers::pi);
    if (connect_again && coordinates_to_the_left.front() == coordinates_to_the_left.back())
    { // if the left and right roads connect again and are closed polygons ...
        const auto area = util::coordinate_calculation::computeArea(coordinates_to_the_left);
        const auto perimeter = distance_traversed_to_the_left;
        const auto area_to_squared_perimeter_ratio = std::abs(area) / (perimeter * perimeter);

        // then don't merge roads if A/L² is greater than the lower bound
        BOOST_ASSERT(area_to_squared_perimeter_ratio <= 1. / (4 * std::numbers::pi));
        if (area_to_squared_perimeter_ratio >= CIRCULAR_POLYGON_ISOPERIMETRIC_LOWER_BOUND)
            return true;
    }

    return false;
}

bool MergableRoadDetector::HaveSameDirection(const NodeID intersection_node,
                                             const MergableRoadData &lhs,
                                             const MergableRoadData &rhs) const
{
    if (angularDeviation(lhs.perceived_bearing, rhs.perceived_bearing) > MERGABLE_ANGLE_DIFFERENCE)
        return false;

    // Find a coordinate following a road that is far away
    NodeBasedGraphWalker graph_walker(node_based_graph,
                                      node_data_container,
                                      node_coordinates,
                                      compressed_geometries,
                                      node_restriction_map,
                                      barrier_nodes,
                                      turn_lanes_data);
    const auto getCoordinatesAlongWay = [&](const EdgeID edge_id, const double max_length)
    {
        LengthLimitedCoordinateAccumulator accumulator(coordinate_extractor, max_length);
        SelectStraightmostRoadByNameAndOnlyChoice selector(
            node_data_container.GetAnnotation(node_based_graph.GetEdgeData(edge_id).annotation_data)
                .name_id,
            lhs.perceived_bearing,
            /*requires_entry=*/false,
            true);
        graph_walker.TraverseRoad(intersection_node, edge_id, accumulator, selector);

        return std::make_pair(accumulator.accumulated_length, accumulator.coordinates);
    };

    std::vector<util::Coordinate> coordinates_to_the_left, coordinates_to_the_right;
    double distance_traversed_to_the_left, distance_traversed_to_the_right;

    std::tie(distance_traversed_to_the_left, coordinates_to_the_left) =
        getCoordinatesAlongWay(lhs.eid, distance_to_extract);

    // tuned parameter, if we didn't get as far as 40 meters, we might barely look past an
    // intersection.
    const auto constexpr MINIMUM_LENGTH_FOR_PARALLEL_DETECTION = 40;
    // quit early if the road is not very long
    if (distance_traversed_to_the_left <= MINIMUM_LENGTH_FOR_PARALLEL_DETECTION)
        return false;

    std::tie(distance_traversed_to_the_right, coordinates_to_the_right) =
        getCoordinatesAlongWay(rhs.eid, distance_to_extract);

    if (distance_traversed_to_the_right <= MINIMUM_LENGTH_FOR_PARALLEL_DETECTION)
        return false;

    const auto connect_again = (coordinates_to_the_left.back() == coordinates_to_the_right.back());
    // sampling to correctly weight longer segments in regression calculations
    const auto constexpr SAMPLE_INTERVAL = 5;
    coordinates_to_the_left = coordinate_extractor.SampleCoordinates(
        coordinates_to_the_left, distance_to_extract, SAMPLE_INTERVAL);

    coordinates_to_the_right = coordinate_extractor.SampleCoordinates(
        coordinates_to_the_right, distance_to_extract, SAMPLE_INTERVAL);

    /* extract the number of lanes for a road
     * restricts a vector to the last two thirds of the length
     */
    const auto prune = [](auto &data_vector)
    {
        BOOST_ASSERT(data_vector.size() >= 3);
        // erase the first third of the vector
        data_vector.erase(data_vector.begin(), data_vector.begin() + data_vector.size() / 3);
    };

    /* if the coordinates meet up again, e.g. due to a split and join, pruning can have a negative
     * effect. We therefore only prune away the beginning, if the roads don't meet up again as well.
     */
    if (!connect_again)
    {
        prune(coordinates_to_the_left);
        prune(coordinates_to_the_right);
    }

    const auto are_parallel =
        util::coordinate_calculation::areParallel(coordinates_to_the_left.begin(),
                                                  coordinates_to_the_left.end(),
                                                  coordinates_to_the_right.begin(),
                                                  coordinates_to_the_right.end());

    if (!are_parallel)
        return false;

    // compare reference distance:
    const auto distance_mid_left_to_right = util::coordinate_calculation::findClosestDistance(
        coordinates_to_the_left[coordinates_to_the_left.size() / 2],
        coordinates_to_the_right.begin(),
        coordinates_to_the_right.end());
    const auto distance_mid_right_to_left = util::coordinate_calculation::findClosestDistance(
        coordinates_to_the_right[coordinates_to_the_right.size() / 2],
        coordinates_to_the_left.begin(),
        coordinates_to_the_left.end());
    const auto distance_between_roads =
        std::min(distance_mid_left_to_right, distance_mid_right_to_left);

    const auto lane_count_lhs = std::max<int>(
        1, node_based_graph.GetEdgeData(lhs.eid).flags.road_classification.GetNumberOfLanes());
    const auto lane_count_rhs = std::max<int>(
        1, node_based_graph.GetEdgeData(rhs.eid).flags.road_classification.GetNumberOfLanes());

    const auto combined_road_width = 0.5 * (lane_count_lhs + lane_count_rhs) * ASSUMED_LANE_WIDTH;
    const auto constexpr MAXIMAL_ALLOWED_SEPARATION_WIDTH = 12;

    return distance_between_roads <= combined_road_width + MAXIMAL_ALLOWED_SEPARATION_WIDTH;
}

bool MergableRoadDetector::IsTrafficIsland(const NodeID intersection_node,
                                           const MergableRoadData &lhs,
                                           const MergableRoadData &rhs) const
{
    /* compute the set of all intersection_nodes along the way of an edge, until it reaches a
     * location with the same name repeatet at least three times
     */
    const auto left_connection =
        intersection::skipDegreeTwoNodes(node_based_graph, {intersection_node, lhs.eid});
    const auto right_connection =
        intersection::skipDegreeTwoNodes(node_based_graph, {intersection_node, rhs.eid});

    const auto left_candidate = node_based_graph.GetTarget(left_connection.edge);
    const auto right_candidate = node_based_graph.GetTarget(right_connection.edge);

    const auto candidate_is_valid =
        left_candidate == right_candidate && left_candidate != intersection_node;

    if (!candidate_is_valid)
        return false;

    // check if all entries at the destination or at the source are the same
    const auto all_same_name_and_degree_three = [this](const NodeID nid)
    {
        // check if the intersection found has degree three
        if (node_based_graph.GetOutDegree(nid) != 3)
            return false;

        // check if all items share a name
        const auto range = node_based_graph.GetAdjacentEdgeRange(nid);
        const auto required_name_id =
            node_data_container
                .GetAnnotation(node_based_graph.GetEdgeData(range.front()).annotation_data)
                .name_id;

        const auto has_required_name = [this, required_name_id](const auto edge_id)
        {
            const auto road_name_id =
                node_data_container
                    .GetAnnotation(node_based_graph.GetEdgeData(edge_id).annotation_data)
                    .name_id;
            const auto &road_name_empty = name_table.GetNameForID(road_name_id).empty();
            const auto &required_name_empty = name_table.GetNameForID(required_name_id).empty();
            if (required_name_empty && road_name_empty)
                return false;
            return !util::guidance::requiresNameAnnounced(
                       required_name_id, road_name_id, name_table, street_name_suffix_table) ||
                   !util::guidance::requiresNameAnnounced(
                       road_name_id, required_name_id, name_table, street_name_suffix_table);
        };

        /* the beautiful way would be:
         * return range.end() == std::find_if_not(range.begin(), range.end(), has_required_name);
         * but that does not work due to range concepts
         */
        for (const auto eid : range)
            if (!has_required_name(eid))
                return false;

        return true;
    };

    const auto degree_three_connect_in = all_same_name_and_degree_three(intersection_node);
    const auto degree_three_connect_out = all_same_name_and_degree_three(left_candidate);

    if (!degree_three_connect_in && !degree_three_connect_out)
        return false;

    const auto distance_between_candidates = util::coordinate_calculation::greatCircleDistance(
        node_coordinates[intersection_node], node_coordinates[left_candidate]);

    const auto both_split_join = degree_three_connect_in && degree_three_connect_out;

    // allow longer separations if both are joining directly
    // widths are chosen via tuning on traffic islands
    return both_split_join ? (distance_between_candidates < 30)
                           : (distance_between_candidates < 15);
}

bool MergableRoadDetector::IsLinkRoad(const NodeID intersection_node,
                                      const MergableRoadData &road) const
{
    const auto next_intersection_parameters =
        intersection::skipDegreeTwoNodes(node_based_graph, {intersection_node, road.eid});
    const auto next_intersection_along_road =
        intersection::getConnectedRoads<false>(node_based_graph,
                                               node_data_container,
                                               node_coordinates,
                                               compressed_geometries,
                                               node_restriction_map,
                                               barrier_nodes,
                                               turn_lanes_data,
                                               next_intersection_parameters);
    const auto extract_name_id = [this](const MergableRoadData &road)
    {
        return node_data_container
            .GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
            .name_id;
    };

    const auto requested_name_id = extract_name_id(road);
    const auto next_road_along_path = next_intersection_along_road.findClosestTurn(
        STRAIGHT_ANGLE,
        makeCheckRoadForName(requested_name_id,
                             node_based_graph,
                             node_data_container,
                             name_table,
                             street_name_suffix_table));

    // we need to have a continuing road to successfully detect a link road
    if (next_road_along_path == next_intersection_along_road.end())
        return false;

    const auto opposite_of_next_road_along_path = next_intersection_along_road.findClosestTurn(
        util::restrictAngleToValidRange(next_road_along_path->angle + STRAIGHT_ANGLE));

    // we cannot be looking at the same road we came from
    if (node_based_graph.GetTarget(opposite_of_next_road_along_path->eid) ==
        next_intersection_parameters.node)
        return false;

    /* check if the opposite of the next road decision was sane. It could have been just as well our
     * incoming road.
     */
    if (angularDeviation(angularDeviation(next_road_along_path->angle, STRAIGHT_ANGLE),
                         angularDeviation(opposite_of_next_road_along_path->angle, 0)) <
        FUZZY_ANGLE_DIFFERENCE)
        return false;

    // near straight road that continues
    return angularDeviation(opposite_of_next_road_along_path->angle, next_road_along_path->angle) >=
               (STRAIGHT_ANGLE - FUZZY_ANGLE_DIFFERENCE) &&
           (node_based_graph.GetEdgeData(next_road_along_path->eid).reversed ==
            node_based_graph.GetEdgeData(opposite_of_next_road_along_path->eid).reversed) &&
           EdgeDataSupportsMerge(
               node_based_graph.GetEdgeData(next_road_along_path->eid).flags,
               node_based_graph.GetEdgeData(opposite_of_next_road_along_path->eid).flags,
               node_data_container.GetAnnotation(
                   node_based_graph.GetEdgeData(next_road_along_path->eid).annotation_data),
               node_data_container.GetAnnotation(
                   node_based_graph.GetEdgeData(opposite_of_next_road_along_path->eid)
                       .annotation_data));
}

} // namespace osrm::extractor::intersection
