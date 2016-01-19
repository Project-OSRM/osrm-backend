#ifndef TRAVEL_MODE_HPP
#define TRAVEL_MODE_HPP

namespace osrm
{
namespace extractor
{

using TravelMode = unsigned char;
}
}

const constexpr osrm::extractor::TravelMode TRAVEL_MODE_INACCESSIBLE = 0;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_DEFAULT = 1;
#endif /* TRAVEL_MODE_HPP */
