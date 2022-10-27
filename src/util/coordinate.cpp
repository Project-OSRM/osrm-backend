#include "util/coordinate_calculation.hpp"

#ifndef NDEBUG
#include "util/log.hpp"
#endif
#include "osrm/coordinate.hpp"

#ifndef NDEBUG
#include <bitset>
#endif
#include <iomanip>
#include <iostream>
#include <limits>

namespace osrm
{
namespace util
{

bool Coordinate::IsValid() const
{
    return !(lat > FixedLatitude{static_cast<std::int32_t>(90 * COORDINATE_PRECISION)} ||
             lat < FixedLatitude{static_cast<std::int32_t>(-90 * COORDINATE_PRECISION)} ||
             lon > FixedLongitude{static_cast<std::int32_t>(180 * COORDINATE_PRECISION)} ||
             lon < FixedLongitude{static_cast<std::int32_t>(-180 * COORDINATE_PRECISION)});
}

bool FloatCoordinate::IsValid() const
{
    return !(lat > FloatLatitude{90} || lat < FloatLatitude{-90} || lon > FloatLongitude{180} ||
             lon < FloatLongitude{-180});
}

bool operator==(const Coordinate lhs, const Coordinate rhs)
{
    return lhs.lat == rhs.lat && lhs.lon == rhs.lon;
}
bool operator==(const FloatCoordinate lhs, const FloatCoordinate rhs)
{
    return lhs.lat == rhs.lat && lhs.lon == rhs.lon;
}

bool operator!=(const Coordinate lhs, const Coordinate rhs) { return !(lhs == rhs); }
bool operator!=(const FloatCoordinate lhs, const FloatCoordinate rhs) { return !(lhs == rhs); }
} // namespace util
} // namespace osrm
