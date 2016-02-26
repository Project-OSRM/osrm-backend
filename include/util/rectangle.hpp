#ifndef RECTANGLE_HPP
#define RECTANGLE_HPP

#include "util/coordinate_calculation.hpp"

#include <boost/assert.hpp>

#include "osrm/coordinate.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace osrm
{
namespace util
{

// TODO: Make template type, add tests
struct RectangleInt2D
{
    RectangleInt2D()
        : min_lon(std::numeric_limits<int32_t>::max()),
          max_lon(std::numeric_limits<int32_t>::min()),
          min_lat(std::numeric_limits<int32_t>::max()), max_lat(std::numeric_limits<int32_t>::min())
    {
    }

    RectangleInt2D(int32_t min_lon_, int32_t max_lon_, int32_t min_lat_, int32_t max_lat_)
        : min_lon(min_lon_), max_lon(max_lon_), min_lat(min_lat_), max_lat(max_lat_)
    {
    }

    int32_t min_lon, max_lon;
    int32_t min_lat, max_lat;

    void MergeBoundingBoxes(const RectangleInt2D &other)
    {
        min_lon = std::min(min_lon, other.min_lon);
        max_lon = std::max(max_lon, other.max_lon);
        min_lat = std::min(min_lat, other.min_lat);
        max_lat = std::max(max_lat, other.max_lat);
        BOOST_ASSERT(min_lat != std::numeric_limits<int32_t>::min());
        BOOST_ASSERT(min_lon != std::numeric_limits<int32_t>::min());
        BOOST_ASSERT(max_lat != std::numeric_limits<int32_t>::min());
        BOOST_ASSERT(max_lon != std::numeric_limits<int32_t>::min());
    }

    FixedPointCoordinate Centroid() const
    {
        FixedPointCoordinate centroid;
        // The coordinates of the midpoints are given by:
        // x = (x1 + x2) /2 and y = (y1 + y2) /2.
        centroid.lon = (min_lon + max_lon) / 2;
        centroid.lat = (min_lat + max_lat) / 2;
        return centroid;
    }

    bool Intersects(const RectangleInt2D &other) const
    {
        // Standard box intersection test - check if boxes *don't* overlap,
        // and return the negative of that
        return !(max_lon < other.min_lon || min_lon > other.max_lon || max_lat < other.min_lat ||
                 min_lat > other.max_lat);
    }

    double GetMinDist(const FixedPointCoordinate location) const
    {
        const bool is_contained = Contains(location);
        if (is_contained)
        {
            return 0.0f;
        }

        enum Direction
        {
            INVALID = 0,
            NORTH = 1,
            SOUTH = 2,
            EAST = 4,
            NORTH_EAST = 5,
            SOUTH_EAST = 6,
            WEST = 8,
            NORTH_WEST = 9,
            SOUTH_WEST = 10
        };

        Direction d = INVALID;
        if (location.lat > max_lat)
            d = (Direction)(d | NORTH);
        else if (location.lat < min_lat)
            d = (Direction)(d | SOUTH);
        if (location.lon > max_lon)
            d = (Direction)(d | EAST);
        else if (location.lon < min_lon)
            d = (Direction)(d | WEST);

        BOOST_ASSERT(d != INVALID);

        double min_dist = std::numeric_limits<double>::max();
        switch (d)
        {
        case NORTH:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(max_lat, location.lon));
            break;
        case SOUTH:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(min_lat, location.lon));
            break;
        case WEST:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(location.lat, min_lon));
            break;
        case EAST:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(location.lat, max_lon));
            break;
        case NORTH_EAST:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(max_lat, max_lon));
            break;
        case NORTH_WEST:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(max_lat, min_lon));
            break;
        case SOUTH_EAST:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(min_lat, max_lon));
            break;
        case SOUTH_WEST:
            min_dist = coordinate_calculation::greatCircleDistance(
                location, FixedPointCoordinate(min_lat, min_lon));
            break;
        default:
            break;
        }

        BOOST_ASSERT(min_dist < std::numeric_limits<double>::max());

        return min_dist;
    }

    double GetMinMaxDist(const FixedPointCoordinate location) const
    {
        double min_max_dist = std::numeric_limits<double>::max();
        // Get minmax distance to each of the four sides
        const FixedPointCoordinate upper_left(max_lat, min_lon);
        const FixedPointCoordinate upper_right(max_lat, max_lon);
        const FixedPointCoordinate lower_right(min_lat, max_lon);
        const FixedPointCoordinate lower_left(min_lat, min_lon);

        min_max_dist =
            std::min(min_max_dist,
                     std::max(coordinate_calculation::greatCircleDistance(location, upper_left),
                              coordinate_calculation::greatCircleDistance(location, upper_right)));

        min_max_dist =
            std::min(min_max_dist,
                     std::max(coordinate_calculation::greatCircleDistance(location, upper_right),
                              coordinate_calculation::greatCircleDistance(location, lower_right)));

        min_max_dist =
            std::min(min_max_dist,
                     std::max(coordinate_calculation::greatCircleDistance(location, lower_right),
                              coordinate_calculation::greatCircleDistance(location, lower_left)));

        min_max_dist =
            std::min(min_max_dist,
                     std::max(coordinate_calculation::greatCircleDistance(location, lower_left),
                              coordinate_calculation::greatCircleDistance(location, upper_left)));
        return min_max_dist;
    }

    bool Contains(const FixedPointCoordinate location) const
    {
        const bool lats_contained = (location.lat >= min_lat) && (location.lat <= max_lat);
        const bool lons_contained = (location.lon >= min_lon) && (location.lon <= max_lon);
        return lats_contained && lons_contained;
    }
};
}
}

#endif
