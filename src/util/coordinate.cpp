#include "util/coordinate_calculation.hpp"

#ifndef NDEBUG
#include "util/simple_logger.hpp"
#endif
#include "osrm/coordinate.hpp"

#ifndef NDEBUG
#include <bitset>
#endif
#include <iostream>
#include <limits>

FixedPointCoordinate::FixedPointCoordinate()
    : lat(std::numeric_limits<int>::min()), lon(std::numeric_limits<int>::min())
{
}

FixedPointCoordinate::FixedPointCoordinate(int lat, int lon) : lat(lat), lon(lon)
{
#ifndef NDEBUG
    if (0 != (std::abs(lat) >> 30))
    {
        std::bitset<32> y_coordinate_vector(lat);
        SimpleLogger().Write(logDEBUG) << "broken lat: " << lat
                                       << ", bits: " << y_coordinate_vector;
    }
    if (0 != (std::abs(lon) >> 30))
    {
        std::bitset<32> x_coordinate_vector(lon);
        SimpleLogger().Write(logDEBUG) << "broken lon: " << lon
                                       << ", bits: " << x_coordinate_vector;
    }
#endif
}

bool FixedPointCoordinate::is_valid() const
{
    if (lat > 90 * COORDINATE_PRECISION || lat < -90 * COORDINATE_PRECISION ||
        lon > 180 * COORDINATE_PRECISION || lon < -180 * COORDINATE_PRECISION)
    {
        return false;
    }
    return true;
}

bool FixedPointCoordinate::operator==(const FixedPointCoordinate &other) const
{
    return lat == other.lat && lon == other.lon;
}

void FixedPointCoordinate::output(std::ostream &out) const
{
    out << "(" << lat / COORDINATE_PRECISION << "," << lon / COORDINATE_PRECISION << ")";
}

double FixedPointCoordinate::bearing(const FixedPointCoordinate &other) const
{
    return coordinate_calculation::bearing(other, *this);
}
