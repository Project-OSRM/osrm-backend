#ifndef OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_
#define OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <numeric> //partial_sum
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>

#include "util/concurrent_id_map.hpp"
#include "util/json_container.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

namespace TurnLaneType
{
namespace detail
{
const constexpr std::size_t num_supported_lane_types = 11;

const constexpr char *translations[detail::num_supported_lane_types] = {"none",
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

} // namespace detail

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

inline std::string toString(const Mask lane_type)
{
    if (lane_type == 0)
        return "none";

    std::bitset<8 * sizeof(Mask)> mask(lane_type);
    std::string result = "";
    for (std::size_t lane_id_nr = 0; lane_id_nr < detail::num_supported_lane_types; ++lane_id_nr)
        if (mask[lane_id_nr])
            result += (result.empty() ? detail::translations[lane_id_nr]
                                      : (std::string(";") + detail::translations[lane_id_nr]));

    return result;
}

inline util::json::Array toJsonArray(const Mask lane_type)
{
    util::json::Array result;
    std::bitset<8 * sizeof(Mask)> mask(lane_type);
    for (std::size_t lane_id_nr = 0; lane_id_nr < detail::num_supported_lane_types; ++lane_id_nr)
        if (mask[lane_id_nr])
            result.values.push_back(detail::translations[lane_id_nr]);
    return result;
}
} // TurnLaneType

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

typedef util::ConcurrentIDMap<guidance::TurnLaneDescription,
                              LaneDescriptionID,
                              guidance::TurnLaneDescription_hash>
    LaneDescriptionMap;

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
    std::vector<guidance::TurnLaneType::Mask> turn_lane_masks(turn_lane_offsets.back());
    for (auto entry = turn_lane_map.data.begin(); entry != turn_lane_map.data.end(); ++entry)
        std::copy(entry->first.begin(),
                  entry->first.end(),
                  turn_lane_masks.begin() + turn_lane_offsets[entry->second]);

    return std::make_tuple(std::move(turn_lane_offsets), std::move(turn_lane_masks));
}

} // guidance
} // extractor
} // osrm

#endif /* OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_ */
