#include "extractor/intersection/intersection_analysis.hpp"
#include "extractor/intersection/coordinate_extractor.hpp"

#include "util/assert.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"

#include <boost/optional/optional_io.hpp>
#include <numbers>

namespace osrm::extractor::intersection
{

IntersectionEdges getIncomingEdges(const util::NodeBasedDynamicGraph &graph,
                                   const NodeID intersection_node)
{
    IntersectionEdges result;

    for (const auto outgoing_edge : graph.GetAdjacentEdgeRange(intersection_node))
    {
        const auto from_node = graph.GetTarget(outgoing_edge);
        const auto incoming_edge = graph.FindEdge(from_node, intersection_node);

        if (!graph.GetEdgeData(incoming_edge).reversed)
        {
            result.push_back({from_node, incoming_edge});
        }
    }

    // Enforce ordering of incoming edges
    std::sort(result.begin(), result.end());
    return result;
}

IntersectionEdges getOutgoingEdges(const util::NodeBasedDynamicGraph &graph,
                                   const NodeID intersection_node)
{
    IntersectionEdges result;

    for (const auto outgoing_edge : graph.GetAdjacentEdgeRange(intersection_node))
    {
        result.push_back({intersection_node, outgoing_edge});
    }

    BOOST_ASSERT(std::is_sorted(result.begin(), result.end()));
    return result;
}

std::vector<util::Coordinate>
getEdgeCoordinates(const extractor::CompressedEdgeContainer &compressed_geometries,
                   const std::vector<util::Coordinate> &node_coordinates,
                   const NodeID from_node,
                   const EdgeID edge,
                   const NodeID to_node)
{
    if (!compressed_geometries.HasEntryForID(edge))
        return {node_coordinates[from_node], node_coordinates[to_node]};

    BOOST_ASSERT(from_node < node_coordinates.size());
    BOOST_ASSERT(to_node < node_coordinates.size());

    // extracts the geometry in coordinates from the compressed edge container
    std::vector<util::Coordinate> result;
    const auto &geometry = compressed_geometries.GetBucketReference(edge);
    result.reserve(geometry.size() + 1);

    result.push_back(node_coordinates[from_node]);
    std::transform(geometry.begin(),
                   geometry.end(),
                   std::back_inserter(result),
                   [&node_coordinates](const auto &compressed_edge)
                   { return node_coordinates[compressed_edge.node_id]; });

    // filter duplicated coordinates
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

namespace
{
double findAngleBisector(double alpha, double beta)
{
    alpha *= std::numbers::pi / 180.;
    beta *= std::numbers::pi / 180.;
    const auto average =
        180. * std::atan2(std::sin(alpha) + std::sin(beta), std::cos(alpha) + std::cos(beta)) *
        std::numbers::inv_pi;
    return std::fmod(average + 360., 360.);
}

double findClosestOppositeBearing(const IntersectionEdgeGeometries &edge_geometries,
                                  const double bearing)
{
    BOOST_ASSERT(!edge_geometries.empty());
    const auto min = std::min_element(
        edge_geometries.begin(),
        edge_geometries.end(),
        [bearing = util::bearing::reverse(bearing)](const auto &lhs, const auto &rhs)
        {
            return util::angularDeviation(lhs.perceived_bearing, bearing) <
                   util::angularDeviation(rhs.perceived_bearing, bearing);
        });
    return util::bearing::reverse(min->perceived_bearing);
}

std::pair<bool, double> findMergedBearing(const util::NodeBasedDynamicGraph &graph,
                                          const IntersectionEdgeGeometries &edge_geometries,
                                          std::size_t lhs_index,
                                          std::size_t rhs_index,
                                          bool neighbor_intersection)
{
    // Function returns a pair with a flag and a value of bearing for merged roads
    // If the flag is false the bearing must not be used as a merged value at neighbor intersections

    using util::angularDeviation;
    using util::bearing::angleBetween;

    const auto &lhs = edge_geometries[lhs_index];
    const auto &rhs = edge_geometries[rhs_index];
    BOOST_ASSERT(graph.GetEdgeData(lhs.eid).reversed != graph.GetEdgeData(rhs.eid).reversed);

    const auto &entry = graph.GetEdgeData(lhs.eid).reversed ? rhs : lhs;
    const auto opposite_bearing =
        findClosestOppositeBearing(edge_geometries, entry.perceived_bearing);
    const auto merged_bearing = findAngleBisector(rhs.perceived_bearing, lhs.perceived_bearing);

    if (angularDeviation(angleBetween(opposite_bearing, entry.perceived_bearing), STRAIGHT_ANGLE) <
        MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // In some intersections, turning roads can introduce artificial turns if we merge here.
        // Consider a scenario like:
        //  
        //  a     .  g - f
        //  |   .
        //  | .
        //  |.
        // d-b--------e
        //  |
        //  c
        //  
        // Merging `bgf` and `be` would introduce an angle, even though d-b-e is perfectly straight
        // We don't change the angle, if such an opposite road exists
        return {false, entry.perceived_bearing};
    }

    if (neighbor_intersection)
    {
        // Check that the merged bearing makes both turns closer to straight line
        const auto turn_angle_lhs = angleBetween(opposite_bearing, lhs.perceived_bearing);
        const auto turn_angle_rhs = angleBetween(opposite_bearing, rhs.perceived_bearing);
        const auto turn_angle_new = angleBetween(opposite_bearing, merged_bearing);

        if (util::angularDeviation(turn_angle_lhs, STRAIGHT_ANGLE) <
                util::angularDeviation(turn_angle_new, STRAIGHT_ANGLE) ||
            util::angularDeviation(turn_angle_rhs, STRAIGHT_ANGLE) <
                util::angularDeviation(turn_angle_new, STRAIGHT_ANGLE))
            return {false, opposite_bearing};
    }

    return {true, merged_bearing};
}

bool isRoadsPairMergeable(const MergableRoadDetector &detector,
                          const IntersectionEdgeGeometries &edge_geometries,
                          const NodeID intersection_node,
                          const std::size_t index)
{
    const auto size = edge_geometries.size();
    BOOST_ASSERT(index < size);

    const auto &llhs = edge_geometries[(index + size - 1) % size];
    const auto &lhs = edge_geometries[index];
    const auto &rhs = edge_geometries[(index + 1) % size];
    const auto &rrhs = edge_geometries[(index + 2) % size];

    // TODO: check IsDistinctFrom - it is an angle and name-only check
    // also check CanMergeRoad for all merging scenarios
    return detector.IsDistinctFrom(llhs, lhs) &&
           detector.CanMergeRoad(intersection_node, lhs, rhs) && detector.IsDistinctFrom(rhs, rrhs);
}

auto getIntersectionLanes(const util::NodeBasedDynamicGraph &graph, const NodeID intersection_node)
{
    std::uint8_t max_lanes_intersection = 0;
    for (auto outgoing_edge : graph.GetAdjacentEdgeRange(intersection_node))
    {
        max_lanes_intersection =
            std::max(max_lanes_intersection,
                     graph.GetEdgeData(outgoing_edge).flags.road_classification.GetNumberOfLanes());
    }
    return max_lanes_intersection;
}

template <bool USE_CLOSE_COORDINATE>
IntersectionEdgeGeometries
getIntersectionOutgoingGeometries(const util::NodeBasedDynamicGraph &graph,
                                  const extractor::CompressedEdgeContainer &compressed_geometries,
                                  const std::vector<util::Coordinate> &node_coordinates,
                                  const NodeID intersection_node)
{
    IntersectionEdgeGeometries edge_geometries;

    // TODO: keep CoordinateExtractor to reproduce bearings, simplify later
    const CoordinateExtractor coordinate_extractor(graph, compressed_geometries, node_coordinates);

    const auto max_lanes_intersection = getIntersectionLanes(graph, intersection_node);

    // Collect outgoing edges
    for (const auto outgoing_edge : graph.GetAdjacentEdgeRange(intersection_node))
    {
        const auto remote_node = graph.GetTarget(outgoing_edge);

        const auto &geometry = getEdgeCoordinates(
            compressed_geometries, node_coordinates, intersection_node, outgoing_edge, remote_node);

        // OSRM_ASSERT(geometry.size() >= 2, node_coordinates[intersection_node]);

        const auto close_coordinate =
            coordinate_extractor.ExtractCoordinateAtLength(2. /*m*/, geometry);
        const auto initial_bearing =
            util::coordinate_calculation::bearing(geometry[0], close_coordinate);

        const auto representative_coordinate =
            USE_CLOSE_COORDINATE || graph.GetOutDegree(intersection_node) <= 2
                ? coordinate_extractor.GetCoordinateCloseToTurn(
                      intersection_node, outgoing_edge, false, remote_node)
                : coordinate_extractor.ExtractRepresentativeCoordinate(intersection_node,
                                                                       outgoing_edge,
                                                                       false,
                                                                       remote_node,
                                                                       max_lanes_intersection,
                                                                       geometry);
        const auto perceived_bearing =
            util::coordinate_calculation::bearing(geometry[0], representative_coordinate);

        const auto edge_length = util::coordinate_calculation::getLength(
            geometry.begin(), geometry.end(), util::coordinate_calculation::greatCircleDistance);

        edge_geometries.push_back({outgoing_edge, initial_bearing, perceived_bearing, edge_length});
    }

    // Sort edges in the clockwise bearings order
    std::sort(edge_geometries.begin(),
              edge_geometries.end(),
              [](const auto &lhs, const auto &rhs)
              { return lhs.perceived_bearing < rhs.perceived_bearing; });
    return edge_geometries;
}
} // namespace

std::pair<IntersectionEdgeGeometries, std::unordered_set<EdgeID>>
getIntersectionGeometries(const util::NodeBasedDynamicGraph &graph,
                          const extractor::CompressedEdgeContainer &compressed_geometries,
                          const std::vector<util::Coordinate> &node_coordinates,
                          const MergableRoadDetector &detector,
                          const NodeID intersection_node)
{
    IntersectionEdgeGeometries edge_geometries = getIntersectionOutgoingGeometries<false>(
        graph, compressed_geometries, node_coordinates, intersection_node);

    const auto edges_number = edge_geometries.size();

    std::vector<bool> merged_edges(edges_number, false);

    // TODO: intersection views do not contain merged and not allowed edges
    // but contain other restricted edges that are used in TurnAnalysis,
    // to be deleted after TurnAnalysis refactoring
    std::unordered_set<EdgeID> merged_edge_ids;

    if (edges_number >= 3)
    { // Adjust bearings of mergeable roads
        for (std::size_t index = 0; index < edges_number; ++index)
        {
            if (isRoadsPairMergeable(detector, edge_geometries, intersection_node, index))
            { // Merge bearings of roads left & right
                const auto next = (index + 1) % edges_number;
                auto &lhs = edge_geometries[index];
                auto &rhs = edge_geometries[next];
                merged_edges[index] = true;
                merged_edges[next] = true;

                const auto merge = findMergedBearing(graph, edge_geometries, index, next, false);

                lhs.perceived_bearing = lhs.initial_bearing = merge.second;
                rhs.perceived_bearing = rhs.initial_bearing = merge.second;

                // Only one of the edges must be reversed, mark it as merged to remove from
                // intersection view
                BOOST_ASSERT(graph.GetEdgeData(lhs.eid).reversed ^
                             graph.GetEdgeData(rhs.eid).reversed);
                merged_edge_ids.insert(graph.GetEdgeData(lhs.eid).reversed ? lhs.eid : rhs.eid);
            }
        }
    }

    if (edges_number >= 2)
    { // Adjust bearings of roads that will be merged at the neighbor intersections
        const double constexpr PRUNING_DISTANCE = 30.;

        for (std::size_t index = 0; index < edges_number; ++index)
        {
            auto &edge_geometry = edge_geometries[index];

            // Don't adjust bearings of roads that were merged at the current intersection
            // or have neighbor intersection farer than the pruning distance
            if (merged_edges[index] || edge_geometry.segment_length > PRUNING_DISTANCE)
                continue;

            const auto neighbor_intersection_node = graph.GetTarget(edge_geometry.eid);

            const auto neighbor_geometries = getIntersectionOutgoingGeometries<false>(
                graph, compressed_geometries, node_coordinates, neighbor_intersection_node);

            const auto neighbor_edges = neighbor_geometries.size();
            if (neighbor_edges <= 1)
                continue;

            const auto neighbor_curr = std::distance(
                neighbor_geometries.begin(),
                std::find_if(neighbor_geometries.begin(),
                             neighbor_geometries.end(),
                             [&graph, &intersection_node](const auto &road)
                             { return graph.GetTarget(road.eid) == intersection_node; }));
            BOOST_ASSERT(static_cast<std::size_t>(neighbor_curr) != neighbor_geometries.size());
            const auto neighbor_prev = (neighbor_curr + neighbor_edges - 1) % neighbor_edges;
            const auto neighbor_next = (neighbor_curr + 1) % neighbor_edges;

            if (isRoadsPairMergeable(
                    detector, neighbor_geometries, neighbor_intersection_node, neighbor_prev))
            { // Neighbor intersection has mergable neighbor_prev and neighbor_curr roads
                BOOST_ASSERT(!isRoadsPairMergeable(
                    detector, neighbor_geometries, neighbor_intersection_node, neighbor_curr));

                // TODO: merge with an angle bisector, but not a reversed closed turn, to be
                // checked as a difference with the previous implementation
                const auto merge = findMergedBearing(
                    graph, neighbor_geometries, neighbor_prev, neighbor_curr, true);

                if (merge.first)
                {
                    const auto offset = util::angularDeviation(
                        merge.second, neighbor_geometries[neighbor_curr].perceived_bearing);

                    // Adjust bearing of AB at the node A if at the node B roads BA (neighbor_curr)
                    // and BC (neighbor_prev) will be merged and will have merged bearing Bb.
                    // The adjustment value is ∠bBA with negative sign (counter-clockwise) to Aa
                    //     A ~~~ a
                    //       \ 
                    //  b --- B ---
                    //       /
                    //     C
                    edge_geometry.perceived_bearing = edge_geometry.initial_bearing =
                        std::fmod(edge_geometry.perceived_bearing + 360. - offset, 360.);
                }
            }
            else if (isRoadsPairMergeable(
                         detector, neighbor_geometries, neighbor_intersection_node, neighbor_curr))
            { // Neighbor intersection has mergable neighbor_curr and neighbor_next roads
                BOOST_ASSERT(!isRoadsPairMergeable(
                    detector, neighbor_geometries, neighbor_intersection_node, neighbor_prev));

                // TODO: merge with an angle bisector, but not a reversed closed turn, to be
                // checked as a difference with the previous implementation
                const auto merge = findMergedBearing(
                    graph, neighbor_geometries, neighbor_curr, neighbor_next, true);
                if (merge.first)
                {
                    const auto offset = util::angularDeviation(
                        merge.second, neighbor_geometries[neighbor_curr].perceived_bearing);

                    // Adjust bearing of AB at the node A if at the node B roads BA (neighbor_curr)
                    // and BC (neighbor_next) will be merged and will have merged bearing Bb.
                    // The adjustment value is ∠bBA with positive sign (clockwise) to Aa
                    //     a ~~~ A
                    //         /
                    //    --- B --- b
                    //         \ 
                    //          C
                    edge_geometry.perceived_bearing = edge_geometry.initial_bearing =
                        std::fmod(edge_geometry.perceived_bearing + offset, 360.);
                }
            }
        }
    }

    // Add incoming edges with reversed bearings
    edge_geometries.resize(2 * edges_number);
    for (std::size_t index = 0; index < edges_number; ++index)
    {
        const auto &geometry = edge_geometries[index];
        const auto remote_node = graph.GetTarget(geometry.eid);
        const auto incoming_edge = graph.FindEdge(remote_node, intersection_node);
        edge_geometries[edges_number + index] = {incoming_edge,
                                                 util::bearing::reverse(geometry.initial_bearing),
                                                 util::bearing::reverse(geometry.perceived_bearing),
                                                 geometry.segment_length};
    }

    // Enforce ordering of edges by IDs
    std::sort(edge_geometries.begin(), edge_geometries.end());

    return std::make_pair(edge_geometries, merged_edge_ids);
}

inline auto findEdge(const IntersectionEdgeGeometries &geometries, const EdgeID &edge)
{
    const auto it =
        std::lower_bound(geometries.begin(),
                         geometries.end(),
                         edge,
                         [](const auto &geometry, const auto edge) { return geometry.eid < edge; });
    BOOST_ASSERT(it != geometries.end() && it->eid == edge);
    return it;
}

double findEdgeBearing(const IntersectionEdgeGeometries &geometries, const EdgeID &edge)
{
    return findEdge(geometries, edge)->perceived_bearing;
}

double findEdgeLength(const IntersectionEdgeGeometries &geometries, const EdgeID &edge)
{
    return findEdge(geometries, edge)->segment_length;
}

template <typename RestrictionsRange>
bool isTurnRestricted(const RestrictionsRange &restrictions, const NodeID to)
{
    // Check if any of the restrictions would prevent a turn to 'to'
    return std::any_of(restrictions.begin(),
                       restrictions.end(),
                       [&to](const auto &restriction)
                       { return restriction->IsTurnRestricted(to); });
}

bool isTurnAllowed(const util::NodeBasedDynamicGraph &graph,
                   const EdgeBasedNodeDataContainer &node_data_container,
                   const RestrictionMap &restriction_map,
                   const std::unordered_set<NodeID> &barrier_nodes,
                   const IntersectionEdgeGeometries &geometries,
                   const TurnLanesIndexedArray &turn_lanes_data,
                   const IntersectionEdge &from,
                   const IntersectionEdge &to)
{
    BOOST_ASSERT(graph.GetTarget(from.edge) == to.node);

    // TODO: to use TurnAnalysis all outgoing edges are required, to be removed later
    if (graph.GetEdgeData(from.edge).reversed || graph.GetEdgeData(to.edge).reversed)
        return false;

    const auto intersection_node = to.node;
    const auto destination_node = graph.GetTarget(to.edge);
    auto const &restrictions = restriction_map.Restrictions(from.node, intersection_node);

    // Check if turn is explicitly restricted by a turn restriction
    if (isTurnRestricted(restrictions, destination_node))
        return false;

    // Precompute reversed bearing of the `from` edge
    const auto from_edge_reversed_bearing =
        util::bearing::reverse(findEdgeBearing(geometries, from.edge));

    // Collect some information about the intersection
    // 1) number of allowed exits and adjacent bidirectional edges
    std::uint32_t allowed_exits = 0, bidirectional_edges = 0;
    // 2) edge IDs of roundabouts edges
    EdgeID roundabout_from = SPECIAL_EDGEID, roundabout_to = SPECIAL_EDGEID;
    double roundabout_from_angle = 0., roundabout_to_angle = 0.;

    for (const auto eid : graph.GetAdjacentEdgeRange(intersection_node))
    {
        const auto &edge_data = graph.GetEdgeData(eid);
        const auto &edge_class = edge_data.flags;
        const auto to_node = graph.GetTarget(eid);
        const auto reverse_edge = graph.FindEdge(to_node, intersection_node);
        BOOST_ASSERT(reverse_edge != SPECIAL_EDGEID);

        const auto is_exit_edge = !edge_data.reversed && !isTurnRestricted(restrictions, to_node);
        const auto is_bidirectional = !graph.GetEdgeData(reverse_edge).reversed;
        allowed_exits += is_exit_edge;
        bidirectional_edges += is_bidirectional;

        if (edge_class.roundabout || edge_class.circular)
        {
            if (edge_data.reversed)
            {
                // "Linked Roundabouts" is an example of tie between two linked roundabouts
                // A tie breaker for that maximizes ∠(roundabout_from_bearing, ¬from_edge_bearing)
                const auto angle = util::bearing::angleBetween(
                    findEdgeBearing(geometries, reverse_edge), from_edge_reversed_bearing);
                if (angle > roundabout_from_angle)
                {
                    roundabout_from = reverse_edge;
                    roundabout_from_angle = angle;
                }
            }
            else
            {
                // a tie breaker that maximizes ∠(¬from_edge_bearing, roundabout_to_bearing)
                const auto angle = util::bearing::angleBetween(from_edge_reversed_bearing,
                                                               findEdgeBearing(geometries, eid));
                if (angle > roundabout_to_angle)
                {
                    roundabout_to = eid;
                    roundabout_to_angle = angle;
                }
            }
        }
    }

    // 3) if the intersection has a barrier
    const bool is_barrier_node = barrier_nodes.find(intersection_node) != barrier_nodes.end();

    // Check a U-turn
    if (from.node == destination_node)
    {
        // Allow U-turns before barrier nodes
        if (is_barrier_node)
            return true;

        // Allow U-turns at dead-ends
        if (graph.GetAdjacentEdgeRange(intersection_node).size() == 1)
            return true;

        // Allow U-turns at dead-ends if there is at most one bidirectional road at the intersection
        // The condition allows U-turns d→a→d and c→b→c ("Bike - Around the Block" test)
        //   a→b
        //   ↕ ↕
        //   d↔c
        if (allowed_exits == 1 || bidirectional_edges <= 1)
            return true;

        // Allow U-turn if the incoming edge has a U-turn lane
        // TODO: revisit the use-case, related PR #2753
        const auto &incoming_edge_annotation_id = graph.GetEdgeData(from.edge).annotation_data;
        const auto lane_description_id = static_cast<std::size_t>(
            node_data_container.GetAnnotation(incoming_edge_annotation_id).lane_description_id);
        if (lane_description_id != INVALID_LANE_DESCRIPTIONID)
        {
            const auto &turn_lane_offsets = std::get<0>(turn_lanes_data);
            const auto &turn_lanes = std::get<1>(turn_lanes_data);
            BOOST_ASSERT(lane_description_id + 1 < turn_lane_offsets.size());

            if (std::any_of(turn_lanes.begin() + turn_lane_offsets[lane_description_id],
                            turn_lanes.begin() + turn_lane_offsets[lane_description_id + 1],
                            [](const auto &lane) { return lane & TurnLaneType::uturn; }))
                return true;
        }

        // Don't allow U-turns on usual intersections
        return false;
    }

    // Don't allow turns via barriers for not U-turn maneuvers
    if (is_barrier_node)
        return false;

    // Check for roundabouts exits in the opposite direction of roundabout flow
    if (roundabout_from != SPECIAL_EDGEID && roundabout_to != SPECIAL_EDGEID)
    {
        // Get bearings of edges
        const auto roundabout_from_bearing = findEdgeBearing(geometries, roundabout_from);
        const auto roundabout_to_bearing = findEdgeBearing(geometries, roundabout_to);
        const auto to_edge_bearing = findEdgeBearing(geometries, to.edge);

        // Get angles from the roundabout edge to three other edges
        const auto roundabout_angle =
            util::bearing::angleBetween(roundabout_from_bearing, roundabout_to_bearing);
        const auto roundabout_from_angle =
            util::bearing::angleBetween(roundabout_from_bearing, from_edge_reversed_bearing);
        const auto roundabout_to_angle =
            util::bearing::angleBetween(roundabout_from_bearing, to_edge_bearing);

        // Restrict turning over a roundabout if `roundabout_to_angle` is in
        // a sector between `roundabout_from_bearing` to `from_bearing` (shaded area)
        //
        //    roundabout_angle = 270°         roundabout_angle = 90°
        //  roundabout_from_angle = 150°    roundabout_from_angle = 150°
        //   roundabout_to_angle = 90°       roundabout_to_angle = 270°
        //
        //             150°                            150°
        //              v░░░░░░                ░░░░░░░░░v
        //             v░░░░░░░                ░░░░░░░░v
        // 270° <-ooo- v -ttt-> 90°       270° <-ttt- v -ooo-> 90°
        //             ^░░░░░░░                ░░░░░░░^
        //             r░░░░░░░                ░░░░░░░r
        //             r░░░░░░░                ░░░░░░░r
        if ((roundabout_from_angle < roundabout_angle &&
             roundabout_to_angle < roundabout_from_angle) ||
            (roundabout_from_angle > roundabout_angle &&
             roundabout_to_angle > roundabout_from_angle))
            return false;
    }

    return true;
}

// The function adapts intersection geometry data to TurnAnalysis
IntersectionView convertToIntersectionView(const util::NodeBasedDynamicGraph &graph,
                                           const EdgeBasedNodeDataContainer &node_data_container,
                                           const RestrictionMap &restriction_map,
                                           const std::unordered_set<NodeID> &barrier_nodes,
                                           const IntersectionEdgeGeometries &edge_geometries,
                                           const TurnLanesIndexedArray &turn_lanes_data,
                                           const IntersectionEdge &incoming_edge,
                                           const IntersectionEdges &outgoing_edges,
                                           const std::unordered_set<EdgeID> &merged_edges)
{
    using util::bearing::angleBetween;

    const auto edge_it = findEdge(edge_geometries, incoming_edge.edge);
    const auto incoming_bearing = edge_it->perceived_bearing;
    const auto initial_incoming_bearing = edge_it->initial_bearing;

    using IntersectionViewDataWithAngle = std::pair<IntersectionViewData, double>;
    std::vector<IntersectionViewDataWithAngle> pre_intersection_view;
    IntersectionViewData uturn{{SPECIAL_EDGEID, 0., 0., 0.}, false, 0.};
    std::size_t allowed_uturns_number = 0;

    const auto is_uturn = [](const auto angle)
    { return std::fabs(angle) < std::numeric_limits<double>::epsilon(); };

    for (const auto &outgoing_edge : outgoing_edges)
    {
        const auto edge_it = findEdge(edge_geometries, outgoing_edge.edge);
        const auto is_merged = merged_edges.contains(outgoing_edge.edge);
        const auto is_turn_allowed = intersection::isTurnAllowed(graph,
                                                                 node_data_container,
                                                                 restriction_map,
                                                                 barrier_nodes,
                                                                 edge_geometries,
                                                                 turn_lanes_data,
                                                                 incoming_edge,
                                                                 outgoing_edge);

        // Compute angles
        const auto outgoing_bearing = edge_it->perceived_bearing;
        const auto initial_outgoing_bearing = edge_it->initial_bearing;
        auto turn_angle = std::fmod(
            std::round(angleBetween(incoming_bearing, outgoing_bearing) * 1e8) / 1e8, 360.);
        auto initial_angle = angleBetween(initial_incoming_bearing, initial_outgoing_bearing);

        // If angle of the allowed turn is in a neighborhood of 0° (±15°) but the initial OSM angle
        // is in the opposite semi-plane then assume explicitly a U-turn to avoid incorrect
        // adjustments due to numerical noise in selection of representative_coordinate
        if (is_turn_allowed &&
            ((turn_angle < 15 && initial_angle > 180) || (turn_angle > 345 && initial_angle < 180)))
        {
            turn_angle = 0;
            initial_angle = 0;
        }

        const auto is_uturn_angle = is_uturn(turn_angle);

        IntersectionViewData road{*edge_it, is_turn_allowed, turn_angle};

        if (graph.GetTarget(outgoing_edge.edge) == incoming_edge.node)
        { // Save the true U-turn road to add later if no allowed U-turns will be added
            uturn = road;
        }
        else if (is_turn_allowed || (!is_merged && !is_uturn_angle))
        { // Add roads that have allowed entry or not U-turns and not merged
            allowed_uturns_number += is_uturn_angle;

            // Adjust computed initial turn angle for non-U-turn road edge cases:
            // 1) use 0° or 360° if the road has 0° initial angle
            // 2) use turn angle if the smallest arc between turn and initial angles passes 0°
            const auto use_turn_angle = (turn_angle > 270 && initial_angle < 90) ||
                                        (turn_angle < 90 && initial_angle > 270);
            const auto adjusted_angle = is_uturn(initial_angle) ? (turn_angle > 180. ? 360. : 0.)
                                        : use_turn_angle        ? turn_angle
                                                                : initial_angle;
            pre_intersection_view.push_back({road, adjusted_angle});
        }
    }

    BOOST_ASSERT(uturn.eid != SPECIAL_EDGEID);
    if (uturn.entry_allowed || allowed_uturns_number == 0)
    { // Add the true U-turn if it is allowed or no other U-turns found
        BOOST_ASSERT(uturn.angle == 0.);
        pre_intersection_view.insert(pre_intersection_view.begin(), {uturn, 0});
    }

    // Order roads in counter-clockwise order starting from the U-turn edge in the OSM order
    std::stable_sort(
        pre_intersection_view.begin(),
        pre_intersection_view.end(),
        [](const auto &lhs, const auto &rhs)
        { return std::tie(lhs.second, lhs.first.angle) < std::tie(rhs.second, rhs.first.angle); });

    // Adjust perceived bearings to keep the initial OSM order with respect to the first edge
    for (auto curr = pre_intersection_view.begin(), next = std::next(curr);
         next != pre_intersection_view.end();
         ++curr, ++next)
    {
        // Check that the perceived angles order is the same as the initial OSM one
        if (next->first.angle < curr->first.angle)
        { // If the true bearing is out of the initial order (next before current) then
            // adjust the next road angle to keep the order. The adjustment angle is at most
            // 0.5° or a half-angle between the current angle and 360° to prevent overlapping
            const auto angle_adjustment =
                std::min(.5, util::restrictAngleToValidRange(360. - curr->first.angle) / 2.);
            next->first.angle =
                util::restrictAngleToValidRange(curr->first.angle + angle_adjustment);
        }
    }

    auto no_uturn = std::none_of(pre_intersection_view.begin(),
                                 pre_intersection_view.end(),
                                 [&is_uturn](const IntersectionViewDataWithAngle &road)
                                 { return is_uturn(road.first.angle); });
    // After all of this, if we now don't have a u-turn, let's add one to the intersection.
    // This is a hack to fix the triggered assertion ( see:
    // https://github.com/Project-OSRM/osrm-backend/issues/6218 ). Ideally we would fix this more
    // robustly, but this will require overhauling all of the intersection logic.
    if (no_uturn)
    {
        BOOST_ASSERT(!uturn.entry_allowed && allowed_uturns_number > 0);
        BOOST_ASSERT(uturn.angle == 0.);
        pre_intersection_view.insert(pre_intersection_view.begin(), {uturn, 0});
    }

    // Copy intersection view data
    IntersectionView intersection_view;
    intersection_view.reserve(pre_intersection_view.size());
    std::transform(pre_intersection_view.begin(),
                   pre_intersection_view.end(),
                   std::back_inserter(intersection_view),
                   [](const auto &road) { return road.first; });

    return intersection_view;
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
template <bool USE_CLOSE_COORDINATE>
IntersectionView getConnectedRoads(const util::NodeBasedDynamicGraph &graph,
                                   const EdgeBasedNodeDataContainer &node_data_container,
                                   const std::vector<util::Coordinate> &node_coordinates,
                                   const extractor::CompressedEdgeContainer &compressed_geometries,
                                   const RestrictionMap &node_restriction_map,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const TurnLanesIndexedArray &turn_lanes_data,
                                   const IntersectionEdge &incoming_edge)
{
    const auto intersection_node = graph.GetTarget(incoming_edge.edge);
    auto edge_geometries = getIntersectionOutgoingGeometries<USE_CLOSE_COORDINATE>(
        graph, compressed_geometries, node_coordinates, intersection_node);
    auto merged_edge_ids = std::unordered_set<EdgeID>();

    return getConnectedRoadsForEdgeGeometries(graph,
                                              node_data_container,
                                              node_restriction_map,
                                              barrier_nodes,
                                              turn_lanes_data,
                                              incoming_edge,
                                              edge_geometries,
                                              merged_edge_ids);
}

IntersectionView
getConnectedRoadsForEdgeGeometries(const util::NodeBasedDynamicGraph &graph,
                                   const EdgeBasedNodeDataContainer &node_data_container,
                                   const RestrictionMap &node_restriction_map,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const TurnLanesIndexedArray &turn_lanes_data,
                                   const IntersectionEdge &incoming_edge,
                                   const IntersectionEdgeGeometries &edge_geometries,
                                   const std::unordered_set<EdgeID> &merged_edge_ids)
{
    const auto intersection_node = graph.GetTarget(incoming_edge.edge);
    const auto &outgoing_edges = intersection::getOutgoingEdges(graph, intersection_node);

    // Add incoming edges with reversed bearings
    auto processed_edge_geometries = IntersectionEdgeGeometries(edge_geometries);
    const auto edges_number = processed_edge_geometries.size();
    processed_edge_geometries.resize(2 * edges_number);
    for (std::size_t index = 0; index < edges_number; ++index)
    {
        const auto &geometry = processed_edge_geometries[index];
        const auto remote_node = graph.GetTarget(geometry.eid);
        const auto incoming_edge = graph.FindEdge(remote_node, intersection_node);
        processed_edge_geometries[edges_number + index] = {
            incoming_edge,
            util::bearing::reverse(geometry.initial_bearing),
            util::bearing::reverse(geometry.perceived_bearing),
            geometry.segment_length};
    }

    // Enforce ordering of edges by IDs
    std::sort(processed_edge_geometries.begin(), processed_edge_geometries.end());

    return convertToIntersectionView(graph,
                                     node_data_container,
                                     node_restriction_map,
                                     barrier_nodes,
                                     processed_edge_geometries,
                                     turn_lanes_data,
                                     incoming_edge,
                                     outgoing_edges,
                                     merged_edge_ids);
}

template IntersectionView
getConnectedRoads<false>(const util::NodeBasedDynamicGraph &graph,
                         const EdgeBasedNodeDataContainer &node_data_container,
                         const std::vector<util::Coordinate> &node_coordinates,
                         const extractor::CompressedEdgeContainer &compressed_geometries,
                         const RestrictionMap &node_restriction_map,
                         const std::unordered_set<NodeID> &barrier_nodes,
                         const TurnLanesIndexedArray &turn_lanes_data,
                         const IntersectionEdge &incoming_edge);

template IntersectionView
getConnectedRoads<true>(const util::NodeBasedDynamicGraph &graph,
                        const EdgeBasedNodeDataContainer &node_data_container,
                        const std::vector<util::Coordinate> &node_coordinates,
                        const extractor::CompressedEdgeContainer &compressed_geometries,
                        const RestrictionMap &node_restriction_map,
                        const std::unordered_set<NodeID> &barrier_nodes,
                        const TurnLanesIndexedArray &turn_lanes_data,
                        const IntersectionEdge &incoming_edge);

IntersectionEdge skipDegreeTwoNodes(const util::NodeBasedDynamicGraph &graph, IntersectionEdge road)
{
    std::unordered_set<NodeID> visited_nodes;
    (void)visited_nodes;

    // Skip trivial nodes without generating the intersection in between, stop at the very first
    // intersection of degree > 2
    const auto starting_node = road.node;
    auto next_node = graph.GetTarget(road.edge);
    while (graph.GetOutDegree(next_node) == 2 && next_node != starting_node)
    {
        BOOST_ASSERT(visited_nodes.insert(next_node).second);
        const auto next_edge = graph.BeginEdges(next_node);
        road.edge = graph.GetTarget(next_edge) == road.node ? next_edge + 1 : next_edge;
        road.node = next_node;
        next_node = graph.GetTarget(road.edge);
    }

    return road;
}
} // namespace osrm::extractor::intersection
