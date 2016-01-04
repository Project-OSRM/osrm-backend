#ifndef POLYLINE_FORMATTER_HPP
#define POLYLINE_FORMATTER_HPP

struct SegmentInformation;

#include "osrm/json_container.hpp"

#include <string>
#include <vector>

struct PolylineFormatter
{
    osrm::json::String printEncodedString(const std::vector<SegmentInformation> &polyline) const;

    osrm::json::Array printUnencodedString(const std::vector<SegmentInformation> &polyline) const;
};

#endif /* POLYLINE_FORMATTER_HPP */
