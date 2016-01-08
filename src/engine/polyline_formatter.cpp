#include "engine/polyline_formatter.hpp"
#include "engine/polyline_compressor.hpp"
#include "osrm/coordinate.hpp"

namespace osrm
{
namespace engine
{

util::json::String polylineEncodeAsJSON(const std::vector<SegmentInformation> &polyline)
{
    return util::json::String(polylineEncode(polyline));
}

util::json::Array polylineUnencodedAsJSON(const std::vector<SegmentInformation> &polyline)
{
    util::json::Array json_geometry_array;
    for (const auto &segment : polyline)
    {
        if (segment.necessary)
        {
            util::json::Array json_coordinate;
            json_coordinate.values.push_back(segment.location.lat / COORDINATE_PRECISION);
            json_coordinate.values.push_back(segment.location.lon / COORDINATE_PRECISION);
            json_geometry_array.values.push_back(json_coordinate);
        }
    }
    return json_geometry_array;
}
}
}
