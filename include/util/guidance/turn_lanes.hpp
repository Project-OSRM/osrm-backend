#ifndef OSRM_UTIL_GUIDANCE_TURN_LANES_HPP
#define OSRM_UTIL_GUIDANCE_TURN_LANES_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>

namespace osrm
{
namespace util
{
namespace guidance
{
class LaneTuple;
} // namespace guidance
} // namespace util
} // namespace osrm

namespace std
{
template <> struct hash<::osrm::util::guidance::LaneTuple>
{
    inline std::size_t operator()(const ::osrm::util::guidance::LaneTuple &bearing_class) const;
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
    LaneID first_lane_from_the_right;

    friend std::size_t hash_value(const LaneTuple &tup)
    {
        std::size_t seed{0};
        boost::hash_combine(seed, tup.lanes_in_turn);
        boost::hash_combine(seed, tup.first_lane_from_the_right);
        return seed;
    }
};

using LaneTupleIdPair = std::pair<util::guidance::LaneTuple, LaneDescriptionID>;
} // namespace guidance
} // namespace util
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_TURN_LANES_HPP */
