#ifndef TRAVEL_MODE_HPP
#define TRAVEL_MODE_HPP

namespace osrm
{
namespace extractor
{

// This is a char instead of a typed enum, so that we can
// pack it into e.g. a "TravelMode mode : 4" packed bitfield
using TravelMode = unsigned char;
}
}

const constexpr osrm::extractor::TravelMode TRAVEL_MODE_INACCESSIBLE = 0;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_DEFAULT = 1;
#endif /* TRAVEL_MODE_HPP */
