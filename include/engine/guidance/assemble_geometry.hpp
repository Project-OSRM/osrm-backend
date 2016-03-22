#ifndef ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP
#define ENGINE_GUIDANCE_ASSEMBLE_GEOMETRY_HPP

#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/toolkit.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/coordinate.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"

#include <vector>
#include <utility>

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
template <typename DataFacadeT>
LegGeometry assembleGeometry(const DataFacadeT &facade,
                             const std::vector<PathData> &leg_data,
                             const PhantomNode &source_node,
                             const PhantomNode &target_node)
{
    LegGeometry geometry;

    // segment 0 first and last
    geometry.segment_offsets.push_back(0);
    geometry.locations.push_back(source_node.location);

    auto current_distance = 0.;
    auto prev_coordinate = geometry.locations.front();
    for (const auto &path_point : leg_data)
    {
        auto coordinate = facade.GetCoordinateOfNode(path_point.turn_via_node);
        current_distance +=
            util::coordinate_calculation::haversineDistance(prev_coordinate, coordinate);

        //all changes to this check have to be matched with assemble_steps
        if (path_point.turn_instruction.type != extractor::guidance::TurnType::NoTurn)
        {
            geometry.segment_distances.push_back(current_distance);
            geometry.segment_offsets.push_back(geometry.locations.size());
            current_distance = 0.;
        }

        prev_coordinate = coordinate;
        geometry.locations.push_back(std::move(coordinate));
    }
    current_distance +=
        util::coordinate_calculation::haversineDistance(prev_coordinate, target_node.location);
    // segment leading to the target node
    geometry.segment_distances.push_back(current_distance);
    geometry.segment_offsets.push_back(geometry.locations.size());
    geometry.locations.push_back(target_node.location);

    BOOST_ASSERT(geometry.segment_distances.size() == geometry.segment_offsets.size() - 1);
    BOOST_ASSERT(geometry.locations.size() > geometry.segment_distances.size());

    return geometry;
}
}
}
}

#endif
