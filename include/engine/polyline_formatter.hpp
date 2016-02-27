#ifndef POLYLINE_FORMATTER_HPP
#define POLYLINE_FORMATTER_HPP

#include "engine/segment_information.hpp"
#include "osrm/json_container.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace engine
{

// Encodes geometry into polyline format, returning an encoded JSON object
// See: https://developers.google.com/maps/documentation/utilities/polylinealgorithm
util::json::String polylineEncodeAsJSON(const std::vector<SegmentInformation> &geometry);

// Does not encode the geometry in polyline format, instead returning an unencoded JSON object
util::json::Array polylineUnencodedAsJSON(const std::vector<SegmentInformation> &geometry);
} // namespace engine
} // namespace osrm

#endif /* POLYLINE_FORMATTER_HPP */
