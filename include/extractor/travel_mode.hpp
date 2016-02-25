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
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_DRIVING = 1;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_CYCLING = 2;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_WALKING = 3;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_FERRY = 4;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_TRAIN = 5;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_PUSHING_BIKE = 6;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_MOVABLE_BRIDGE = 7;
// FIXME only for testbot.lua
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_STEPS_UP = 8;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_STEPS_DOWN = 9;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_RIVER_UP = 10;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_RIVER_DOWN = 11;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_ROUTE = 12;

#endif /* TRAVEL_MODE_HPP */
