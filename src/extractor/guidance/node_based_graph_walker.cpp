#include "extractor/guidance/node_based_graph_walker.hpp"
#include "util/coordinate_calculation.hpp"
#include <utility>

using osrm::util::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

// ---------------------------------------------------------------------------------
NodeBasedGraphWalker::NodeBasedGraphWalker(const util::NodeBasedDynamicGraph &node_based_graph,
                                           const IntersectionGenerator &intersection_generator)
    : node_based_graph(node_based_graph), intersection_generator(intersection_generator)
{
}

LengthLimitedCoordinateAccumulator::LengthLimitedCoordinateAccumulator(
    const extractor::guidance::CoordinateExtractor &coordinate_extractor,
    const util::NodeBasedDynamicGraph &node_based_graph,
    const double max_length)
    : coordinate_extractor(coordinate_extractor), node_based_graph(node_based_graph),
      max_length(max_length), accumulated_length(0)
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

    const auto length = util::coordinate_calculation::getLength(
        current_coordinates, util::coordinate_calculation::haversineDistance);

    // in case we get too many coordinates, we limit them to our desired length
    if (length + accumulated_length > max_length)
        current_coordinates = coordinate_extractor.TrimCoordinatesToLength(
            std::move(current_coordinates), max_length - accumulated_length);

    coordinates.insert(coordinates.end(), current_coordinates.begin(), current_coordinates.end());

    accumulated_length += length;
}

// ---------------------------------------------------------------------------------
SelectRoadByNameOnlyChoiceAndStraightness::SelectRoadByNameOnlyChoiceAndStraightness(
    const NameID desired_name_id, const bool requires_entry)
    : desired_name_id(desired_name_id), requires_entry(requires_entry)
{
}

boost::optional<EdgeID> SelectRoadByNameOnlyChoiceAndStraightness::
operator()(const NodeID /*nid*/,
           const EdgeID /*via_edge_id*/,
           const Intersection &intersection,
           const util::NodeBasedDynamicGraph &node_based_graph) const
{
    BOOST_ASSERT(!intersection.empty());
    const auto comparator = [this, &node_based_graph](const ConnectedRoad &lhs,
                                                      const ConnectedRoad &rhs) {
        // the score of an elemnt results in an ranking preferring valid entries, if required over
        // invalid
        // requested name_ids over non-requested
        // narrow deviations over non-narrow
        const auto score = [this, &node_based_graph](const ConnectedRoad &road) {
            double result_score = 0;
            // since angular deviation is limited by 0-180, we add 360 for invalid
            if (requires_entry && !road.entry_allowed)
                result_score += 360.;

            // 180 for undesired name-ids
            if (desired_name_id != node_based_graph.GetEdgeData(road.eid).name_id)
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
IntersectionFinderAccumulator::IntersectionFinderAccumulator(
    const std::uint8_t hop_limit, const IntersectionGenerator &intersection_generator)
    : hops(0), hop_limit(hop_limit), intersection_generator(intersection_generator)
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

    intersection = intersection_generator.GetConnectedRoads(from_node, via_edge);
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
