#ifndef POLYLINE_FORMATTER_HPP
#define POLYLINE_FORMATTER_HPP

#include "osrm/json_container.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace engine
{

struct SegmentInformation;

struct PolylineFormatter
{
    util::json::String printEncodedString(const std::vector<SegmentInformation> &polyline) const;

    util::json::Array printUnencodedString(const std::vector<SegmentInformation> &polyline) const;
};
}
}

#endif /* POLYLINE_FORMATTER_HPP */
