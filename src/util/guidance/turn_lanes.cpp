#include "util/guidance/turn_lanes.hpp"

#include <algorithm>
#include <iostream>
#include <tuple>

#include <boost/assert.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{
LaneTuple::LaneTuple() : lanes_in_turn(0), first_lane_from_the_right(INVALID_LANEID)
{
    // basic constructor, set everything to zero
}

LaneTuple::LaneTuple(const LaneID lanes_in_turn, const LaneID first_lane_from_the_right)
    : lanes_in_turn(lanes_in_turn), first_lane_from_the_right(first_lane_from_the_right)
{
}

// comparation based on interpretation as unsigned 32bit integer
bool LaneTuple::operator==(const LaneTuple other) const
{
    return std::tie(lanes_in_turn, first_lane_from_the_right) ==
           std::tie(other.lanes_in_turn, other.first_lane_from_the_right);
}

bool LaneTuple::operator!=(const LaneTuple other) const { return !(*this == other); }

bool LaneTupleIdPair::operator==(const LaneTupleIdPair &other) const
{
    return other.first == first && other.second == second;
}

} // namespace guidance
} // namespace util
} // namespace osrm
