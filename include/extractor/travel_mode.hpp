/*

Copyright (c) 2016, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TRAVEL_MODE_HPP
#define TRAVEL_MODE_HPP

#include <cstdint>

namespace osrm
{
namespace extractor
{

// This is a char instead of a typed enum, so that we can
// pack it into e.g. a "TravelMode mode : 4" packed bitfield
using TravelMode = std::uint8_t;
}
}

const constexpr osrm::extractor::TravelMode TRAVEL_MODE_INACCESSIBLE = 0;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_DRIVING = 1;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_CYCLING = 2;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_WALKING = 3;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_FERRY = 4;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_TRAIN = 5;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_PUSHING_BIKE = 6;
// FIXME only for testbot.lua
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_STEPS_UP = 8;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_STEPS_DOWN = 9;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_RIVER_UP = 10;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_RIVER_DOWN = 11;
const constexpr osrm::extractor::TravelMode TRAVEL_MODE_ROUTE = 12;

#endif /* TRAVEL_MODE_HPP */
