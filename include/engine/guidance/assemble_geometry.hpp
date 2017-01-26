#ifndef ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP
#define ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

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
    const std::vector<NodeID> source_geometry =
        facade.GetUncompressedForwardGeometry(source_node.packed_geometry_id);
    geometry.osm_node_ids.push_back(
        facade.GetOSMNodeIDOfNode(source_geometry[source_segment_start_coordinate]));

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
        if (path_point.turn_instruction.type != extractor::guidance::TurnType::NoTurn)
        {
            geometry.segment_distances.push_back(cumulative_distance);
            geometry.segment_offsets.push_back(geometry.locations.size());
            cumulative_distance = 0.;
        }

        prev_coordinate = coordinate;
        geometry.annotations.emplace_back(
            LegGeometry::Annotation{current_distance,
                                    path_point.duration_until_turn / 10.,
                                    path_point.weight_until_turn / facade.GetWeightMultiplier(),
                                    path_point.datasource_id});
        geometry.locations.push_back(std::move(coordinate));
        geometry.osm_node_ids.push_back(facade.GetOSMNodeIDOfNode(path_point.turn_via_node));
    }
    current_distance =
        util::coordinate_calculation::haversineDistance(prev_coordinate, target_node.location);
    cumulative_distance += current_distance;
    // segment leading to the target node
    geometry.segment_distances.push_back(cumulative_distance);

    const std::vector<DatasourceID> forward_datasources =
        facade.GetUncompressedForwardDatasources(target_node.packed_geometry_id);

    // FIXME if source and target phantoms are on the same segment then duration and weight
    // will be from one projected point till end of segment
    // testbot/weight.feature:Start and target on the same and adjacent edge
    geometry.annotations.emplace_back(LegGeometry::Annotation{
        current_distance,
        (reversed_target ? target_node.reverse_duration : target_node.forward_duration) / 10.,
        (reversed_target ? target_node.reverse_weight : target_node.forward_weight) /
            facade.GetWeightMultiplier(),
        forward_datasources[target_node.fwd_segment_position]});

    geometry.segment_offsets.push_back(geometry.locations.size());
    geometry.locations.push_back(target_node.location);

    //                           u       *      v
    //                           0 -- 1 -- 2 -- 3
    // fwd_segment_position:  1
    // target node fwd:       2  0 -> 1 -> 2
    // target node rev:       1       1 <- 2 <- 3
    const auto target_segment_end_coordinate =
        target_node.fwd_segment_position + (reversed_target ? 0 : 1);
    const std::vector<NodeID> target_geometry =
        facade.GetUncompressedForwardGeometry(target_node.packed_geometry_id);
    geometry.osm_node_ids.push_back(
        facade.GetOSMNodeIDOfNode(target_geometry[target_segment_end_coordinate]));

    BOOST_ASSERT(geometry.segment_distances.size() == geometry.segment_offsets.size() - 1);
    BOOST_ASSERT(geometry.locations.size() > geometry.segment_distances.size());
    BOOST_ASSERT(geometry.annotations.size() == geometry.locations.size() - 1);

    return geometry;
}
}
}
}

#endif
