#include "engine/polyline_compressor.hpp"

#include <cstddef>
#include <cstdlib>
#include <cmath>

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

std::string polylineEncode(const std::vector<SegmentInformation> &polyline)
{
    if (polyline.empty())
    {
        return {};
    }

    std::vector<int> delta_numbers;
    delta_numbers.reserve((polyline.size() - 1) * 2);
    util::FixedPointCoordinate previous_coordinate = {0, 0};
    for (const auto &segment : polyline)
    {
        if (segment.necessary)
        {
            const int lat_diff = segment.location.lat - previous_coordinate.lat;
            const int lon_diff = segment.location.lon - previous_coordinate.lon;
            delta_numbers.emplace_back(lat_diff);
            delta_numbers.emplace_back(lon_diff);
            previous_coordinate = segment.location;
        }
    }
    return encode(delta_numbers);
}

std::vector<util::FixedPointCoordinate> polylineDecode(const std::string &geometry_string)
{
    std::vector<util::FixedPointCoordinate> new_coordinates;
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

        util::FixedPointCoordinate p;
        p.lat = COORDINATE_PRECISION * (((double)lat / 1E6));
        p.lon = COORDINATE_PRECISION * (((double)lng / 1E6));
        new_coordinates.push_back(p);
    }

    return new_coordinates;
}
}
}
