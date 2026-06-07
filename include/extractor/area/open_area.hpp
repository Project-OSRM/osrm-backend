#ifndef OSRM_EXTRACTOR_AREA_OPEN_AREA_HPP
#define OSRM_EXTRACTOR_AREA_OPEN_AREA_HPP

#include "extractor/packed_osm_ids.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/rectangle.hpp"
#include "util/typedefs.hpp"

#include <osmium/osm/types.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace osrm::extractor::area
{

struct OpenAreaRing
{
    std::uint32_t offset{0};
    std::uint32_t size{0};
};

struct OpenAreaGeometry
{
    util::RectangleInt2D bbox;
    std::uint32_t outer_offset{0};
    std::uint32_t outer_size{0};
    std::uint32_t inner_offset{0};
    std::uint32_t inner_count{0};
    std::uint32_t visibility_offset{0};
    std::uint32_t visibility_count{0};
};

struct OpenAreaBoundsLeaf
{
    NodeID u{SPECIAL_NODEID};
    NodeID v{SPECIAL_NODEID};
    std::uint32_t area_id{0};
};

struct OpenAreaCollection
{
    std::vector<OpenAreaGeometry> areas;
    std::vector<util::Coordinate> ring_vertices;
    std::vector<OpenAreaRing> inner_rings;
    PackedOSMIDs visibility_nodes;
    std::vector<util::Coordinate> bbox_coordinates;
    std::vector<OpenAreaBoundsLeaf> bounds_leaves;
};

inline bool pointOnSegment(const util::Coordinate &point,
                           const util::Coordinate &first,
                           const util::Coordinate &second)
{
    const auto cross = util::coordinate_calculation::signedArea(first, second, point);
    if (cross != 0.0)
    {
        return false;
    }

    const auto min_lon = std::min(first.lon, second.lon);
    const auto max_lon = std::max(first.lon, second.lon);
    const auto min_lat = std::min(first.lat, second.lat);
    const auto max_lat = std::max(first.lat, second.lat);
    return point.lon >= min_lon && point.lon <= max_lon && point.lat >= min_lat &&
           point.lat <= max_lat;
}

inline bool pointInRingInclusive(const util::Coordinate &point,
                                 std::span<const util::Coordinate> ring)
{
    const auto ring_size = ring.size();
    if (ring_size < 3)
    {
        return false;
    }

    const auto *prev = ring.data() + ring.size() - 1;
    const auto point_lon = from_alias<double>(point.lon);
    const auto point_lat = from_alias<double>(point.lat);
    bool inside = false;
    for (const auto *current = ring.data(); current != ring.data() + ring.size(); ++current)
    {
        if (pointOnSegment(point, *prev, *current))
        {
            return true;
        }

        const auto current_lon = from_alias<double>(current->lon);
        const auto current_lat = from_alias<double>(current->lat);
        const auto prev_lon = from_alias<double>(prev->lon);
        const auto prev_lat = from_alias<double>(prev->lat);
        const bool intersects = ((current_lat > point_lat) != (prev_lat > point_lat)) &&
                                (point_lon < (prev_lon - current_lon) *
                                                     (point_lat - current_lat) /
                                                     (prev_lat - current_lat) +
                                                 current_lon);
        if (intersects)
        {
            inside = !inside;
        }

        prev = current;
    }

    return inside;
}

inline bool pointInPolygonInclusive(const util::Coordinate &point,
                                    std::span<const util::Coordinate> outer,
                                    std::span<const OpenAreaRing> inner_rings,
                                    std::span<const util::Coordinate> inner_vertices)
{
    if (!pointInRingInclusive(point, outer))
    {
        return false;
    }

    for (const auto &ring : inner_rings)
    {
        const auto begin = inner_vertices.data() + ring.offset;
        const auto end = begin + ring.size;
        if (pointInRingInclusive(point, std::span<const util::Coordinate>{begin, end}))
        {
            return false;
        }
    }

    return true;
}

} // namespace osrm::extractor::area

#endif // OSRM_EXTRACTOR_AREA_OPEN_AREA_HPP
