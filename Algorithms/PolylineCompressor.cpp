/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "PolylineCompressor.h"
#include "../Util/StringUtil.h"

//TODO: return vector of start indices for each leg

void PolylineCompressor::encodeVectorSignedNumber(std::vector<int> &numbers, std::string &output)
    const
{
    const unsigned end = numbers.size();
    for (unsigned i = 0; i < end; ++i)
    {
        numbers[i] <<= 1;
        if (numbers[i] < 0)
        {
            numbers[i] = ~(numbers[i]);
        }
    }
    for (const int number: numbers)
    {
        encodeNumber(number, output);
    }
}

void PolylineCompressor::encodeNumber(int number_to_encode, std::string &output) const
{
    while (number_to_encode >= 0x20)
    {
        int next_value = (0x20 | (number_to_encode & 0x1f)) + 63;
        output += static_cast<char>(next_value);
        if (92 == next_value)
        {
            output += static_cast<char>(next_value);
        }
        number_to_encode >>= 5;
    }

    number_to_encode += 63;
    output += static_cast<char>(number_to_encode);
    if (92 == number_to_encode)
    {
        output += static_cast<char>(number_to_encode);
    }
}

JSON::String PolylineCompressor::printEncodedString(const std::vector<SegmentInformation> &polyline) const
{
    std::string output;
    std::vector<int> delta_numbers;
    if (!polyline.empty())
    {
        FixedPointCoordinate last_coordinate = polyline[0].location;
        delta_numbers.emplace_back(last_coordinate.lat);
        delta_numbers.emplace_back(last_coordinate.lon);
        for (const auto & segment : polyline)
        {
            if (segment.necessary)
            {
                int lat_diff = segment.location.lat - last_coordinate.lat;
                int lon_diff = segment.location.lon - last_coordinate.lon;
                delta_numbers.emplace_back(lat_diff);
                delta_numbers.emplace_back(lon_diff);
                last_coordinate = segment.location;
            }
        }
        encodeVectorSignedNumber(delta_numbers, output);
    }
    JSON::String return_value(output);
    return return_value;
}

JSON::String PolylineCompressor::printEncodedString(const std::vector<FixedPointCoordinate> &polyline) const
{
    std::string output;
    std::vector<int> delta_numbers(2 * polyline.size());
    if (!polyline.empty())
    {
        delta_numbers[0] = polyline[0].lat;
        delta_numbers[1] = polyline[0].lon;
        for (unsigned i = 1; i < polyline.size(); ++i)
        {
            int lat_diff = polyline[i].lat - polyline[i - 1].lat;
            int lon_diff = polyline[i].lon - polyline[i - 1].lon;
            delta_numbers[(2 * i)] = (lat_diff);
            delta_numbers[(2 * i) + 1] = (lon_diff);
        }
        encodeVectorSignedNumber(delta_numbers, output);
    }
    JSON::String return_value(output);
    return return_value;
}


JSON::Array PolylineCompressor::printUnencodedString(const std::vector<FixedPointCoordinate> &polyline) const
{
    JSON::Array json_geometry_array;
    for( const auto & coordinate : polyline)
    {
        std::string tmp, output;
        FixedPointCoordinate::convertInternalLatLonToString(coordinate.lat, tmp);
        output += (tmp + ",");
        FixedPointCoordinate::convertInternalLatLonToString(coordinate.lon, tmp);
        output += tmp;
        json_geometry_array.values.push_back(output);
    }
    return json_geometry_array;
}

JSON::Array PolylineCompressor::printUnencodedString(const std::vector<SegmentInformation> &polyline) const
{
    JSON::Array json_geometry_array;
    for( const auto & segment : polyline)
    {
        if (segment.necessary)
        {
            std::string tmp, output;
            FixedPointCoordinate::convertInternalLatLonToString(segment.location.lat, tmp);
            output += (tmp + ",");
            FixedPointCoordinate::convertInternalLatLonToString(segment.location.lon, tmp);
            output += tmp;
            json_geometry_array.values.push_back(output);
        }
    }
    return json_geometry_array;
}
