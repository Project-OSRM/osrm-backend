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

namespace osrm
{
namespace util
{

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

bool FixedPointCoordinate::IsValid() const
{
    return !(lat > 90 * COORDINATE_PRECISION || lat < -90 * COORDINATE_PRECISION ||
             lon > 180 * COORDINATE_PRECISION || lon < -180 * COORDINATE_PRECISION);
}

bool FixedPointCoordinate::operator==(const FixedPointCoordinate &other) const
{
    return lat == other.lat && lon == other.lon;
}

std::ostream &operator<<(std::ostream &out, const FixedPointCoordinate &coordinate)
{
    out << "(" << static_cast<double>(coordinate.lat / COORDINATE_PRECISION) << "," << static_cast<double>(coordinate.lon / COORDINATE_PRECISION) << ")";
    return out;
}
}
}
