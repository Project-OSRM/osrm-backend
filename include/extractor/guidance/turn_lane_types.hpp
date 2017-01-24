#ifndef OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_
#define OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/functional/hash.hpp>

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

typedef std::unordered_map<guidance::TurnLaneDescription,
                           LaneDescriptionID,
                           guidance::TurnLaneDescription_hash>
    LaneDescriptionMap;

} // guidance
} // extractor
} // osrm

#endif /* OSRM_GUIDANCE_TURN_LANE_TYPES_HPP_ */
