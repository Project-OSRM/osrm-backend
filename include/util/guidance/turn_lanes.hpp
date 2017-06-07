#ifndef OSRM_UTIL_GUIDANCE_TURN_LANES_HPP
#define OSRM_UTIL_GUIDANCE_TURN_LANES_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include "util/concurrent_id_map.hpp"
#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{
class LaneTuple;
class LaneTupleIdPair;
} // namespace guidance
} // namespace util
} // namespace osrm

namespace std
{
template <> struct hash<::osrm::util::guidance::LaneTuple>
{
    inline std::size_t operator()(const ::osrm::util::guidance::LaneTuple &bearing_class) const;
};
template <> struct hash<::osrm::util::guidance::LaneTupleIdPair>
{
    inline std::size_t
    operator()(const ::osrm::util::guidance::LaneTupleIdPair &bearing_class) const;
};
} // namespace std

namespace osrm
{
namespace util
{
namespace guidance
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

    friend std::size_t hash_value(const LaneTuple &tup)
    {
        std::size_t seed{0};
        boost::hash_combine(seed, tup.lanes_in_turn);
        boost::hash_combine(seed, tup.first_lane_from_the_right);
        return seed;
    }
};

class LaneTupleIdPair
{
  public:
    util::guidance::LaneTuple first;
    LaneDescriptionID second;

    bool operator==(const LaneTupleIdPair &other) const;

    friend std::size_t hash_value(const LaneTupleIdPair &pair)
    {
        std::size_t seed{0};
        boost::hash_combine(seed, pair.first);
        boost::hash_combine(seed, pair.second);
        return seed;
    }
};

using LaneDataIdMap = ConcurrentIDMap<LaneTupleIdPair, LaneDataID, boost::hash<LaneTupleIdPair>>;

} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_TURN_LANES_HPP */
