#ifndef RECTANGLE_H
#define RECTANGLE_H

// TODO: Make template type
struct RectangleInt2D
{
    RectangleInt2D() : min_lon(INT_MAX), max_lon(INT_MIN), min_lat(INT_MAX), max_lat(INT_MIN) {}

    int32_t min_lon, max_lon;
    int32_t min_lat, max_lat;


    inline void MergeBoundingBoxes(const RectangleInt2D &other)
    {
        min_lon = std::min(min_lon, other.min_lon);
        max_lon = std::max(max_lon, other.max_lon);
        min_lat = std::min(min_lat, other.min_lat);
        max_lat = std::max(max_lat, other.max_lat);
        BOOST_ASSERT(min_lat != std::numeric_limits<int>::min());
        BOOST_ASSERT(min_lon != std::numeric_limits<int>::min());
        BOOST_ASSERT(max_lat != std::numeric_limits<int>::min());
        BOOST_ASSERT(max_lon != std::numeric_limits<int>::min());
    }

    inline FixedPointCoordinate Centroid() const
    {
        FixedPointCoordinate centroid;
        // The coordinates of the midpoints are given by:
        // x = (x1 + x2) /2 and y = (y1 + y2) /2.
        centroid.lon = (min_lon + max_lon) / 2;
        centroid.lat = (min_lat + max_lat) / 2;
        return centroid;
    }

    inline bool Intersects(const RectangleInt2D &other) const
    {
        FixedPointCoordinate upper_left(other.max_lat, other.min_lon);
        FixedPointCoordinate upper_right(other.max_lat, other.max_lon);
        FixedPointCoordinate lower_right(other.min_lat, other.max_lon);
        FixedPointCoordinate lower_left(other.min_lat, other.min_lon);

        return (Contains(upper_left) || Contains(upper_right) || Contains(lower_right) ||
                Contains(lower_left));
    }

    inline float GetMinDist(const FixedPointCoordinate &location) const
    {
        const bool is_contained = Contains(location);
        if (is_contained)
        {
            return 0.;
        }

        enum Direction
        {
            INVALID    = 0,
            NORTH      = 1,
            SOUTH      = 2,
            EAST       = 4,
            NORTH_EAST = 5,
            SOUTH_EAST = 6,
            WEST       = 8,
            NORTH_WEST = 9,
            SOUTH_WEST = 10
        };

        Direction d = INVALID;
        if (location.lat > max_lat)
            d = (Direction) (d | NORTH);
        else if (location.lat < min_lat)
            d = (Direction) (d | SOUTH);
        if (location.lon > max_lon)
            d = (Direction) (d | EAST);
        else if (location.lon < min_lon)
            d = (Direction) (d | WEST);

        BOOST_ASSERT(d != INVALID);

        float min_dist = std::numeric_limits<float>::max();
        switch (d)
        {
            case NORTH:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(max_lat, location.lon));
                break;
            case SOUTH:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(min_lat, location.lon));
                break;
            case WEST:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(location.lat, min_lon));
                break;
            case EAST:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(location.lat, max_lon));
                break;
            case NORTH_EAST:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(max_lat, max_lon));
                break;
            case NORTH_WEST:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(max_lat, min_lon));
                break;
            case SOUTH_EAST:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(min_lat, max_lon));
                break;
            case SOUTH_WEST:
                min_dist = FixedPointCoordinate::ApproximateEuclideanDistance(location, FixedPointCoordinate(min_lat, min_lon));
                break;
            default:
                break;
        }

        BOOST_ASSERT(min_dist != std::numeric_limits<float>::max());

        return min_dist;
    }

    inline float GetMinMaxDist(const FixedPointCoordinate &location) const
    {
        float min_max_dist = std::numeric_limits<float>::max();
        // Get minmax distance to each of the four sides
        const FixedPointCoordinate upper_left(max_lat, min_lon);
        const FixedPointCoordinate upper_right(max_lat, max_lon);
        const FixedPointCoordinate lower_right(min_lat, max_lon);
        const FixedPointCoordinate lower_left(min_lat, min_lon);

        min_max_dist = std::min(
            min_max_dist,
            std::max(
                FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_left),
                FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_right)));

        min_max_dist = std::min(
            min_max_dist,
            std::max(
                FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_right),
                FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_right)));

        min_max_dist = std::min(
            min_max_dist,
            std::max(FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_right),
                     FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_left)));

        min_max_dist = std::min(
            min_max_dist,
            std::max(FixedPointCoordinate::ApproximateEuclideanDistance(location, lower_left),
                     FixedPointCoordinate::ApproximateEuclideanDistance(location, upper_left)));
        return min_max_dist;
    }

    inline bool Contains(const FixedPointCoordinate &location) const
    {
        const bool lats_contained = (location.lat >= min_lat) && (location.lat <= max_lat);
        const bool lons_contained = (location.lon >= min_lon) && (location.lon <= max_lon);
        return lats_contained && lons_contained;
    }

    inline friend std::ostream &operator<<(std::ostream &out, const RectangleInt2D &rect)
    {
        out << rect.min_lat / COORDINATE_PRECISION << "," << rect.min_lon / COORDINATE_PRECISION
            << " " << rect.max_lat / COORDINATE_PRECISION << ","
            << rect.max_lon / COORDINATE_PRECISION;
        return out;
    }
};

#endif
