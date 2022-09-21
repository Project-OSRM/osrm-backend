#ifndef ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP
#define ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP

#include "extractor/travel_mode.hpp"
#include "guidance/turn_instruction.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{
// Extracts the geometry for each segment and calculates the traveled distance
// Combines the geometry form the phantom node with the PathData
// to the full route geometry.
//
// turn    0   1   2   3   4
//         s...x...y...z...t
//         |---|segment 0
//             |---| segment 1
//                 |---| segment 2
//                     |---| segment 3
inline LegGeometry assembleGeometry(const datafacade::BaseDataFacade &facade,
                                    const std::vector<PathData> &leg_data,
                                    const PhantomNode &source_node,
                                    const PhantomNode &target_node,
                                    const bool reversed_source,
                                    const bool reversed_target)
{
    LegGeometry geometry;

    // segment 0 first and last
    geometry.segment_offsets.push_back(0);
    geometry.locations.push_back(source_node.location);

    //                          u       *      v
    //                          0 -- 1 -- 2 -- 3
    // fwd_segment_position:  1
    // source node fwd:       1      1 -> 2 -> 3
    // source node rev:       2 0 <- 1 <- 2
    const auto source_segment_start_coordinate =
        source_node.fwd_segment_position + (reversed_source ? 1 : 0);
    const auto source_node_id =
        reversed_source ? source_node.reverse_segment_id.id : source_node.forward_segment_id.id;
    const auto source_geometry_id = facade.GetGeometryIndex(source_node_id).id;
    const auto source_geometry = facade.GetUncompressedForwardGeometry(source_geometry_id);

    geometry.osm_node_ids.push_back(
        facade.GetOSMNodeIDOfNode(source_geometry(source_segment_start_coordinate)));

    auto cumulative_distance = 0.;
    auto current_distance = 0.;
    auto prev_coordinate = geometry.locations.front();
    for (const auto &path_point : leg_data)
    {
        auto coordinate = facade.GetCoordinateOfNode(path_point.turn_via_node);
        current_distance =
            util::coordinate_calculation::haversineDistance(prev_coordinate, coordinate);
        cumulative_distance += current_distance;

        // all changes to this check have to be matched with assemble_steps
        if (path_point.turn_instruction.type != osrm::guidance::TurnType::NoTurn)
        {
            geometry.segment_distances.push_back(cumulative_distance);
            geometry.segment_offsets.push_back(geometry.locations.size());
            cumulative_distance = 0.;
        }

        prev_coordinate = coordinate;

        const auto osm_node_id = facade.GetOSMNodeIDOfNode(path_point.turn_via_node);

        if (osm_node_id != geometry.osm_node_ids.back() ||
            path_point.turn_instruction.type != osrm::guidance::TurnType::NoTurn)
        {
            geometry.annotations.emplace_back(LegGeometry::Annotation{
                current_distance,
                // NOTE: we want annotations to include only the duration/weight
                //       of the segment itself.  For segments immediately before
                //       a turn, the duration_until_turn/weight_until_turn values
                //       include the turn cost.  To counter this, we subtract
                //       the duration_of_turn/weight_of_turn value, which is 0 for
                //       non-preceeding-turn segments, but contains the turn value
                //       for segments before a turn.
                (path_point.duration_until_turn - path_point.duration_of_turn) / 10.,
                (path_point.weight_until_turn - path_point.weight_of_turn) /
                    facade.GetWeightMultiplier(),
                path_point.datasource_id});
            geometry.locations.push_back(std::move(coordinate));
            geometry.osm_node_ids.push_back(osm_node_id);
        }
    }
    current_distance =
        util::coordinate_calculation::haversineDistance(prev_coordinate, target_node.location);
    cumulative_distance += current_distance;
    // segment leading to the target node
    geometry.segment_distances.push_back(cumulative_distance);

    const auto target_node_id =
        reversed_target ? target_node.reverse_segment_id.id : target_node.forward_segment_id.id;
    const auto target_geometry_id = facade.GetGeometryIndex(target_node_id).id;
    const auto forward_datasources = facade.GetUncompressedForwardDatasources(target_geometry_id);

    // This happens when the source/target are on the same edge-based-node
    // There will be no entries in the unpacked path, thus no annotations.
    // We will need to calculate the lone annotation by looking at the position
    // of the source/target nodes, and calculating their differences.
    if (geometry.annotations.empty())
    {
        auto duration =
            std::abs(
                (reversed_target ? target_node.reverse_duration : target_node.forward_duration) -
                (reversed_source ? source_node.reverse_duration : source_node.forward_duration)) /
            10.;
        BOOST_ASSERT(duration >= 0);
        auto weight =
            std::abs((reversed_target ? target_node.reverse_weight : target_node.forward_weight) -
                     (reversed_source ? source_node.reverse_weight : source_node.forward_weight)) /
            facade.GetWeightMultiplier();
        BOOST_ASSERT(weight >= 0);

        geometry.annotations.emplace_back(
            LegGeometry::Annotation{current_distance,
                                    duration,
                                    weight,
                                    forward_datasources(target_node.fwd_segment_position)});
    }
    else
    {
        geometry.annotations.emplace_back(LegGeometry::Annotation{
            current_distance,
            (reversed_target ? target_node.reverse_duration : target_node.forward_duration) / 10.,
            (reversed_target ? target_node.reverse_weight : target_node.forward_weight) /
                facade.GetWeightMultiplier(),
            forward_datasources(target_node.fwd_segment_position)});
    }

    geometry.segment_offsets.push_back(geometry.locations.size());
    geometry.locations.push_back(target_node.location);

    //                           u       *      v
    //                           0 -- 1 -- 2 -- 3
    // fwd_segment_position:  1
    // target node fwd:       2  0 -> 1 -> 2
    // target node rev:       1       1 <- 2 <- 3
    const auto target_segment_end_coordinate =
        target_node.fwd_segment_position + (reversed_target ? 0 : 1);
    const auto target_geometry = facade.GetUncompressedForwardGeometry(target_geometry_id);
    geometry.osm_node_ids.push_back(
        facade.GetOSMNodeIDOfNode(target_geometry(target_segment_end_coordinate)));

    BOOST_ASSERT(geometry.segment_distances.size() == geometry.segment_offsets.size() - 1);
    BOOST_ASSERT(geometry.locations.size() > geometry.segment_distances.size());
    BOOST_ASSERT(geometry.annotations.size() == geometry.locations.size() - 1);

    return geometry;
}
} // namespace guidance
} // namespace engine
} // namespace osrm

#endif
