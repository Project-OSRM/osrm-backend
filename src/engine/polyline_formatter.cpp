#include "engine/polyline_formatter.hpp"

#include "engine/polyline_compressor.hpp"
#include "engine/segment_information.hpp"

#include "osrm/coordinate.hpp"

osrm::json::String
PolylineFormatter::printEncodedString(const std::vector<SegmentInformation> &polyline) const
{
    return osrm::json::String(PolylineCompressor().get_encoded_string(polyline));
}

osrm::json::Array
PolylineFormatter::printUnencodedString(const std::vector<SegmentInformation> &polyline) const
{
    osrm::json::Array json_geometry_array;
    for (const auto &segment : polyline)
    {
        if (segment.necessary)
        {
            osrm::json::Array json_coordinate;
            json_coordinate.values.push_back(segment.location.lat / COORDINATE_PRECISION);
            json_coordinate.values.push_back(segment.location.lon / COORDINATE_PRECISION);
            json_geometry_array.values.push_back(json_coordinate);
        }
    }
    return json_geometry_array;
}
