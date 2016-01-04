#include "engine/polyline_compressor.hpp"
#include "engine/segment_information.hpp"

#include "osrm/coordinate.hpp"

std::string PolylineCompressor::encode_vector(std::vector<int> &numbers) const
{
    std::string output;
    const auto end = numbers.size();
    for (std::size_t i = 0; i < end; ++i)
    {
        numbers[i] <<= 1;
        if (numbers[i] < 0)
        {
            numbers[i] = ~(numbers[i]);
        }
    }
    for (const int number : numbers)
    {
        output += encode_number(number);
    }
    return output;
}

std::string PolylineCompressor::encode_number(int number_to_encode) const
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

std::string
PolylineCompressor::get_encoded_string(const std::vector<SegmentInformation> &polyline) const
{
    if (polyline.empty())
    {
        return {};
    }

    std::vector<int> delta_numbers;
    delta_numbers.reserve((polyline.size() - 1) * 2);
    FixedPointCoordinate previous_coordinate = {0, 0};
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
    return encode_vector(delta_numbers);
}

std::vector<FixedPointCoordinate> PolylineCompressor::decode_string(const std::string &geometry_string) const
{
    std::vector<FixedPointCoordinate> new_coordinates;
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

        FixedPointCoordinate p;
        p.lat = COORDINATE_PRECISION * (((double) lat / 1E6));
        p.lon = COORDINATE_PRECISION * (((double) lng / 1E6));
        new_coordinates.push_back(p);
    }

    return new_coordinates;
}
