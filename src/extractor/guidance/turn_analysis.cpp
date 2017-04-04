#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/road_classification.hpp"

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include <cstddef>
#include <set>
#include <unordered_set>
#include <utility>

using osrm::extractor::guidance::getTurnDirection;

namespace osrm
{
namespace extractor
{
namespace guidance
{

using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

bool requiresAnnouncement(const EdgeData &from, const EdgeData &to)
{
    return !from.CanCombineWith(to);
}

TurnAnalysis::TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                           const std::vector<util::Coordinate> &coordinates,
                           const RestrictionMap &restriction_map,
                           const std::unordered_set<NodeID> &barrier_nodes,
                           const CompressedEdgeContainer &compressed_edge_container,
                           const util::NameTable &name_table,
                           const SuffixTable &street_name_suffix_table,
                           const ProfileProperties &profile_properties)
    : node_based_graph(node_based_graph),
      intersection_generator(
          node_based_graph, restriction_map, barrier_nodes, coordinates, compressed_edge_container),
      intersection_normalizer(node_based_graph,
                              coordinates,
                              name_table,
                              street_name_suffix_table,
                              intersection_generator),
      roundabout_handler(node_based_graph,
                         coordinates,
                         compressed_edge_container,
                         name_table,
                         street_name_suffix_table,
                         profile_properties,
                         intersection_generator),
      motorway_handler(node_based_graph,
                       coordinates,
                       name_table,
                       street_name_suffix_table,
                       intersection_generator),
      turn_handler(node_based_graph,
                   coordinates,
                   name_table,
                   street_name_suffix_table,
                   intersection_generator),
      sliproad_handler(intersection_generator,
                       node_based_graph,
                       coordinates,
                       name_table,
                       street_name_suffix_table),
      suppress_mode_handler(intersection_generator,
                            node_based_graph,
                            coordinates,
                            name_table,
                            street_name_suffix_table)
{
}

Intersection TurnAnalysis::operator()(const NodeID node_prior_to_intersection,
                                      const EdgeID entering_via_edge) const
{
    TurnAnalysis::ShapeResult shape_result =
        ComputeIntersectionShapes(node_based_graph.GetTarget(entering_via_edge));

    // assign valid flags to normalized_shape
    const auto intersection_view = intersection_generator.TransformIntersectionShapeIntoView(
        node_prior_to_intersection,
        entering_via_edge,
        shape_result.annotated_normalized_shape.normalized_shape,
        shape_result.intersection_shape,
        shape_result.annotated_normalized_shape.performed_merges);

    // assign the turn types to the intersection
    return AssignTurnTypes(node_prior_to_intersection, entering_via_edge, intersection_view);
}

Intersection TurnAnalysis::AssignTurnTypes(const NodeID node_prior_to_intersection,
                                           const EdgeID entering_via_edge,
                                           const IntersectionView &intersection_view) const
{
    // Roundabouts are a main priority. If there is a roundabout instruction present, we process the
    // turn as a roundabout

    // the following lines create a partly invalid intersection object. We might want to refactor
    // this at some point
    Intersection intersection;
    intersection.reserve(intersection_view.size());
    std::transform(intersection_view.begin(),
                   intersection_view.end(),
                   std::back_inserter(intersection),
                   [&](const IntersectionViewData &data) {
                       return ConnectedRoad(data,
                                            {TurnType::Invalid, DirectionModifier::UTurn},
                                            INVALID_LANE_DATAID);
                   });

    // Suppress turns on ways between mode types that do not need guidance, think ferry routes.
    // This handler has to come first and when it triggers we're done with the intersection: there's
    // nothing left to be done once we suppressed instructions on such routes. Exit early.
    if (suppress_mode_handler.canProcess(
            node_prior_to_intersection, entering_via_edge, intersection))
    {
        intersection = suppress_mode_handler(
            node_prior_to_intersection, entering_via_edge, std::move(intersection));

        return intersection;
    }

    if (roundabout_handler.canProcess(node_prior_to_intersection, entering_via_edge, intersection))
    {
        intersection = roundabout_handler(
            node_prior_to_intersection, entering_via_edge, std::move(intersection));
    }
    else
    {
        // set initial defaults for normal turns and modifier based on angle
        intersection =
            setTurnTypes(node_prior_to_intersection, entering_via_edge, std::move(intersection));
        if (motorway_handler.canProcess(
                node_prior_to_intersection, entering_via_edge, intersection))
        {
            intersection = motorway_handler(
                node_prior_to_intersection, entering_via_edge, std::move(intersection));
        }
        else
        {
            BOOST_ASSERT(turn_handler.canProcess(
                node_prior_to_intersection, entering_via_edge, intersection));
            intersection = turn_handler(
                node_prior_to_intersection, entering_via_edge, std::move(intersection));
        }
    }
    // Handle sliproads
    if (sliproad_handler.canProcess(node_prior_to_intersection, entering_via_edge, intersection))
        intersection = sliproad_handler(
            node_prior_to_intersection, entering_via_edge, std::move(intersection));

    // Turn On Ramps Into Off Ramps, if we come from a motorway-like road
    if (node_based_graph.GetEdgeData(entering_via_edge).road_classification.IsMotorwayClass())
    {
        std::for_each(intersection.begin(), intersection.end(), [](ConnectedRoad &road) {
            if (road.instruction.type == TurnType::OnRamp)
                road.instruction.type = TurnType::OffRamp;
        });
    }
    return intersection;
}

TurnAnalysis::ShapeResult
TurnAnalysis::ComputeIntersectionShapes(const NodeID node_at_center_of_intersection) const
{
    ShapeResult intersection_shape;
    intersection_shape.intersection_shape =
        intersection_generator.ComputeIntersectionShape(node_at_center_of_intersection);

    intersection_shape.annotated_normalized_shape = intersection_normalizer(
        node_at_center_of_intersection, intersection_shape.intersection_shape);

    return intersection_shape;
}

// Sets basic turn types as fallback for otherwise unhandled turns
Intersection TurnAnalysis::setTurnTypes(const NodeID node_prior_to_intersection,
                                        const EdgeID,
                                        Intersection intersection) const
{
    for (auto &road : intersection)
    {
        if (!road.entry_allowed)
            continue;

        const EdgeID onto_edge = road.eid;
        const NodeID to_nid = node_based_graph.GetTarget(onto_edge);

        road.instruction = {TurnType::Turn,
                            (node_prior_to_intersection == to_nid) ? DirectionModifier::UTurn
                                                                   : getTurnDirection(road.angle)};
    }
    return intersection;
}

const IntersectionGenerator &TurnAnalysis::GetIntersectionGenerator() const
{
    return intersection_generator;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
