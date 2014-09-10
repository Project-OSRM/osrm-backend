#ifndef SYMBOLIC_COORDINATE_H_
#define SYMBOLIC_COORDINATE_H_

#include "../Util/MercatorUtil.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

#include <limits>
#include <iostream>
#include <vector>

/**
 * In contrast to FixedPointCoordinate this coordinates are in
 * euclidian space.
 *
 * Usually the x and y coordinates are normed to [0, 1] of the bounding box
 * of the path they are computed in.
 * For PathSchematization this scaling is not really important (hence the name *symbolic* coordinate),
 * but it makes it easier to define size independent lengths.
 */
static const unsigned INVALID_IDX = std::numeric_limits<unsigned>::max();

struct SymbolicCoordinate
{
    typedef double ScalarT;

    SymbolicCoordinate()
    : x(0)
    , y(0)
    , original_idx(INVALID_IDX)
    , interval_id(INVALID_IDX)
    {
    }

    SymbolicCoordinate(ScalarT x,
                       ScalarT y,
                       unsigned original_idx=INVALID_IDX,
                       unsigned interval_id=INVALID_IDX)
    : x(x)
    , y(y)
    , original_idx(original_idx)
    , interval_id(interval_id)
    {
    }

    ScalarT x;
    ScalarT y;
    // link to original FixedPointCoordinate in the un-schematized path
    unsigned original_idx;
    // we use this to indicate street segments: Edge merging will not merge edge with different interval ids.
    unsigned interval_id;

    bool operator==(const SymbolicCoordinate& other) const
    {
        return (x == other.x && y == other.y);
    }
};

inline std::ostream& operator<<(std::ostream& s, const SymbolicCoordinate& sym)
{
    s << "(" << sym.x << "/" << sym.y << ")";
    return s;
}

inline FixedPointCoordinate transformSymbolicToFixed(const SymbolicCoordinate& s, double scaling, const FixedPointCoordinate& origin)
{
    return FixedPointCoordinate(y2lat(s.y * scaling + lat2y(origin.lat / COORDINATE_PRECISION)) * COORDINATE_PRECISION,
                                origin.lon + s.x * scaling * COORDINATE_PRECISION);
}

/**
 * Scale coords to bounding box and transform to mercator.
 *
 * The getter functions make it possible to input both FixedPointCoordinate
 * and SegmentInformation as CoordinateT.
 */
template<typename CoordinateT, typename CoordGetterT, typename IntervalGetterT>
void transformFixedPointToSymbolic(const std::vector<CoordinateT>& coords,
                                   std::vector<SymbolicCoordinate>& symbolicCoords,
                                   double& scaling,
                                   FixedPointCoordinate& origin,
                                   CoordGetterT coord_getter,
                                   IntervalGetterT interval_getter)
{
    // Compute bounding box
    int min_lat = std::numeric_limits<int>::max();
    int min_lon = std::numeric_limits<int>::max();
    int max_lat = 0;
    int max_lon = 0;

    for (const auto& c : coords)
    {
        FixedPointCoordinate f;
        if (!coord_getter(c, f))
        {
            continue;
        }

        min_lat = std::min(min_lat, f.lat);
        min_lon = std::min(min_lon, f.lon);
        max_lat = std::max(max_lat, f.lat);
        max_lon = std::max(max_lon, f.lon);
    }

    BOOST_ASSERT(coords.size() > 0);
    // find first valid coordinate
    for (const auto& c: coords)
    {
        if(coord_getter(c, origin))
        {
            break;
        }
    }

    double width = (max_lon - min_lon) / COORDINATE_PRECISION;
    double height = lat2y(max_lat / COORDINATE_PRECISION) - lat2y(min_lat / COORDINATE_PRECISION);
    double min_y = lat2y(origin.lat / COORDINATE_PRECISION);
    scaling = std::max(width, height);

    for (unsigned i = 0; i < coords.size(); i++)
    {
        FixedPointCoordinate f;
        if (!coord_getter(coords[i], f))
        {
            continue;
        }

        double y = ((lat2y(f.lat / COORDINATE_PRECISION) - min_y) / scaling);
        double x = ((f.lon - origin.lon) / COORDINATE_PRECISION / scaling);
        symbolicCoords.emplace_back(x, y, i, interval_getter(coords[i]));
    }
}

#endif
