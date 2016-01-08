#ifndef POLYLINECOMPRESSOR_H_
#define POLYLINECOMPRESSOR_H_

#include "osrm/coordinate.hpp"
#include "engine/segment_information.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
// Encodes geometry into polyline format.
// See: https://developers.google.com/maps/documentation/utilities/polylinealgorithm
std::string polylineEncode(const std::vector<SegmentInformation> &geometry);

// Decodes geometry from polyline format
// See: https://developers.google.com/maps/documentation/utilities/polylinealgorithm
std::vector<util::FixedPointCoordinate> polylineDecode(const std::string &polyline);
}
}

#endif /* POLYLINECOMPRESSOR_H_ */
