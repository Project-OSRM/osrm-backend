#include "extractor/intersection/node_based_graph_walker.hpp"
#include "extractor/intersection/intersection_analysis.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"

#include <utility>

using osrm::util::angularDeviation;

namespace osrm::extractor::intersection
{

// ---------------------------------------------------------------------------------
NodeBasedGraphWalker::NodeBasedGraphWalker(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const EdgeBasedNodeDataContainer &node_data_container,
    const std::vector<util::Coordinate> &node_coordinates,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const RestrictionMap &node_restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const TurnLanesIndexedArray &turn_lanes_data)
    : node_based_graph(node_based_graph), node_data_container(node_data_container),
      node_coordinates(node_coordinates), compressed_geometries(compressed_geometries),
      node_restriction_map(node_restriction_map), barrier_nodes(barrier_nodes),
      turn_lanes_data(turn_lanes_data)
{
}

LengthLimitedCoordinateAccumulator::LengthLimitedCoordinateAccumulator(
    const CoordinateExtractor &coordinate_extractor, const double max_length)
    : accumulated_length(0), coordinate_extractor(coordinate_extractor), max_length(max_length)
{
}

bool LengthLimitedCoordinateAccumulator::terminate() { return accumulated_length >= max_length; }

// update the accumulator
void LengthLimitedCoordinateAccumulator::update(const NodeID from_node,
                                                const EdgeID via_edge,
                                                const NodeID /*to_node*/)

{
    auto current_coordinates =
        coordinate_extractor.GetForwardCoordinatesAlongRoad(from_node, via_edge);

    const auto length =
        util::coordinate_calculation::getLength(current_coordinates.begin(),
                                                current_coordinates.end(),
                                                util::coordinate_calculation::greatCircleDistance);

    // in case we get too many coordinates, we limit them to our desired length
    if (length + accumulated_length > max_length)
        current_coordinates = coordinate_extractor.TrimCoordinatesToLength(
            std::move(current_coordinates), max_length - accumulated_length);

    coordinates.insert(coordinates.end(), current_coordinates.begin(), current_coordinates.end());

    accumulated_length += length;
    accumulated_length = std::min(accumulated_length, max_length);
}

// ---------------------------------------------------------------------------------
SelectRoadByNameOnlyChoiceAndStraightness::SelectRoadByNameOnlyChoiceAndStraightness(
    const NameID desired_name_id, const bool requires_entry)
    : desired_name_id(desired_name_id), requires_entry(requires_entry)
{
}

std::optional<EdgeID> SelectRoadByNameOnlyChoiceAndStraightness::operator()(
    const NodeID /*nid*/,
    const EdgeID /*via_edge_id*/,
    const IntersectionView &intersection,
    const util::NodeBasedDynamicGraph &node_based_graph,
    const EdgeBasedNodeDataContainer &node_data_container) const
{
    BOOST_ASSERT(!intersection.empty());
    const auto comparator = [&](const IntersectionViewData &lhs, const IntersectionViewData &rhs)
    {
        // the score of an elemnt results in an ranking preferring valid entries, if required over
        // invalid requested name_ids over non-requested narrow deviations over non-narrow
        const auto score = [&](const IntersectionViewData &road)
        {
            double result_score = 0;
            // since angular deviation is limited by 0-180, we add 360 for invalid
            if (requires_entry && !road.entry_allowed)
                result_score += 360.;

            // 180 for undesired name-ids
            if (desired_name_id !=
                node_data_container
                    .GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
                    .name_id)
                result_score += 180;

            return result_score + angularDeviation(road.angle, STRAIGHT_ANGLE);
        };

        return score(lhs) < score(rhs);
    };

    const auto min_element =
        std::min_element(std::next(std::begin(intersection)), std::end(intersection), comparator);

    if (min_element == intersection.end() || (requires_entry && !min_element->entry_allowed))
        return {};
    else
        return (*min_element).eid;
}

// ---------------------------------------------------------------------------------
SelectStraightmostRoadByNameAndOnlyChoice::SelectStraightmostRoadByNameAndOnlyChoice(
    const NameID desired_name_id,
    const double initial_bearing,
    const bool requires_entry,
    const bool stop_on_ambiguous_turns)
    : desired_name_id(desired_name_id), initial_bearing(initial_bearing),
      requires_entry(requires_entry), stop_on_ambiguous_turns(stop_on_ambiguous_turns)
{
}

std::optional<EdgeID> SelectStraightmostRoadByNameAndOnlyChoice::operator()(
    const NodeID /*nid*/,
    const EdgeID /*via_edge_id*/,
    const IntersectionView &intersection,
    const util::NodeBasedDynamicGraph &node_based_graph,
    const EdgeBasedNodeDataContainer &node_data_container) const
{
    BOOST_ASSERT(!intersection.empty());
    if (intersection.size() == 1)
        return {};

    const auto comparator = [&](const IntersectionViewData &lhs, const IntersectionViewData &rhs)
    {
        // the score of an elemnt results in an ranking preferring valid entries, if required over
        // invalid requested name_ids over non-requested narrow deviations over non-narrow
        const auto score = [&](const IntersectionViewData &road)
        {
            double result_score = 0;
            // since angular deviation is limited by 0-180, we add 360 for invalid
            if (requires_entry && !road.entry_allowed)
                result_score += 360.;

            // 180 for undesired name-ids
            if (desired_name_id !=
                node_data_container
                    .GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
                    .name_id)
                result_score += 180;

            return result_score + angularDeviation(road.angle, STRAIGHT_ANGLE);
        };

        return score(lhs) < score(rhs);
    };

    const auto count_desired_name = std::count_if(
        std::begin(intersection),
        std::end(intersection),
        [&](const auto &road)
        {
            return node_data_container
                       .GetAnnotation(node_based_graph.GetEdgeData(road.eid).annotation_data)
                       .name_id == desired_name_id;
        });
    if (count_desired_name > 2)
        return {};

    const auto min_element =
        std::min_element(std::next(std::begin(intersection)), std::end(intersection), comparator);

    const auto is_valid_choice = !requires_entry || min_element->entry_allowed;

    if (!is_valid_choice)
        return {};

    // only road exiting or continuing in the same general direction
    const auto has_valid_angle =
        ((intersection.size() == 2 ||
          intersection.findClosestTurn(STRAIGHT_ANGLE) == min_element) &&
         angularDeviation(min_element->angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE) &&
        angularDeviation(initial_bearing, min_element->perceived_bearing) < NARROW_TURN_ANGLE;

    if (has_valid_angle)
        return (*min_element).eid;

    // in some cases, stronger turns are appropriate. We allow turns of just a bit over 90 degrees,
    // if it's not a end of road situation. These angles come into play where roads split into dual
    // carriage-ways.
    //
    //            e - - f
    // a - - - - b
    //            c - - d
    //            |
    //            g
    //
    // is technically
    //
    //
    // a - - - - b (ce) - - (fg)
    //              |
    //              g
    const auto is_only_choice_with_same_name =
        count_desired_name <= 2 && //  <= in case we come from a bridge, otherwise we have a u-turn
                                   //  and the outgoing edge
        node_data_container
                .GetAnnotation(node_based_graph.GetEdgeData(min_element->eid).annotation_data)
                .name_id == desired_name_id &&
        angularDeviation(min_element->angle, STRAIGHT_ANGLE) < 100; // don't do crazy turns

    // do not allow major turns in the road, if other similar turns are present
    // e.g.a turn at the end of the road:
    //
    // a - - a - - a - b - b
    //             |
    //       a - - a
    //             |
    //             c
    //
    // Such a turn can never be part of a merge
    // We check if there is a similar turn to the other side. If such a turn exists, we consider the
    // continuation of the road not possible
    if (stop_on_ambiguous_turns &&
        util::angularDeviation(STRAIGHT_ANGLE, min_element->angle) > GROUP_ANGLE)
    {
        auto opposite = intersection.findClosestTurn(util::bearing::reverse(min_element->angle));
        auto opposite_deviation = util::angularDeviation(min_element->angle, opposite->angle);
        // d - - - - c - - - -e
        //           |
        //           |
        // a - - - - b
        // from b-c onto min_element d with opposite side e
        if (opposite_deviation > (180 - FUZZY_ANGLE_DIFFERENCE))
            return {};

        //           e
        //           |
        // a - - - - b - - - - -d
        // doing a left turn while straight is a choice
        auto const best = intersection.findClosestTurn(STRAIGHT_ANGLE);
        if (util::angularDeviation(best->angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
            return {};
    }

    return is_only_choice_with_same_name ? std::optional<EdgeID>(min_element->eid) : std::nullopt;
}

// ---------------------------------------------------------------------------------
IntersectionFinderAccumulator::IntersectionFinderAccumulator(
    const std::uint8_t hop_limit,
    const util::NodeBasedDynamicGraph &node_based_graph,
    const EdgeBasedNodeDataContainer &node_data_container,
    const std::vector<util::Coordinate> &node_coordinates,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const RestrictionMap &node_restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const TurnLanesIndexedArray &turn_lanes_data)
    : hops(0), hop_limit(hop_limit), node_based_graph(node_based_graph),
      node_data_container(node_data_container), node_coordinates(node_coordinates),
      compressed_geometries(compressed_geometries), node_restriction_map(node_restriction_map),
      barrier_nodes(barrier_nodes), turn_lanes_data(turn_lanes_data)
{
}

bool IntersectionFinderAccumulator::terminate()
{
    if (intersection.size() > 2 || hops == hop_limit)
    {
        hops = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void IntersectionFinderAccumulator::update(const NodeID from_node,
                                           const EdgeID via_edge,
                                           const NodeID /*to_node*/)
{
    ++hops;
    nid = from_node;
    via_edge_id = via_edge;

    intersection = intersection::getConnectedRoads<true>(node_based_graph,
                                                         node_data_container,
                                                         node_coordinates,
                                                         compressed_geometries,
                                                         node_restriction_map,
                                                         barrier_nodes,
                                                         turn_lanes_data,
                                                         {from_node, via_edge});
}

} // namespace osrm::extractor::intersection
