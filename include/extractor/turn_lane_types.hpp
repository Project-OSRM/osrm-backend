#ifndef OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_
#define OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_

#include "util/concurrent_id_map.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

#include <boost/functional/hash.hpp>

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <numeric> //partial_sum
#include <vector>

namespace osrm
{
namespace extractor
{

namespace TurnLaneType
{
const constexpr std::size_t NUM_TYPES = 11;

inline auto laneTypeToName(const std::size_t type_id)
{
    const static char *name[NUM_TYPES] = {"none",
                                          "straight",
                                          "sharp left",
                                          "left",
                                          "slight left",
                                          "slight right",
                                          "right",
                                          "sharp right",
                                          "uturn",
                                          "merge to left",
                                          "merge to right"};
    return name[type_id];
}

typedef std::uint16_t Mask;
const constexpr Mask empty = 0u;
const constexpr Mask none = 1u << 0u;
const constexpr Mask straight = 1u << 1u;
const constexpr Mask sharp_left = 1u << 2u;
const constexpr Mask left = 1u << 3u;
const constexpr Mask slight_left = 1u << 4u;
const constexpr Mask slight_right = 1u << 5u;
const constexpr Mask right = 1u << 6u;
const constexpr Mask sharp_right = 1u << 7u;
const constexpr Mask uturn = 1u << 8u;
const constexpr Mask merge_to_left = 1u << 9u;
const constexpr Mask merge_to_right = 1u << 10u;

} // namespace TurnLaneType

typedef std::vector<TurnLaneType::Mask> TurnLaneDescription;

// hash function for TurnLaneDescription
struct TurnLaneDescription_hash
{
    std::size_t operator()(const TurnLaneDescription &lane_description) const
    {
        std::size_t seed = 0;
        boost::hash_range(seed, lane_description.begin(), lane_description.end());
        return seed;
    }
};

using LaneDescriptionMap =
    util::ConcurrentIDMap<TurnLaneDescription, LaneDescriptionID, TurnLaneDescription_hash>;

using TurnLanesIndexedArray =
    std::tuple<std::vector<std::uint32_t>, std::vector<TurnLaneType::Mask>>;

inline TurnLanesIndexedArray transformTurnLaneMapIntoArrays(const LaneDescriptionMap &turn_lane_map)
{
    // could use some additional capacity? To avoid a copy during processing, though small data so
    // probably not that important.
    //
    // From the map, we construct an adjacency array that allows access from all IDs to the list of
    // associated Turn Lane Masks.
    //
    // turn lane offsets points into the locations of the turn_lane_masks array. We use a standard
    // adjacency array like structure to store the turn lane masks.
    std::vector<std::uint32_t> turn_lane_offsets(turn_lane_map.data.size() + 1); // + sentinel
    for (auto entry = turn_lane_map.data.begin(); entry != turn_lane_map.data.end(); ++entry)
        turn_lane_offsets[entry->second + 1] = entry->first.size();

    // inplace prefix sum
    std::partial_sum(turn_lane_offsets.begin(), turn_lane_offsets.end(), turn_lane_offsets.begin());

    // allocate the current masks
    std::vector<TurnLaneType::Mask> turn_lane_masks(turn_lane_offsets.back());
    for (auto entry = turn_lane_map.data.begin(); entry != turn_lane_map.data.end(); ++entry)
        std::copy(entry->first.begin(),
                  entry->first.end(),
                  turn_lane_masks.begin() + turn_lane_offsets[entry->second]);

    return std::make_tuple(std::move(turn_lane_offsets), std::move(turn_lane_masks));
}

} // namespace extractor
} // namespace osrm

#endif /* OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_ */
