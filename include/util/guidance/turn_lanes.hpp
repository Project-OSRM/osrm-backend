#ifndef OSRM_UTIL_GUIDANCE_TURN_LANES_HPP
#define OSRM_UTIL_GUIDANCE_TURN_LANES_HPP

#include "util/concurrent_id_map.hpp"
#include "util/std_hash.hpp"
#include "util/typedefs.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace osrm::util::guidance
{
class LaneTuple;
class LaneTupleIdPair;
} // namespace osrm::util::guidance

namespace osrm::util::guidance
{

// The mapping of turn lanes can be done two values. We describe every turn by the number of
// contributing lanes and the first lane from the right..
// Given a road like this:
//           |   |   |
//           |   |   |
// -----------       |
//      -^           |
// -----------        -------------
//      -^ ->
// --------------------------------
//      -v       |
// ----------    |
//          |    |
//
// we generate a set of tuples in the form of:
// (2,1), (1,1), (1,0) for left, through and right respectively
class LaneTuple
{
  public:
    LaneTuple();
    LaneTuple(const LaneID lanes_in_turn, const LaneID first_lane_from_the_right);

    bool operator==(const LaneTuple other) const;
    bool operator!=(const LaneTuple other) const;

    LaneID lanes_in_turn;
    LaneID first_lane_from_the_right; // is INVALID_LANEID when no lanes present
};

class LaneTupleIdPair
{
  public:
    util::guidance::LaneTuple first;
    LaneDescriptionID second;

    bool operator==(const LaneTupleIdPair &other) const;
};

using LaneDataIdMap = ConcurrentIDMap<LaneTupleIdPair, LaneDataID>;

} // namespace osrm::util::guidance

namespace std
{
template <> struct hash<::osrm::util::guidance::LaneTuple>
{
    inline std::size_t operator()(const ::osrm::util::guidance::LaneTuple &lane_tuple) const
    {
        std::size_t seed{0};
        hash_combine(seed, lane_tuple.lanes_in_turn);
        hash_combine(seed, lane_tuple.first_lane_from_the_right);
        return seed;
    }
};

template <> struct hash<::osrm::util::guidance::LaneTupleIdPair>
{
    inline std::size_t
    operator()(const ::osrm::util::guidance::LaneTupleIdPair &lane_tuple_id_pair) const
    {
        std::size_t seed{0};
        hash_combine(seed, lane_tuple_id_pair.first);
        hash_combine(seed, lane_tuple_id_pair.second);
        return seed;
    }
};
} // namespace std

#endif /* OSRM_UTIL_GUIDANCE_TURN_LANES_HPP */
