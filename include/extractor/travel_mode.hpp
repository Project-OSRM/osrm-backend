#ifndef TRAVEL_MODE_HPP
#define TRAVEL_MODE_HPP

namespace osrm
{
namespace extractor
{

using TravelMode = unsigned char;

}
}

namespace {
static const osrm::extractor::TravelMode TRAVEL_MODE_INACCESSIBLE = 0;
static const osrm::extractor::TravelMode TRAVEL_MODE_DEFAULT = 1;
}
#endif /* TRAVEL_MODE_HPP */
