#include "engine/polyline_compressor.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <cmath>
#include <cstddef>
#include <cstdlib>

namespace osrm
{
namespace engine
{
namespace /*detail*/ // anonymous to keep TU local
{

std::string encode(int number_to_encode)
{
    std::string output;
    while (number_to_encode >= 0x20)
    {
        const int next_value = (0x20 | (number_to_encode & 0x1f)) + 63;
        output += static_cast<char>(next_value);
        number_to_encode >>= 5;
    }

    number_to_encode += 63;
    output += static_cast<char>(number_to_encode);
    return output;
}

std::string encode(std::vector<int> &numbers)
{
    std::string output;
    for (auto &number : numbers)
    {
        bool isNegative = number < 0;

        if (isNegative)
        {
            const unsigned binary = std::llabs(number);
            const unsigned twos = (~binary) + 1u;
            number = twos;
        }

        number <<= 1u;

        if (isNegative)
        {
            number = ~number;
        }
    }
    for (const int number : numbers)
    {
        output += encode(number);
    }
    return output;
}
} // anonymous ns

std::string encodePolyline(CoordVectorForwardIter begin, CoordVectorForwardIter end)
{
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
        begin, end, [&delta_numbers, &current_lat, &current_lon](const util::Coordinate loc) {
            const int lat_diff =
                std::round(static_cast<int>(loc.lat) * detail::COORDINATE_TO_POLYLINE) -
                current_lat;
            const int lon_diff =
                std::round(static_cast<int>(loc.lon) * detail::COORDINATE_TO_POLYLINE) -
                current_lon;
            delta_numbers.emplace_back(lat_diff);
            delta_numbers.emplace_back(lon_diff);
            current_lat += lat_diff;
            current_lon += lon_diff;
        });
    return encode(delta_numbers);
}

std::vector<util::Coordinate> decodePolyline(const std::string &geometry_string)
{
    std::vector<util::Coordinate> new_coordinates;
    int index = 0, len = geometry_string.size();
    int lat = 0, lng = 0;

    while (index < len)
    {
        int b, shift = 0, result = 0;
        do
        {
            b = geometry_string.at(index++) - 63;
            result |= (b & 0x1f) << shift;
            shift += 5;
        } while (b >= 0x20);
        int dlat = ((result & 1) != 0 ? ~(result >> 1) : (result >> 1));
        lat += dlat;

        shift = 0;
        result = 0;
        do
        {
            b = geometry_string.at(index++) - 63;
            result |= (b & 0x1f) << shift;
            shift += 5;
        } while (b >= 0x20);
        int dlng = ((result & 1) != 0 ? ~(result >> 1) : (result >> 1));
        lng += dlng;

        util::Coordinate p;
        p.lat =
            util::FixedLatitude{static_cast<std::int32_t>(lat * detail::POLYLINE_TO_COORDINATE)};
        p.lon =
            util::FixedLongitude{static_cast<std::int32_t>(lng * detail::POLYLINE_TO_COORDINATE)};
        new_coordinates.push_back(p);
    }

    return new_coordinates;
}
}
}
