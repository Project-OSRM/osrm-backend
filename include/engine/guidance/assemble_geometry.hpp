#ifndef ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP
#define ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/guidance/toolkit.hpp"
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
                                    const PhantomNode &target_node)
{
    LegGeometry geometry;

    // segment 0 first and last
    geometry.segment_offsets.push_back(0);
    geometry.locations.push_back(source_node.location);

    // Need to get the node ID preceding the source phantom node
    // TODO: check if this was traversed in reverse?
    std::vector<NodeID> reverse_geometry;
    facade.GetUncompressedGeometry(source_node.reverse_packed_geometry_id, reverse_geometry);
    geometry.osm_node_ids.push_back(facade.GetOSMNodeIDOfNode(
        reverse_geometry[reverse_geometry.size() - source_node.fwd_segment_position - 1]));

    std::vector<uint8_t> forward_datasource_vector;
    facade.GetUncompressedDatasources(source_node.forward_packed_geometry_id, forward_datasource_vector);


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
            LegGeometry::Annotation{current_distance, path_point.duration_until_turn / 10., path_point.datasource_id});
        geometry.locations.push_back(std::move(coordinate));
        geometry.osm_node_ids.push_back(facade.GetOSMNodeIDOfNode(path_point.turn_via_node));
    }
    current_distance =
        util::coordinate_calculation::haversineDistance(prev_coordinate, target_node.location);
    cumulative_distance += current_distance;
    // segment leading to the target node
    geometry.segment_distances.push_back(cumulative_distance);

    std::vector<DatasourceID> forward_datasources;
    facade.GetUncompressedDatasources(target_node.forward_packed_geometry_id, forward_datasources);

    geometry.annotations.emplace_back(
        LegGeometry::Annotation{current_distance, target_node.forward_weight / 10., forward_datasources[target_node.fwd_segment_position]});
    geometry.segment_offsets.push_back(geometry.locations.size());
    geometry.locations.push_back(target_node.location);

    // Need to get the node ID following the destination phantom node
    // TODO: check if this was traversed in reverse??
    std::vector<NodeID> forward_geometry;
    facade.GetUncompressedGeometry(target_node.forward_packed_geometry_id, forward_geometry);
    geometry.osm_node_ids.push_back(
        facade.GetOSMNodeIDOfNode(forward_geometry[target_node.fwd_segment_position]));


    BOOST_ASSERT(geometry.segment_distances.size() == geometry.segment_offsets.size() - 1);
    BOOST_ASSERT(geometry.locations.size() > geometry.segment_distances.size());
    BOOST_ASSERT(geometry.annotations.size() == geometry.locations.size() - 1);

    return geometry;
}
}
}
}

#endif
