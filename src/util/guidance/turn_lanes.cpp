#include "util/guidance/turn_lanes.hpp"

#include <algorithm>
#include <iostream>

#include <boost/assert.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{
LaneTupel::LaneTupel()
    : lanes_in_turn(0), first_lane_from_the_right(INVALID_LANEID)
{
    // basic constructor, set everything to zero
}

LaneTupel::LaneTupel(const LaneID lanes_in_turn, const LaneID first_lane_from_the_right)
    : lanes_in_turn(lanes_in_turn), first_lane_from_the_right(first_lane_from_the_right)
{
}

// comparation based on interpretation as unsigned 32bit integer
bool LaneTupel::operator==(const LaneTupel other) const
{
    static_assert(sizeof(LaneTupel) == sizeof(std::uint16_t),
                  "Comparation requires LaneTupel to be the of size 16Bit");
    return *reinterpret_cast<const std::uint16_t *>(this) ==
           *reinterpret_cast<const std::uint16_t *>(&other);
}

bool LaneTupel::operator!=(const LaneTupel other) const { return !(*this == other); }

// comparation based on interpretation as unsigned 32bit integer
bool LaneTupel::operator<(const LaneTupel other) const
{
    static_assert(sizeof(LaneTupel) == sizeof(std::uint16_t),
                  "Comparation requires LaneTupel to be the of size 16Bit");
    return *reinterpret_cast<const std::uint16_t *>(this) <
           *reinterpret_cast<const std::uint16_t *>(&other);
}

} // namespace guidance
} // namespace util
} // namespace osrm
