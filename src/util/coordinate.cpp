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

Coordinate::Coordinate()
    : lon(std::numeric_limits<int>::min()), lat(std::numeric_limits<int>::min())
{
}

Coordinate::Coordinate(const FloatLongitude lon_, const FloatLatitude lat_)
    : Coordinate(toFixed(lon_), toFixed(lat_))
{
}

Coordinate::Coordinate(const FixedLongitude lon_, const FixedLatitude lat_) : lon(lon_), lat(lat_)
{
#ifndef NDEBUG
    if (0 != (std::abs(static_cast<int>(lon)) >> 30))
    {
        std::bitset<32> x_coordinate_vector(static_cast<int>(lon));
        SimpleLogger().Write(logDEBUG) << "broken lon: " << lon
                                       << ", bits: " << x_coordinate_vector;
    }
    if (0 != (std::abs(static_cast<int>(lat)) >> 30))
    {
        std::bitset<32> y_coordinate_vector(static_cast<int>(lat));
        SimpleLogger().Write(logDEBUG) << "broken lat: " << lat
                                       << ", bits: " << y_coordinate_vector;
    }
#endif
}

bool Coordinate::IsValid() const
{
    return !(lat > FixedLatitude(90 * COORDINATE_PRECISION) ||
             lat < FixedLatitude(-90 * COORDINATE_PRECISION) ||
             lon > FixedLongitude(180 * COORDINATE_PRECISION) ||
             lon < FixedLongitude(-180 * COORDINATE_PRECISION));
}

bool operator==(const Coordinate lhs, const Coordinate rhs)
{
    return lhs.lat == rhs.lat && lhs.lon == rhs.lon;
}

bool operator!=(const Coordinate lhs, const Coordinate rhs) { return !(lhs == rhs); }

std::ostream &operator<<(std::ostream &out, const Coordinate coordinate)
{
    out << "(lon:" << toFloating(coordinate.lon)
        << ", lat:" << toFloating(coordinate.lat)
        << ")";
    return out;
}
}
}
