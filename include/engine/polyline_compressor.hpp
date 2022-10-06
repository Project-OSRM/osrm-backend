#ifndef POLYLINECOMPRESSOR_H_
#define POLYLINECOMPRESSOR_H_

#include "util/coordinate.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace detail
{
std::string encode(std::vector<int> &numbers);
std::int32_t decode_polyline_integer(std::string::const_iterator &first,
                                     std::string::const_iterator last);
} // namespace detail
using CoordVectorForwardIter = std::vector<util::Coordinate>::const_iterator;
// Encodes geometry into polyline format.
// See: https://developers.google.com/maps/documentation/utilities/polylinealgorithm

template <unsigned POLYLINE_PRECISION = 100000>
std::string encodePolyline(CoordVectorForwardIter begin, CoordVectorForwardIter end)
{
    double coordinate_to_polyline = POLYLINE_PRECISION / COORDINATE_PRECISION;
    auto size = std::distance(begin, end);
    if (size == 0)
    {
        return {};
    }

    std::vector<int> delta_numbers;
    BOOST_ASSERT(size > 0);
    delta_numbers.reserve((size - 1) * 2);
    int current_lat = 0;
    int current_lon = 0;
    std::for_each(
        begin,
        end,
        [&delta_numbers, &current_lat, &current_lon, coordinate_to_polyline](
            const util::Coordinate loc) {
            const int lat_diff =
                std::round(static_cast<int>(loc.lat) * coordinate_to_polyline) - current_lat;
            const int lon_diff =
                std::round(static_cast<int>(loc.lon) * coordinate_to_polyline) - current_lon;
            delta_numbers.emplace_back(lat_diff);
            delta_numbers.emplace_back(lon_diff);
            current_lat += lat_diff;
            current_lon += lon_diff;
        });
    return detail::encode(delta_numbers);
}

// Decodes geometry from polyline format
// See: https://developers.google.com/maps/documentation/utilities/polylinealgorithm

template <unsigned POLYLINE_PRECISION = 100000>
std::vector<util::Coordinate> decodePolyline(const std::string &polyline)
{
    double polyline_to_coordinate = COORDINATE_PRECISION / POLYLINE_PRECISION;
    std::vector<util::Coordinate> coordinates;
    std::int32_t latitude = 0, longitude = 0;

    std::string::const_iterator first = polyline.begin();
    const std::string::const_iterator last = polyline.end();
    while (first != last)
    {
        const auto dlat = detail::decode_polyline_integer(first, last);
        const auto dlon = detail::decode_polyline_integer(first, last);

        latitude += dlat;
        longitude += dlon;

        coordinates.emplace_back(util::Coordinate{
            util::FixedLongitude{static_cast<std::int32_t>(longitude * polyline_to_coordinate)},
            util::FixedLatitude{static_cast<std::int32_t>(latitude * polyline_to_coordinate)}});
    }
    return coordinates;
}
} // namespace engine
} // namespace osrm

#endif /* POLYLINECOMPRESSOR_H_ */
