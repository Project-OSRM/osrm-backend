#include "extractor/intersection/intersection_analysis.hpp"

#include "util/assert.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"

#include "extractor/guidance/coordinate_extractor.hpp"

#include <boost/optional/optional_io.hpp>

namespace osrm
{
namespace extractor
{
namespace intersection
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
        // TODO: to use TurnAnalysis all outgoing edges are required, to be uncommented later
        // if (!graph.GetEdgeData(outgoing_edge).reversed)
        {
            result.push_back({intersection_node, outgoing_edge});
        }
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
                   [&node_coordinates](const auto &compressed_edge) {
                       return node_coordinates[compressed_edge.node_id];
                   });

    // filter duplicated coordinates
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

namespace
{
double findAngleBisector(double alpha, double beta)
{
    alpha *= M_PI / 180.;
    beta *= M_PI / 180.;
    const auto average =
        180. * std::atan2(std::sin(alpha) + std::sin(beta), std::cos(alpha) + std::cos(beta)) /
        M_PI;
    return std::fmod(average + 360., 360.);
}

double findClosestOppositeBearing(const IntersectionEdgeGeometries &edge_geometries,
                                  const double bearing)
{
    BOOST_ASSERT(!edge_geometries.empty());
    const auto min = std::min_element(
        edge_geometries.begin(),
        edge_geometries.end(),
        [bearing = util::bearing::reverse(bearing)](const auto &lhs, const auto &rhs) {
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

    using guidance::STRAIGHT_ANGLE;
    using guidance::MAXIMAL_ALLOWED_NO_TURN_DEVIATION;
    using util::bearing::angleBetween;
    using util::angularDeviation;

    const auto &lhs = edge_geometries[lhs_index];
    const auto &rhs = edge_geometries[rhs_index];
    BOOST_ASSERT(graph.GetEdgeData(lhs.edge).reversed != graph.GetEdgeData(rhs.edge).reversed);

    const auto &entry = graph.GetEdgeData(lhs.edge).reversed ? rhs : lhs;
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

bool isRoadsPairMergeable(const guidance::MergableRoadDetector &detector,
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
    return detector.IsDistinctFrom({llhs.edge, llhs.perceived_bearing, llhs.length},
                                   {lhs.edge, lhs.perceived_bearing, lhs.length}) &&
           detector.CanMergeRoad(intersection_node,
                                 {lhs.edge, lhs.perceived_bearing, lhs.length},
                                 {rhs.edge, rhs.perceived_bearing, rhs.length}) &&
           detector.IsDistinctFrom({rhs.edge, rhs.perceived_bearing, rhs.length},
                                   {rrhs.edge, rrhs.perceived_bearing, rrhs.length});
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

IntersectionEdgeGeometries
getIntersectionOutgoingGeometries(const util::NodeBasedDynamicGraph &graph,
                                  const extractor::CompressedEdgeContainer &compressed_geometries,
                                  const std::vector<util::Coordinate> &node_coordinates,
                                  const NodeID intersection_node)
{
    IntersectionEdgeGeometries edge_geometries;

    // TODO: keep CoordinateExtractor to reproduce bearings, simplify later
    const guidance::CoordinateExtractor coordinate_extractor(
        graph, compressed_geometries, node_coordinates);

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
            graph.GetOutDegree(intersection_node) <= 2
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
            geometry.begin(), geometry.end(), util::coordinate_calculation::haversineDistance);

        edge_geometries.push_back({outgoing_edge, initial_bearing, perceived_bearing, edge_length});
    }

    // TODO: remove to fix https://github.com/Project-OSRM/osrm-backend/issues/4704
    if (!edge_geometries.empty())
    { // Adjust perceived bearings to keep the initial order with respect to the first edge
        // Sort geometries by initial bearings
        std::sort(edge_geometries.begin(),
                  edge_geometries.end(),
                  [base_bearing = util::bearing::reverse(edge_geometries.front().initial_bearing)](
                      const auto &lhs, const auto &rhs) {
                      return (util::bearing::angleBetween(lhs.initial_bearing, base_bearing) <
                              util::bearing::angleBetween(rhs.initial_bearing, base_bearing)) ||
                             (lhs.initial_bearing == rhs.initial_bearing &&
                              util::bearing::angleBetween(lhs.perceived_bearing,
                                                          rhs.perceived_bearing) < 180.);
                  });

        // Make a bearings ordering functor
        const auto base_bearing = util::bearing::reverse(edge_geometries.front().perceived_bearing);
        const auto bearings_order = [base_bearing](const auto &lhs, const auto &rhs) {
            return util::bearing::angleBetween(lhs.perceived_bearing, base_bearing) <
                   util::bearing::angleBetween(rhs.perceived_bearing, base_bearing);
        };

        // Check the perceived bearings order is the same as the initial one
        for (auto curr = edge_geometries.begin(), next = std::next(curr);
             next != edge_geometries.end();
             ++curr, ++next)
        {
            if (bearings_order(*next, *curr))
            { // If the true bearing is out of the initial order (next before current) then
                // adjust the next bearing to keep the order. The adjustment angle is at most
                // 0.5° or a half-angle between the current bearing and the base bearing.
                // to prevent overlapping over base bearing + 360°.
                const auto angle_adjustment = std::min(
                    .5,
                    util::restrictAngleToValidRange(base_bearing - curr->perceived_bearing) / 2.);
                next->perceived_bearing =
                    util::restrictAngleToValidRange(curr->perceived_bearing + angle_adjustment);
            }
        }
    }

    return edge_geometries;
}
}

std::pair<IntersectionEdgeGeometries, std::unordered_set<EdgeID>>
getIntersectionGeometries(const util::NodeBasedDynamicGraph &graph,
                          const extractor::CompressedEdgeContainer &compressed_geometries,
                          const std::vector<util::Coordinate> &node_coordinates,
                          const guidance::MergableRoadDetector &detector,
                          const NodeID intersection_node)
{
    IntersectionEdgeGeometries edge_geometries = getIntersectionOutgoingGeometries(
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

                lhs.perceived_bearing = merge.second;
                rhs.perceived_bearing = merge.second;
                merged_edge_ids.insert(lhs.edge);
                merged_edge_ids.insert(rhs.edge);
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
            if (merged_edges[index] || edge_geometry.length > PRUNING_DISTANCE)
                continue;

            const auto neighbor_intersection_node = graph.GetTarget(edge_geometry.edge);

            const auto neighbor_geometries = getIntersectionOutgoingGeometries(
                graph, compressed_geometries, node_coordinates, neighbor_intersection_node);

            const auto neighbor_edges = neighbor_geometries.size();
            if (neighbor_edges <= 1)
                continue;

            const auto neighbor_curr = std::distance(
                neighbor_geometries.begin(),
                std::find_if(neighbor_geometries.begin(),
                             neighbor_geometries.end(),
                             [&graph, &intersection_node](const auto &road) {
                                 return graph.GetTarget(road.edge) == intersection_node;
                             }));
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
                    edge_geometry.perceived_bearing =
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
                    edge_geometry.perceived_bearing =
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
        const auto remote_node = graph.GetTarget(geometry.edge);
        const auto incoming_edge = graph.FindEdge(remote_node, intersection_node);
        edge_geometries[edges_number + index] = {incoming_edge,
                                                 util::bearing::reverse(geometry.initial_bearing),
                                                 util::bearing::reverse(geometry.perceived_bearing),
                                                 geometry.length};
    }

    // Enforce ordering of edges by IDs
    std::sort(edge_geometries.begin(), edge_geometries.end());

    return std::make_pair(edge_geometries, merged_edge_ids);
}

inline auto findEdge(const IntersectionEdgeGeometries &geometries, const EdgeID &edge)
{
    const auto it = std::lower_bound(
        geometries.begin(), geometries.end(), edge, [](const auto &geometry, const auto edge) {
            return geometry.edge < edge;
        });
    BOOST_ASSERT(it != geometries.end() && it->edge == edge);
    return it;
}

double findEdgeBearing(const IntersectionEdgeGeometries &geometries, const EdgeID &edge)
{
    return findEdge(geometries, edge)->perceived_bearing;
}

double findEdgeLength(const IntersectionEdgeGeometries &geometries, const EdgeID &edge)
{
    return findEdge(geometries, edge)->length;
}

template <typename RestrictionsRange>
bool isTurnRestricted(const RestrictionsRange &restrictions, const NodeID to)
{
    // Check turn restrictions to find a node that is the only allowed target when coming from a
    // node to an intersection
    //     d
    //     |
    // a - b - c  and `only_straight_on ab | bc would return `c` for `a,b`
    const auto is_only = std::find_if(restrictions.first,
                                      restrictions.second,
                                      [](const auto &pair) { return pair.second->is_only; });
    if (is_only != restrictions.second)
        return is_only->second->AsNodeRestriction().to != to;

    // Check if explicitly forbidden
    const auto no_turn =
        std::find_if(restrictions.first, restrictions.second, [&to](const auto &restriction) {
            return restriction.second->AsNodeRestriction().to == to;
        });

    return no_turn != restrictions.second;
}

bool isTurnAllowed(const util::NodeBasedDynamicGraph &graph,
                   const EdgeBasedNodeDataContainer &node_data_container,
                   const RestrictionMap &restriction_map,
                   const std::unordered_set<NodeID> &barrier_nodes,
                   const IntersectionEdgeGeometries &geometries,
                   const guidance::TurnLanesIndexedArray &turn_lanes_data,
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
                            [](const auto &lane) { return lane & guidance::TurnLaneType::uturn; }))
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

// TODO: the function adapts intersection geometry data to TurnAnalysis
guidance::IntersectionView
convertToIntersectionView(const util::NodeBasedDynamicGraph &graph,
                          const EdgeBasedNodeDataContainer &node_data_container,
                          const RestrictionMap &restriction_map,
                          const std::unordered_set<NodeID> &barrier_nodes,
                          const IntersectionEdgeGeometries &edge_geometries,
                          const guidance::TurnLanesIndexedArray &turn_lanes_data,
                          const IntersectionEdge &incoming_edge,
                          const IntersectionEdges &outgoing_edges,
                          const std::unordered_set<EdgeID> &merged_edges)
{
    const auto incoming_bearing = findEdgeBearing(edge_geometries, incoming_edge.edge);

    guidance::IntersectionView intersection_view;
    guidance::IntersectionViewData uturn{{SPECIAL_EDGEID, 0., 0.}, false, 0.};
    std::size_t allowed_uturns_number = 0;
    for (const auto &outgoing_edge : outgoing_edges)
    {
        const auto is_uturn = [](const auto angle) {
            return std::fabs(angle) < std::numeric_limits<double>::epsilon();
        };

        const auto edge_it = findEdge(edge_geometries, outgoing_edge.edge);
        const auto outgoing_bearing = edge_it->perceived_bearing;
        const auto initial_outgoing_bearing = edge_it->initial_bearing;
        const auto segment_length = edge_it->length;
        const auto turn_angle = std::fmod(
            std::round(util::bearing::angleBetween(incoming_bearing, outgoing_bearing) * 1e8) / 1e8,
            360.);
        const auto is_turn_allowed = intersection::isTurnAllowed(graph,
                                                                 node_data_container,
                                                                 restriction_map,
                                                                 barrier_nodes,
                                                                 edge_geometries,
                                                                 turn_lanes_data,
                                                                 incoming_edge,
                                                                 outgoing_edge);
        const auto is_uturn_angle = is_uturn(turn_angle);
        const auto is_merged = merged_edges.count(outgoing_edge.edge) != 0;

        guidance::IntersectionViewData road{
            {outgoing_edge.edge, outgoing_bearing, segment_length}, is_turn_allowed, turn_angle};

        if (graph.GetTarget(outgoing_edge.edge) == incoming_edge.node)
        { // Save the true U-turn road to add later if no allowed U-turns will be added
            uturn = road;
        }
        else if (is_turn_allowed || (!is_merged && !is_uturn_angle))
        { // Add roads that have allowed entry or not U-turns and not merged
            allowed_uturns_number += is_uturn_angle;

            intersection_view.push_back(road);
        }
    }

    BOOST_ASSERT(uturn.eid != SPECIAL_EDGEID);
    if (uturn.entry_allowed || allowed_uturns_number == 0)
    { // Add the true U-turn if it is allowed or no other U-turns found
        intersection_view.insert(intersection_view.begin(), uturn);
    }

    // Order roads in counter-clockwise order starting from the U-turn edge
    std::sort(intersection_view.begin(),
              intersection_view.end(),
              [](const auto &lhs, const auto &rhs) { return lhs.angle < rhs.angle; });

    return intersection_view;
}
}
}
}
