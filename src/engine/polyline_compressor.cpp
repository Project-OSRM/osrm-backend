#include "engine/polyline_compressor.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <climits>
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
    // https://developers.google.com/maps/documentation/utilities/polylinealgorithm
    auto decode_polyline_integer = [](auto &first, auto last) {
        // varint coding parameters
        const std::uint32_t bits_in_chunk = 5;
        const std::uint32_t continuation_bit = 1 << bits_in_chunk;
        const std::uint32_t chunk_mask = (1 << bits_in_chunk) - 1;

        std::uint32_t result = 0;
        for (std::uint32_t value = continuation_bit, shift = 0;
             (value & continuation_bit) && (shift < CHAR_BIT * sizeof(result) - 1) && first != last;
             ++first, shift += bits_in_chunk)
        {
            value = *first - 63; // convert ASCII coding [?..~] to an integer [0..63]
            result |= (value & chunk_mask) << shift;
        }

        // change "zig-zag" sign coding to two's complement
        result = ((result & 1) == 1) ? ~(result >> 1) : (result >> 1);
        return static_cast<std::int32_t>(result);
    };

    auto polyline_to_coordinate = [](auto value) {
        return static_cast<std::int32_t>(value * detail::POLYLINE_TO_COORDINATE);
    };

    std::vector<util::Coordinate> coordinates;
    std::int32_t latitude = 0, longitude = 0;

    std::string::const_iterator first = geometry_string.begin();
    const std::string::const_iterator last = geometry_string.end();
    while (first != last)
    {
        const auto dlat = decode_polyline_integer(first, last);
        const auto dlon = decode_polyline_integer(first, last);

        latitude += dlat;
        longitude += dlon;

        coordinates.emplace_back(
            util::Coordinate{util::FixedLongitude{polyline_to_coordinate(longitude)},
                             util::FixedLatitude{polyline_to_coordinate(latitude)}});
    }

    return coordinates;
}
}
}
