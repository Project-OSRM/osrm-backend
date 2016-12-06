#include "extractor/guidance/intersection_generator.hpp"

#include "extractor/geojson_debug_policies.hpp"
#include "util/geojson_debug_logger.hpp"

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <functional> // mem_fn
#include <limits>
#include <numeric>
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

IntersectionView IntersectionGenerator::operator()(const NodeID from_node,
                                                   const EdgeID via_eid) const
{
    return GetConnectedRoads(from_node, via_eid, USE_HIGH_PRECISION_MODE);
}

IntersectionShape
IntersectionGenerator::ComputeIntersectionShape(const NodeID node_at_center_of_intersection,
                                                const boost::optional<NodeID> sorting_base,
                                                const bool use_low_precision_angles) const
{
    IntersectionShape intersection;
    // reserve enough items (+ the possibly missing u-turn edge)
    const auto intersection_degree = node_based_graph.GetOutDegree(node_at_center_of_intersection);
    intersection.reserve(intersection_degree);
    const util::Coordinate turn_coordinate = node_info_list[node_at_center_of_intersection];

    // number of lanes at the intersection changes how far we look down the road
    const auto edge_range = node_based_graph.GetAdjacentEdgeRange(node_at_center_of_intersection);
    const auto max_lanes_intersection = std::accumulate(
        edge_range.begin(),
        edge_range.end(),
        std::uint8_t{0},
        [this](const auto current_max, const auto current_eid) {
            return std::max(
                current_max,
                node_based_graph.GetEdgeData(current_eid).road_classification.GetNumberOfLanes());
        });

    for (const EdgeID edge_connected_to_intersection :
         node_based_graph.GetAdjacentEdgeRange(node_at_center_of_intersection))
    {
        BOOST_ASSERT(edge_connected_to_intersection != SPECIAL_EDGEID);
        const NodeID to_node = node_based_graph.GetTarget(edge_connected_to_intersection);
        double bearing = 0.;

        auto coordinates = coordinate_extractor.GetCoordinatesAlongRoad(
            node_at_center_of_intersection, edge_connected_to_intersection, !INVERT, to_node);

        const auto segment_length = util::coordinate_calculation::getLength(
            coordinates.begin(),
            coordinates.end(),
            util::coordinate_calculation::haversineDistance);

        const auto extract_coordinate = [&](const NodeID from_node,
                                            const EdgeID via_eid,
                                            const bool traversed_in_reverse,
                                            const NodeID to_node) {
            return (use_low_precision_angles || intersection_degree <= 2)
                       ? coordinate_extractor.GetCoordinateCloseToTurn(
                             from_node, via_eid, traversed_in_reverse, to_node)
                       : coordinate_extractor.ExtractRepresentativeCoordinate(
                             from_node,
                             via_eid,
                             traversed_in_reverse,
                             to_node,
                             max_lanes_intersection,
                             std::move(coordinates));
        };

        // we have to look down the road a bit to get the correct turn
        const auto coordinate_along_edge_leaving = extract_coordinate(
            node_at_center_of_intersection, edge_connected_to_intersection, !INVERT, to_node);

        bearing =
            util::coordinate_calculation::bearing(turn_coordinate, coordinate_along_edge_leaving);

        intersection.push_back({edge_connected_to_intersection, bearing, segment_length});
    }

    if (!intersection.empty())
    {
        const auto base_bearing = [&]() {
            if (sorting_base)
            {
                const auto itr =
                    std::find_if(intersection.begin(),
                                 intersection.end(),
                                 [&](const IntersectionShapeData &data) {
                                     return node_based_graph.GetTarget(data.eid) == *sorting_base;
                                 });
                if (itr != intersection.end())
                    return util::bearing::reverse(itr->bearing);
            }
            return util::bearing::reverse(intersection.begin()->bearing);
        }();
        std::sort(
            intersection.begin(), intersection.end(), makeCompareShapeDataByBearing(base_bearing));
    }
    return intersection;
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
IntersectionView IntersectionGenerator::GetConnectedRoads(const NodeID from_node,
                                                          const EdgeID via_eid,
                                                          const bool use_low_precision_angles) const
{
    // make sure the via-eid is valid
    BOOST_ASSERT([this](const NodeID from_node, const EdgeID via_eid) {
        const auto range = node_based_graph.GetAdjacentEdgeRange(from_node);
        return range.front() <= via_eid && via_eid <= range.back();
    }(from_node, via_eid));

    auto intersection = ComputeIntersectionShape(
        node_based_graph.GetTarget(via_eid), boost::none, use_low_precision_angles);
    return TransformIntersectionShapeIntoView(from_node, via_eid, std::move(intersection));
}

IntersectionGenerationParameters
IntersectionGenerator::SkipDegreeTwoNodes(const NodeID starting_node, const EdgeID via_edge) const
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

        query_node = next_node;
        query_edge = next_edge;

        if (!node_based_graph.GetEdgeData(query_edge)
                 .IsCompatibleTo(node_based_graph.GetEdgeData(next_edge)) ||
            node_based_graph.GetTarget(next_edge) == starting_node)
            break;
    }

    return {query_node, query_edge};
}

IntersectionView IntersectionGenerator::TransformIntersectionShapeIntoView(
    const NodeID previous_node,
    const EdgeID entering_via_edge,
    const IntersectionShape &intersection_shape) const
{
    // requires a copy of the intersection
    return TransformIntersectionShapeIntoView(previous_node,
                                              entering_via_edge,
                                              intersection_shape, // creates a copy
                                              intersection_shape, // reference to local
                                              {}); // empty vector of performed merges
}

IntersectionView IntersectionGenerator::TransformIntersectionShapeIntoView(
    const NodeID previous_node,
    const EdgeID entering_via_edge,
    const IntersectionShape &normalized_intersection,
    const IntersectionShape &intersection,
    const std::vector<IntersectionNormalizationOperation> &performed_merges) const
{
    const auto node_at_intersection = node_based_graph.GetTarget(entering_via_edge);

    // check if there is a single valid turn entering the current intersection
    const auto only_valid_turn = GetOnlyAllowedTurnIfExistent(previous_node, node_at_intersection);

    // barriers change our behaviour regarding u-turns
    const bool is_barrier_node = barrier_nodes.find(node_at_intersection) != barrier_nodes.end();

    const auto connect_to_previous_node = [this, previous_node](const IntersectionShapeData road) {
        return node_based_graph.GetTarget(road.eid) == previous_node;
    };

    // check which of the edges is the u-turn edge
    const auto uturn_edge_itr =
        std::find_if(intersection.begin(), intersection.end(), connect_to_previous_node);

    // there needs to be a connection, otherwise stuff went seriously wrong. Note that this is not
    // necessarily the same id as `entering_via_edge`.
    // In cases where parallel edges are present, we only remember the minimal edge. Both share
    // exactly the same coordinates, so the u-turn is still the best choice here.
    BOOST_ASSERT(uturn_edge_itr != intersection.end());

    const auto is_restricted = [&](const NodeID destination) {
        // check if we have a dedicated destination
        if (only_valid_turn && *only_valid_turn != destination)
            return true;

        // not explicitly forbidden
        return restriction_map.CheckIfTurnIsRestricted(
            previous_node, node_at_intersection, destination);
    };

    const auto is_allowed_turn = [&](const IntersectionShapeData &road) {
        const auto &road_data = node_based_graph.GetEdgeData(road.eid);
        const NodeID road_destination_node = node_based_graph.GetTarget(road.eid);
        // reverse edges are never valid turns because the resulting turn would look like this:
        // from_node --via_edge--> node_at_intersection <--onto_edge-- to_node
        // however we need this for capture intersection shape for incoming one-ways
        return !road_data.reversed &&
               // we are not turning over a barrier
               (!is_barrier_node || road_destination_node == previous_node) &&
               // don't allow restricted turns
               !is_restricted(road_destination_node);

    };

    // due to merging of roads, the u-turn might actually not be part of the intersection anymore
    const auto uturn_bearing = [&]() {
        const auto merge_entry = std::find_if(
            performed_merges.begin(), performed_merges.end(), [&uturn_edge_itr](const auto entry) {
                return entry.merged_eid == uturn_edge_itr->eid;
            });
        if (merge_entry != performed_merges.end())
        {
            const auto merged_into_id = merge_entry->into_eid;
            const auto merged_u_turn = std::find_if(
                normalized_intersection.begin(),
                normalized_intersection.end(),
                [&](const IntersectionShapeData &road) { return road.eid == merged_into_id; });
            BOOST_ASSERT(merged_u_turn != normalized_intersection.end());
            return util::bearing::reverse(merged_u_turn->bearing);
        }
        else
        {
            const auto uturn_edge_at_normalized_intersection_itr =
                std::find_if(normalized_intersection.begin(),
                             normalized_intersection.end(),
                             connect_to_previous_node);
            BOOST_ASSERT(uturn_edge_at_normalized_intersection_itr !=
                         normalized_intersection.end());
            return util::bearing::reverse(uturn_edge_at_normalized_intersection_itr->bearing);
        }
    }();

    IntersectionView intersection_view;
    intersection_view.reserve(normalized_intersection.size());
    std::transform(normalized_intersection.begin(),
                   normalized_intersection.end(),
                   std::back_inserter(intersection_view),
                   [&](const IntersectionShapeData &road) {
                       return IntersectionViewData(
                           road,
                           is_allowed_turn(road),
                           util::bearing::angleBetween(uturn_bearing, road.bearing));
                   });

    const auto uturn_edge_at_intersection_view_itr =
        std::find_if(intersection_view.begin(), intersection_view.end(), connect_to_previous_node);
    // number of found valid exit roads
    const auto valid_count =
        std::count_if(intersection_view.begin(),
                      intersection_view.end(),
                      [](const IntersectionViewData &road) { return road.entry_allowed; });
    // in general, we don't wan't to allow u-turns. If we don't look at a barrier, we have to check
    // for dead end streets. These are the only ones that we allow uturns for, next to barriers
    // (which are also kind of a dead end, but we don't have to check these again :))
    if (uturn_edge_at_intersection_view_itr != intersection_view.end() &&
        ((uturn_edge_at_intersection_view_itr->entry_allowed && !is_barrier_node &&
          valid_count != 1) ||
         valid_count == 0))
    {
        const auto allow_uturn_at_dead_end = [&]() {
            const auto &uturn_data = node_based_graph.GetEdgeData(uturn_edge_itr->eid);

            // we can't turn back onto oneway streets
            if (uturn_data.reversed)
                return false;

            // don't allow explicitly restricted turns
            if (is_restricted(previous_node))
                return false;

            // we define dead ends as roads that can only be entered via the possible u-turn
            const auto is_bidirectional = [&](const EdgeID entering_via_edge) {
                const auto to_node = node_based_graph.GetTarget(entering_via_edge);
                const auto reverse_edge = node_based_graph.FindEdge(to_node, node_at_intersection);
                BOOST_ASSERT(reverse_edge != SPECIAL_EDGEID);
                return !node_based_graph.GetEdgeData(reverse_edge).reversed;
            };

            const auto bidirectional_edges = [&]() {
                std::uint32_t count = 0;
                for (const auto eid : node_based_graph.GetAdjacentEdgeRange(node_at_intersection))
                    if (is_bidirectional(eid))
                        ++count;
                return count;
            }();

            // Checking for dead-end streets is kind of difficult. There is obvious dead ends
            // (single road connected)
            return bidirectional_edges <= 1;
        }();
        uturn_edge_at_intersection_view_itr->entry_allowed = allow_uturn_at_dead_end;
    }
    std::sort(std::begin(intersection_view),
              std::end(intersection_view),
              std::mem_fn(&IntersectionViewData::CompareByAngle));

    BOOST_ASSERT(intersection_view[0].angle >= 0. &&
                 intersection_view[0].angle < std::numeric_limits<double>::epsilon());

    return intersection_view;
}

boost::optional<NodeID>
IntersectionGenerator::GetOnlyAllowedTurnIfExistent(const NodeID coming_from_node,
                                                    const NodeID node_at_intersection) const
{
    // If only restrictions refer to invalid ways somewhere far away, we rather ignore the
    // restriction than to not route over the intersection at all.
    const auto only_restriction_to_node =
        restriction_map.CheckForEmanatingIsOnlyTurn(coming_from_node, node_at_intersection);
    if (only_restriction_to_node != SPECIAL_NODEID)
    {
        // if the mentioned node does not exist anymore, we don't return it. This checks for broken
        // turn restrictions
        for (const auto onto_edge : node_based_graph.GetAdjacentEdgeRange(node_at_intersection))
            if (only_restriction_to_node == node_based_graph.GetTarget(onto_edge))
                return only_restriction_to_node;
    }
    // Ignore broken only restrictions.
    return boost::none;
}

const CoordinateExtractor &IntersectionGenerator::GetCoordinateExtractor() const
{
    return coordinate_extractor;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
