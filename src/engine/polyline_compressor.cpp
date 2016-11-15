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
namespace detail // anonymous to keep TU local
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
        if (number < 0)
        {
            const unsigned binary = std::llabs(number);
            const unsigned twos = (~binary) + 1u;
            const unsigned shl = twos << 1u;
            number = static_cast<int>(~shl);
        }
        else
        {
            number <<= 1u;
        }
    }
    for (const int number : numbers)
    {
        output += encode(number);
    }
    return output;
}
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
        } while (b >= 0x20 && index < len);
        int dlat = ((result & 1) != 0 ? ~(result >> 1) : (result >> 1));
        lat += dlat;

        shift = 0;
        result = 0;
        do
        {
            b = geometry_string.at(index++) - 63;
            result |= (b & 0x1f) << shift;
            shift += 5;
        } while (b >= 0x20 && index < len);
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
