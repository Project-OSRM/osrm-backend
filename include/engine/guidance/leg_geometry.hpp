#ifndef ENGINE_GUIDANCE_LEG_GEOMETRY_HPP
#define ENGINE_GUIDANCE_LEG_GEOMETRY_HPP

#include "util/coordinate.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>

#include <cstdlib>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

// locations 0---1---2-...-n-1---n
// turns     s       x      y    t
// segment   |   0   |  1   | 2  | sentinel
// offsets       0      2    n-1     n
struct LegGeometry
{
    std::vector<util::Coordinate> locations;
    // segment_offset[i] .. segment_offset[i+1] (inclusive)
    // contains the geometry of segment i
    std::vector<std::size_t> segment_offsets;
    // length of the segment in meters
    std::vector<double> segment_distances;
    // original OSM node IDs for each coordinate
    std::vector<OSMNodeID> osm_node_ids;

    // Per-coordinate metadata
    struct Annotation
    {
        double distance;
        double duration;
        DatasourceID datasource;
    };
    std::vector<Annotation> annotations;

    std::size_t FrontIndex(std::size_t segment_index) const
    {
        return segment_offsets[segment_index];
    }

    std::size_t BackIndex(std::size_t segment_index) const
    {
        return segment_offsets[segment_index + 1];
    }

    std::size_t GetNumberOfSegments() const
    {
        BOOST_ASSERT(segment_offsets.size() > 0);
        return segment_offsets.size() - 1;
    }
};
}
}
}

#endif
