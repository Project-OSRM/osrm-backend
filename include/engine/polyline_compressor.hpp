#ifndef POLYLINECOMPRESSOR_H_
#define POLYLINECOMPRESSOR_H_

#include "util/coordinate.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace detail
{
    constexpr double POLYLINE_PRECISION = 1e5;
    constexpr double COORDINATE_TO_POLYLINE = POLYLINE_PRECISION / COORDINATE_PRECISION;
    constexpr double POLYLINE_TO_COORDINATE = COORDINATE_PRECISION / POLYLINE_PRECISION;
}

using CoordVectorForwardIter = std::vector<util::Coordinate>::const_iterator;
// Encodes geometry into polyline format.
// See: https://developers.google.com/maps/documentation/utilities/polylinealgorithm
std::string encodePolyline(CoordVectorForwardIter begin, CoordVectorForwardIter end);

// Decodes geometry from polyline format
// See: https://developers.google.com/maps/documentation/utilities/polylinealgorithm
std::vector<util::Coordinate> decodePolyline(const std::string &polyline);
}
}

#endif /* POLYLINECOMPRESSOR_H_ */
