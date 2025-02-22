#ifndef POLYLINECOMPRESSOR_H_
#define POLYLINECOMPRESSOR_H_

#include "util/coordinate.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <string>
#include <vector>

namespace osrm::engine
{
namespace detail
{
void encode(int number_to_encode, std::string &output);
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

    std::string output;
    // just a guess that we will need ~4 bytes per coordinate to avoid reallocations
    output.reserve(size * 4);

    int current_lat = 0;
    int current_lon = 0;
    for (auto it = begin; it != end; ++it)
    {
        const int lat_diff =
            std::round(static_cast<int>(it->lat) * coordinate_to_polyline) - current_lat;
        const int lon_diff =
            std::round(static_cast<int>(it->lon) * coordinate_to_polyline) - current_lon;
        detail::encode(lat_diff, output);
        detail::encode(lon_diff, output);
        current_lat += lat_diff;
        current_lon += lon_diff;
    }
    return output;
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
} // namespace osrm::engine

#endif /* POLYLINECOMPRESSOR_H_ */
