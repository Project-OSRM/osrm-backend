#include "guidance/sliproad_handler.hpp"
#include "guidance/constants.hpp"
#include "util/assert.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/name_announcements.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include <boost/assert.hpp>

using osrm::guidance::getTurnDirection;
using osrm::util::angularDeviation;

namespace osrm
{
namespace guidance
{

SliproadHandler::SliproadHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                 const extractor::EdgeBasedNodeDataContainer &node_data_container,
                                 const std::vector<util::Coordinate> &node_coordinates,
                                 const extractor::CompressedEdgeContainer &compressed_geometries,
                                 const extractor::RestrictionMap &node_restriction_map,
                                 const std::unordered_set<NodeID> &barrier_nodes,
                                 const extractor::TurnLanesIndexedArray &turn_lanes_data,
                                 const extractor::NameTable &name_table,
                                 const extractor::SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          node_data_container,
                          node_coordinates,
                          compressed_geometries,
                          node_restriction_map,
                          barrier_nodes,
                          turn_lanes_data,
                          name_table,
                          street_name_suffix_table),
      coordinate_extractor(node_based_graph, compressed_geometries, node_coordinates)
{
}

// The intersection has to connect a Sliproad, see the example scenario below:
// Intersection at `d`: Sliproad `bd` connecting `cd` and the road starting at `d`.
bool SliproadHandler::canProcess(const NodeID /*nid*/,
                                 const EdgeID /*via_eid*/,
                                 const Intersection &intersection) const
{
    return intersection.size() > 2;
}

// Detect sliproad b-d in the following example:
//
//       .
//       e
//       .
//       .
// a ... b .... c .
//       `      .
//         `    .
//           `  .
//              d
//              .
//
// ^ a nid
//    ^ ab source_edge_id
//       ^ b intersection
Intersection SliproadHandler::operator()(const NodeID /*nid*/,
                                         const EdgeID source_edge_id,
                                         Intersection intersection) const
{
    BOOST_ASSERT(intersection.size() > 2);

    // Potential splitting / start of a Sliproad (b)
    auto intersection_node_id = node_based_graph.GetTarget(source_edge_id);

    // Road index prefering non-sliproads (bc)
    auto obvious = getObviousIndexWithSliproads(source_edge_id, intersection, intersection_node_id);

    if (!obvious)
    {
        return intersection;
    }

    // Potential non-sliproad road (bc), leading to the intersection (c) the Sliproad (bd) shortcuts
    const auto &main_road = intersection[*obvious];

    // The road leading to the intersection (bc) has to continue from our source
    if (!roadContinues(source_edge_id, main_road.eid))
    {
        return intersection;
    }

    // Link-check for (bc) and later on (cd) which both are getting shortcutted by Sliproad
    const auto is_potential_link = [this, main_road](const ConnectedRoad &road) {
        if (!road.entry_allowed)
        {
            return false;
        }

        // Prevent from starting in or going onto a roundabout
        auto onto_roundabout = hasRoundaboutType(road.instruction);

        if (onto_roundabout)
        {
            return false;
        }

        // Narrow turn angle for road (bd) and guard against data issues (overlapping roads)
        auto is_narrow = angularDeviation(road.angle, STRAIGHT_ANGLE) <= 2 * NARROW_TURN_ANGLE;
        auto same_angle = angularDeviation(main_road.angle, road.angle) //
                          <= std::numeric_limits<double>::epsilon();

        if (!is_narrow || same_angle)
        {
            return false;
        }

        const auto &road_data = node_based_graph.GetEdgeData(road.eid).flags;

        auto is_roundabout = road_data.roundabout;

        if (is_roundabout)
        {
            return false;
        }

        return true;
    };

    if (!std::any_of(begin(intersection), end(intersection), is_potential_link))
    {
        return intersection;
    }

    // If the intersection is too far away, don't bother continuing
    if (nextIntersectionIsTooFarAway(intersection_node_id, main_road.eid))
    {
        return intersection;
    }

    // Try to find the intersection at (c) which the Sliproad shortcuts
    const auto main_road_intersection = getNextIntersection(intersection_node_id, main_road.eid);

    if (!main_road_intersection)
    {
        return intersection;
    }

    if (main_road_intersection->intersection.isDeadEnd())
    {
        return intersection;
    }

    // If we are at a traffic loop at the end of a road, don't consider it a sliproad
    if (intersection_node_id == main_road_intersection->node)
    {
        return intersection;
    }

    std::vector<NameID> target_road_name_ids;
    target_road_name_ids.reserve(main_road_intersection->intersection.size());

    for (const auto &road : main_road_intersection->intersection)
    {
        const auto target_annotation_id = node_based_graph.GetEdgeData(road.eid).annotation_data;
        const auto &target_data = node_data_container.GetAnnotation(target_annotation_id);
        target_road_name_ids.push_back(target_data.name_id);
    }

    auto sliproad_found = false;

    // For all roads at the main intersection except the UTurn road: check Sliproad scenarios.
    for (std::size_t road_index = 1, last = intersection.size(); road_index < last; ++road_index)
    {
        const auto index_left_of_main_road = (*obvious - 1) % intersection.size();
        const auto index_right_of_main_road = (*obvious + 1) % intersection.size();

        // Be strict and require the Sliproad to be either left or right of the main road.
        if (road_index != index_left_of_main_road && road_index != index_right_of_main_road)
            continue;

        auto &sliproad = intersection[road_index]; // this is what we're checking and assigning to
        const auto &sliproad_edge_data = node_based_graph.GetEdgeData(sliproad.eid);

        // Intersection is orderd: 0 is UTurn, then from sharp right to sharp left.
        // We already have an obvious index (bc) for going straight-ish.
        const auto is_right_sliproad_turn = road_index < *obvious;
        const auto is_left_sliproad_turn = road_index > *obvious;

        // Road at the intersection the main road leads onto which the sliproad arrives onto
        const auto crossing_road = [&] {
            if (is_left_sliproad_turn)
                return main_road_intersection->intersection.getLeftmostRoad();

            BOOST_ASSERT_MSG(is_right_sliproad_turn,
                             "Sliproad is neither a left nor right of obvious main road");
            return main_road_intersection->intersection.getRightmostRoad();
        }();

        const auto &crossing_road_data = node_based_graph.GetEdgeData(crossing_road.eid);

        // The crossing road at the main road's intersection must not be incoming-only
        if (crossing_road_data.reversed)
        {
            continue;
        }

        // Discard service and other low priority roads - never Sliproad candidate
        if (sliproad_edge_data.flags.road_classification.IsLowPriorityRoadClass())
        {
            continue;
        }

        // Incoming-only can never be a Sliproad
        if (sliproad_edge_data.reversed)
        {
            continue;
        }

        // This is what we know so far:
        //
        //       .
        //       e
        //       .
        //       .
        // a ... b .... c .   < `main_road_intersection` is intersection at `c`
        //       `      .
        //         `    .
        //           `  .
        //              d     < `target_intersection` is intersection at `d`
        //              .       `sliproad_edge_target` is node `d`
        //              e
        //
        //
        //          ^ `sliproad` is `bd`
        //       ^ `intersection` is intersection at `b`

        if (!is_potential_link(sliproad))
        {
            continue;
        }

        // The Sliproad graph edge - in the following we use the graph walker to
        // adjust this edge forward jumping over artificial intersections.
        auto sliproad_edge = sliproad.eid;

        // Starting out at the intersection and going onto the Sliproad we skip artificial
        // degree two intersections and limit the max hop count in doing so.
        extractor::intersection::IntersectionFinderAccumulator intersection_finder{
            10,
            node_based_graph,
            node_data_container,
            node_coordinates,
            compressed_geometries,
            node_restriction_map,
            barrier_nodes,
            turn_lanes_data};
        const extractor::intersection::SkipTrafficSignalBarrierRoadSelector road_selector{};
        (void)graph_walker.TraverseRoad(intersection_node_id, // start node
                                        sliproad_edge,        // onto edge
                                        intersection_finder,  // accumulator
                                        road_selector);       // selector

        sliproad_edge = intersection_finder.via_edge_id;
        const auto target_intersection = intersection_finder.intersection;
        if (target_intersection.isDeadEnd())
            continue;

        const auto find_valid = [](const extractor::intersection::IntersectionView &view) {
            // according to our current sliproad idea, there should only be one valid turn
            auto itr = std::find_if(
                view.begin(), view.end(), [](const auto &road) { return road.entry_allowed; });
            BOOST_ASSERT(itr != view.end());
            return itr;
        };

        // require all to be same mode, don't allow changes
        if (!allSameMode(source_edge_id, sliproad.eid, find_valid(target_intersection)->eid))
            continue;

        // Constrain the sliproad's target intersection to 1 or 2 sliproads, outgoing road
        // and incoming one from the main intersection
        if (target_intersection.size() < 3 || target_intersection.size() > 4)
        {
            continue;
        }

        const NodeID sliproad_edge_target = node_based_graph.GetTarget(sliproad_edge);

        // Distinct triangle nodes `bcd`
        if (intersection_node_id == main_road_intersection->node ||
            intersection_node_id == sliproad_edge_target ||
            main_road_intersection->node == sliproad_edge_target)
        {
            continue;
        }

        if (target_intersection.size() == 4)
        {
            // Handle target intersections at `d` with 4 roads
            //
            //              | `main_road_intersection` is intersection at `c`
            //              v
            // a ... b .... c .... e  <- fo
            //        `     .     '
            //          `   .   '
            //            ` . '
            //              d     < `target_intersection` is intersection at `d`
            //              |
            // Conditions for road `bd` to be a sliproad:
            //  - target_intersection at `d` has 4 roads
            //  - main_road_intersection at `c` has at least 3 roads
            //  - target nodes of `db` and `cd` roads is the same node `d`
            //  - target nodes of `ce` and `de` roads is the same node `e`
            //  - angle `bde` is sharp

            // Check `c` has at least 3 roads at `c` and roads `bd` and `cd` share the node `d`
            if (main_road_intersection->intersection.size() < 3 ||
                sliproad_edge_target != node_based_graph.GetTarget(crossing_road.eid))
            {
                continue;
            }

            // Find a road at `d` that shares the same node `e` with `ce` and ∠ `bde` is sharp
            auto next_to_crossing_idx =
                is_left_sliproad_turn ? main_road_intersection->intersection.size() - 2 : 2;
            auto next_to_crossing_road = main_road_intersection->intersection[next_to_crossing_idx];
            auto next_to_crossing_node = node_based_graph.GetTarget(next_to_crossing_road.eid);
            auto found_common_node = std::find_if(
                begin(target_intersection), end(target_intersection), [&](const auto &road) {
                    if (next_to_crossing_node == node_based_graph.GetTarget(road.eid))
                    {
                        auto direction = getTurnDirection(road.angle);
                        return direction == DirectionModifier::SharpRight ||
                               direction == DirectionModifier::SharpLeft;
                    }
                    return false;
                });
            if (found_common_node == target_intersection.end())
                continue;
        }

        // If the sliproad candidate is a through street, we cannot handle it as a sliproad.
        auto sliproad_in_target_intersection =
            std::find_if(begin(target_intersection),
                         end(target_intersection),
                         [&](const auto &road) { return road.eid == sliproad_edge; });
        if (sliproad_in_target_intersection != target_intersection.end())
        {
            auto index_of_sliproad_in_target_intersection =
                sliproad_in_target_intersection - target_intersection.begin();

            if (isThroughStreet<extractor::intersection::IntersectionView>(
                    index_of_sliproad_in_target_intersection,
                    target_intersection,
                    node_based_graph,
                    node_data_container,
                    name_table,
                    street_name_suffix_table))
            {
                continue;
            }
        }

        // The turn off of the Sliproad has to be obvious and a narrow turn and must not be a
        // roundabout
        {
            const auto index = findObviousTurn(sliproad_edge, target_intersection);

            if (index == 0)
            {
                continue;
            }

            const auto onto = target_intersection[index];
            const auto angle_deviation = angularDeviation(onto.angle, STRAIGHT_ANGLE);
            const auto is_narrow_turn = angle_deviation <= 2 * NARROW_TURN_ANGLE;

            if (!is_narrow_turn)
            {
                continue;
            }

            const auto &onto_data = node_based_graph.GetEdgeData(onto.eid).flags;

            if (onto_data.roundabout)
            {
                continue;
            }
        }

        // Check for curvature. Depending on the turn's direction at `c`. Scenario for right turn:
        //
        // a ... b .... c .   a ... b .... c .   a ... b .... c .
        //       `      .           `  .   .             .    .
        //         `    .                . .           .      .
        //           `  .                 ..             .    .
        //              d                  d                . d
        //
        //                    Sliproad           Not a Sliproad
        {
            const NodeID start = intersection_node_id; // b
            const EdgeID edge = sliproad_edge;         // bd

            const auto coords = coordinate_extractor.GetForwardCoordinatesAlongRoad(start, edge);

            // due to filtering of duplicated coordinates, we can end up with empty segments.
            // this can only happen as long as
            // https://github.com/Project-OSRM/osrm-backend/issues/3470 persists
            if (coords.size() < 2)
                continue;
            BOOST_ASSERT(coords.size() >= 2);

            // Now keep start and end coordinate fix and check for curvature
            const auto start_coord = coords.front();
            const auto end_coord = coords.back();

            const auto first = std::begin(coords) + 1;
            const auto last = std::end(coords) - 1;

            auto snuggles = false;

            using namespace util::coordinate_calculation;

            // In addition, if it's a right/left turn we expect the rightmost/leftmost
            // turn at `c` to be more than a minimal angle (40°) for a Sliproad scenario.
            double deviation_from_straight = 0;

            if (is_right_sliproad_turn)
            {
                snuggles = std::all_of(first, last, [=](auto each) { //
                    return !isCCW(start_coord, each, end_coord);
                });

                const auto rightmost = main_road_intersection->intersection.getRightmostRoad();
                deviation_from_straight = angularDeviation(rightmost.angle, STRAIGHT_ANGLE);
            }
            else if (is_left_sliproad_turn)
            {
                snuggles = std::all_of(first, last, [=](auto each) { //
                    return isCCW(start_coord, each, end_coord);
                });

                const auto leftmost = main_road_intersection->intersection.getLeftmostRoad();
                deviation_from_straight = angularDeviation(leftmost.angle, STRAIGHT_ANGLE);
            }

            // The data modelling for small Sliproads is not reliable enough.
            // Only check for curvature and ~90 degree when it makes sense to do so.
            const constexpr auto MIN_LENGTH = 3.;

            const auto length = haversineDistance(node_coordinates[intersection_node_id],
                                                  node_coordinates[main_road_intersection->node]);

            const double minimal_crossroad_angle_of_intersection = 40.;

            if (length >= MIN_LENGTH)
            {
                if (!snuggles)
                {
                    continue;
                }

                // Check sliproads with skew main intersections
                if (deviation_from_straight > 180. - minimal_crossroad_angle_of_intersection &&
                    !node_based_graph.GetEdgeData(sliproad.eid)
                         .flags.road_classification.IsLinkClass())
                {
                    continue;
                }
            }
        }

        // Check for area under triangle `bdc`.
        //
        // a ... b .... c .
        //       `      .
        //         `    .
        //           `  .
        //              d
        //
        const auto area_threshold =
            std::pow(scaledThresholdByRoadClass(MAX_SLIPROAD_THRESHOLD,
                                                sliproad_edge_data.flags.road_classification),
                     2.);

        if (!isValidSliproadArea(area_threshold,
                                 intersection_node_id,
                                 main_road_intersection->node,
                                 sliproad_edge_target))
        {
            continue;
        }

        // Check all roads at `d` if one is connected to `c`, is so `bd` is Sliproad.
        for (const auto &candidate_road : target_intersection)
        {
            const auto &candidate_data = node_data_container.GetAnnotation(
                node_based_graph.GetEdgeData(candidate_road.eid).annotation_data);

            // Name mismatch: check roads at `c` and `d` for same name
            const auto name_mismatch = [&](const NameID road_name_id) {
                return util::guidance::requiresNameAnnounced(road_name_id,              //
                                                             candidate_data.name_id,    //
                                                             name_table,                //
                                                             street_name_suffix_table); //
            };

            const auto candidate_road_name_mismatch = std::all_of(begin(target_road_name_ids), //
                                                                  end(target_road_name_ids),   //
                                                                  name_mismatch);              //

            if (candidate_road_name_mismatch)
            {
                continue;
            }

            // Check if main road -> sliproad (non-link) -> candidate road requires two name
            // announcements then don't suppress one announcement via sliproad handler
            const auto main_road_name_id =
                node_data_container
                    .GetAnnotation(node_based_graph.GetEdgeData(main_road.eid).annotation_data)
                    .name_id;
            const auto main_road_name_empty = name_table.GetNameForID(main_road_name_id).empty();
            const auto &sliproad_annotation =
                node_data_container.GetAnnotation(sliproad_edge_data.annotation_data);
            const auto sliproad_name_empty =
                name_table.GetNameForID(sliproad_annotation.name_id).empty();
            const auto candidate_road_name_empty =
                name_table.GetNameForID(candidate_data.name_id).empty();
            if (!sliproad_edge_data.flags.road_classification.IsLinkClass() &&
                !sliproad_name_empty && !main_road_name_empty && !candidate_road_name_empty &&
                util::guidance::requiresNameAnnounced(main_road_name_id,
                                                      sliproad_annotation.name_id,
                                                      name_table,
                                                      street_name_suffix_table) &&
                util::guidance::requiresNameAnnounced(sliproad_annotation.name_id,
                                                      candidate_data.name_id,
                                                      name_table,
                                                      street_name_suffix_table))
            {
                continue;
            }

            // If the Sliproad `bd` is a link, `bc` and `cd` must not be links.
            if (!isValidSliproadLink(sliproad, main_road, candidate_road))
            {
                continue;
            }

            // Check that the cross-road `candidate_road_target` that starts at `d` ends at
            // main intersection node `c` or has a common node `e` with a cross-road from `c`
            // a ... b .... c                a ... b .... c
            //       `      .                      `      .
            //         `    .                        `    e...
            //           `  .                          `  .
            //              d                             d
            //
            const auto candidate_road_target = node_based_graph.GetTarget(candidate_road.eid);
            if ((candidate_road_target == main_road_intersection->node) ||
                (candidate_road_target == node_based_graph.GetTarget(crossing_road.eid) &&
                 util::bearing::angleBetween(candidate_road.perceived_bearing,
                                             crossing_road.perceived_bearing) <
                     FUZZY_ANGLE_DIFFERENCE &&
                 (getTurnDirection(candidate_road.angle) == DirectionModifier::SharpRight ||
                  getTurnDirection(candidate_road.angle) == DirectionModifier::SharpLeft)))
            {
                sliproad.instruction.type = TurnType::Sliproad;
                sliproad_found = true;
                break;
            }
            else
            {
                const auto skip_traffic_light_intersection =
                    extractor::intersection::getConnectedRoads<false>(
                        node_based_graph,
                        node_data_container,
                        node_coordinates,
                        compressed_geometries,
                        node_restriction_map,
                        barrier_nodes,
                        turn_lanes_data,
                        {node_based_graph.GetTarget(sliproad_edge), candidate_road.eid});
                if (skip_traffic_light_intersection.isTrafficSignalOrBarrier() &&
                    node_based_graph.GetTarget(skip_traffic_light_intersection[1].eid) ==
                        main_road_intersection->node)
                {

                    sliproad.instruction.type = TurnType::Sliproad;
                    sliproad_found = true;
                    break;
                }
            }
        }
    }

    // Now in case we found a Sliproad and assigned the corresponding type to the road,
    // it could be that the intersection from which the Sliproad splits off was a Fork before.
    // In those cases the obvious non-Sliproad is now obvious and we discard the Fork turn type.
    if (sliproad_found && main_road.instruction.type == TurnType::Fork)
    {
        const auto &main_data = node_based_graph.GetEdgeData(main_road.eid);
        const auto &main_annotation = node_data_container.GetAnnotation(main_data.annotation_data);
        if (isSameName(source_edge_id, main_road.eid))
        {
            if (angularDeviation(main_road.angle, STRAIGHT_ANGLE) < 5)
                intersection[*obvious].instruction.type = TurnType::Suppressed;
            else
                intersection[*obvious].instruction.type = TurnType::Continue;
            intersection[*obvious].instruction.direction_modifier =
                getTurnDirection(intersection[*obvious].angle);
        }
        else if (!name_table.GetNameForID(main_annotation.name_id).empty())
        {
            intersection[*obvious].instruction.type = TurnType::NewName;
            intersection[*obvious].instruction.direction_modifier =
                getTurnDirection(intersection[*obvious].angle);
        }
        else
        {
            intersection[*obvious].instruction.type = TurnType::Suppressed;
        }
    }

    return intersection;
}

// Implementation details

boost::optional<std::size_t> SliproadHandler::getObviousIndexWithSliproads(
    const EdgeID from, const Intersection &intersection, const NodeID at) const
{
    BOOST_ASSERT(from != SPECIAL_EDGEID);
    BOOST_ASSERT(at != SPECIAL_NODEID);

    // If a turn is obvious without taking Sliproads into account use this
    const auto index = findObviousTurn(from, intersection);

    if (index != 0)
    {
        return boost::make_optional(index);
    }

    // Otherwise check if the road is forking into two and one of them is a Sliproad;
    // then the non-Sliproad is the obvious one.
    if (intersection.size() != 3)
    {
        return boost::none;
    }

    const auto forking = intersection[1].instruction.type == TurnType::Fork &&
                         intersection[2].instruction.type == TurnType::Fork;

    if (!forking)
    {
        return boost::none;
    }

    const auto first = getNextIntersection(at, intersection.getRightmostRoad().eid);
    const auto second = getNextIntersection(at, intersection.getLeftmostRoad().eid);

    if (!first || !second)
    {
        return boost::none;
    }

    if (first->intersection.isDeadEnd() || second->intersection.isDeadEnd())
    {
        return boost::none;
    }

    // In case of loops at the end of the road, we will arrive back at the intersection
    // itself. If that is the case, the road is obviously not a sliproad.
    if (canBeTargetOfSliproad(first->intersection) && at != second->node)
    {
        return boost::make_optional(std::size_t{2});
    }

    if (canBeTargetOfSliproad(second->intersection) && at != first->node)
    {
        return boost::make_optional(std::size_t{1});
    }

    return boost::none;
}

bool SliproadHandler::nextIntersectionIsTooFarAway(const NodeID start, const EdgeID onto) const
{
    BOOST_ASSERT(start != SPECIAL_NODEID);
    BOOST_ASSERT(onto != SPECIAL_EDGEID);

    // Base max distance threshold on the current road class we're on
    const auto &data = node_based_graph.GetEdgeData(onto).flags;
    const auto threshold = scaledThresholdByRoadClass(MAX_SLIPROAD_THRESHOLD, // <- scales down
                                                      data.road_classification);

    extractor::intersection::DistanceToNextIntersectionAccumulator accumulator{
        coordinate_extractor, node_based_graph, threshold};
    const extractor::intersection::SkipTrafficSignalBarrierRoadSelector selector{};

    (void)graph_walker.TraverseRoad(start, onto, accumulator, selector);

    return accumulator.too_far_away;
}

bool SliproadHandler::roadContinues(const EdgeID current, const EdgeID next) const
{
    const auto &current_data =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(current).annotation_data);
    const auto &next_data =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(next).annotation_data);

    auto same_road_category = node_based_graph.GetEdgeData(current).flags.road_classification ==
                              node_based_graph.GetEdgeData(next).flags.road_classification;
    auto same_travel_mode = current_data.travel_mode == next_data.travel_mode;

    auto same_name = current_data.name_id != EMPTY_NAMEID && //
                     next_data.name_id != EMPTY_NAMEID &&    //
                     !util::guidance::requiresNameAnnounced(current_data.name_id,
                                                            next_data.name_id,
                                                            name_table,
                                                            street_name_suffix_table); //

    const auto continues = same_road_category && same_travel_mode && same_name;
    return continues;
}

bool SliproadHandler::isValidSliproadArea(const double max_area,
                                          const NodeID a,
                                          const NodeID b,
                                          const NodeID c) const

{
    using namespace util::coordinate_calculation;

    const auto first = node_coordinates[a];
    const auto second = node_coordinates[b];
    const auto third = node_coordinates[c];

    const auto length = haversineDistance(first, second);
    const auto heigth = haversineDistance(second, third);

    const auto area = (length * heigth) / 2.;

    // Everything below is data issue - there are some weird situations where
    // nodes are really close to each other and / or tagging ist just plain off.
    const constexpr auto MIN_SLIPROAD_AREA = 3.;

    if (area < MIN_SLIPROAD_AREA || area > max_area)
    {
        return false;
    }

    return true;
}

bool SliproadHandler::isValidSliproadLink(
    const extractor::intersection::IntersectionViewData &sliproad,
    const extractor::intersection::IntersectionViewData & /*first*/,
    const extractor::intersection::IntersectionViewData &second) const
{
    // If the Sliproad is not a link we don't care
    const auto &sliproad_data = node_based_graph.GetEdgeData(sliproad.eid).flags;
    if (!sliproad_data.road_classification.IsLinkClass())
    {
        return true;
    }

    // and the second road coming from the intersection we shortcut must be a non-link
    const auto &second_road_data = node_based_graph.GetEdgeData(second.eid).flags;
    if (second_road_data.road_classification.IsLinkClass())
    {
        return false;
    }

    return true;
}

bool SliproadHandler::allSameMode(const EdgeID from,
                                  const EdgeID sliproad_candidate,
                                  const EdgeID target_road) const
{
    const auto &from_annotation =
        node_data_container.GetAnnotation(node_based_graph.GetEdgeData(from).annotation_data);
    const auto &sliproad_annotation = node_data_container.GetAnnotation(
        node_based_graph.GetEdgeData(sliproad_candidate).annotation_data);
    const auto &target_annotation = node_data_container.GetAnnotation(
        node_based_graph.GetEdgeData(target_road).annotation_data);

    return (from_annotation.travel_mode == sliproad_annotation.travel_mode) &&
           (target_annotation.travel_mode == sliproad_annotation.travel_mode);
}

bool SliproadHandler::canBeTargetOfSliproad(
    const extractor::intersection::IntersectionView &intersection)
{
    // Example to handle:
    //       .
    // a . . b .
    //  `    .
    //    `  .
    //       c    < intersection
    //       .
    //

    // One outgoing two incoming
    if (intersection.size() != 3)
    {
        return false;
    }

    const auto backwards = intersection[0].entry_allowed;
    const auto multiple_allowed = intersection[1].entry_allowed && intersection[2].entry_allowed;

    if (backwards || multiple_allowed)
    {
        return false;
    }

    return true;
}

double
SliproadHandler::scaledThresholdByRoadClass(const double max_threshold,
                                            const extractor::RoadClassification &classification)
{
    double factor = 1.0;

    switch (classification.GetPriority())
    {
    case extractor::RoadPriorityClass::MOTORWAY:
        factor = 1.0;
        break;
    case extractor::RoadPriorityClass::TRUNK:
        factor = 0.8;
        break;
    case extractor::RoadPriorityClass::PRIMARY:
        factor = 0.8;
        break;
    case extractor::RoadPriorityClass::SECONDARY:
        factor = 0.6;
        break;
    case extractor::RoadPriorityClass::TERTIARY:
        factor = 0.5;
        break;
    case extractor::RoadPriorityClass::MAIN_RESIDENTIAL:
        factor = 0.4;
        break;
    case extractor::RoadPriorityClass::SIDE_RESIDENTIAL:
        factor = 0.3;
        break;
    case extractor::RoadPriorityClass::MOTORWAY_LINK:
    case extractor::RoadPriorityClass::TRUNK_LINK:
    case extractor::RoadPriorityClass::PRIMARY_LINK:
    case extractor::RoadPriorityClass::SECONDARY_LINK:
    case extractor::RoadPriorityClass::TERTIARY_LINK:
    case extractor::RoadPriorityClass::LINK_ROAD:
        factor = 0.3;
        break;
    case extractor::RoadPriorityClass::CONNECTIVITY:
        factor = 0.1;
        break;

    // What
    case extractor::RoadPriorityClass::BIKE_PATH:
    case extractor::RoadPriorityClass::FOOT_PATH:
    default:
        factor = 0.1;
    }

    const auto scaled = max_threshold * factor;

    BOOST_ASSERT(scaled <= max_threshold);
    return scaled;
}

} // namespace guidance
} // namespace osrm
