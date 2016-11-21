#ifndef OSRM_GUIDANCE_TOOLKIT_HPP_
#define OSRM_GUIDANCE_TOOLKIT_HPP_

#include "util/attributes.hpp"
#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/query_node.hpp"

#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/road_classification.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/tokenizer.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

using util::guidance::LaneTupleIdPair;
using LaneDataIdMap = std::unordered_map<LaneTupleIdPair, LaneDataID, boost::hash<LaneTupleIdPair>>;

using util::guidance::angularDeviation;
using util::guidance::entersRoundabout;
using util::guidance::leavesRoundabout;

// To simplify handling of Left/Right hand turns, we can mirror turns and write an intersection
// handler only for one side. The mirror function turns a left-hand turn in a equivalent right-hand
// turn and vice versa.

inline bool hasRoundaboutType(const TurnInstruction instruction)
{
    using namespace extractor::guidance::TurnType;
    const constexpr TurnType::Enum valid_types[] = {TurnType::EnterRoundabout,
                                                    TurnType::EnterAndExitRoundabout,
                                                    TurnType::EnterRotary,
                                                    TurnType::EnterAndExitRotary,
                                                    TurnType::EnterRoundaboutIntersection,
                                                    TurnType::EnterAndExitRoundaboutIntersection,
                                                    TurnType::EnterRoundaboutAtExit,
                                                    TurnType::ExitRoundabout,
                                                    TurnType::EnterRotaryAtExit,
                                                    TurnType::ExitRotary,
                                                    TurnType::EnterRoundaboutIntersectionAtExit,
                                                    TurnType::ExitRoundaboutIntersection,
                                                    TurnType::StayOnRoundabout};

    const auto *first = valid_types;
    const auto *last = first + sizeof(valid_types) / sizeof(valid_types[0]);

    return std::find(first, last, instruction.type) != last;
}

// Public service vehicle lanes and similar can introduce additional lanes into the lane string that
// are not specifically marked for left/right turns. This function can be used from the profile to
// trim the lane string appropriately
//
// left|throught|
// in combination with lanes:psv:forward=1
// will be corrected to left|throught, since the final lane is not drivable.
// This is in contrast to a situation with lanes:psv:forward=0 (or not set) where left|through|
// represents left|through|through
OSRM_ATTR_WARN_UNUSED
inline std::string
trimLaneString(std::string lane_string, std::int32_t count_left, std::int32_t count_right)
{
    if (count_left)
    {
        bool sane = count_left < static_cast<std::int32_t>(lane_string.size());
        for (std::int32_t i = 0; i < count_left; ++i)
            // this is adjusted for our fake pipe. The moment cucumber can handle multiple escaped
            // pipes, the '&' part can be removed
            if (lane_string[i] != '|')
            {
                sane = false;
                break;
            }

        if (sane)
        {
            lane_string.erase(lane_string.begin(), lane_string.begin() + count_left);
        }
    }
    if (count_right)
    {
        bool sane = count_right < static_cast<std::int32_t>(lane_string.size());
        for (auto itr = lane_string.rbegin();
             itr != lane_string.rend() && itr != lane_string.rbegin() + count_right;
             ++itr)
        {
            if (*itr != '|')
            {
                sane = false;
                break;
            }
        }
        if (sane)
            lane_string.resize(lane_string.size() - count_right);
    }
    return lane_string;
}

// https://github.com/Project-OSRM/osrm-backend/issues/2638
// It can happen that some lanes are not drivable by car. Here we handle this tagging scheme
// (vehicle:lanes) to filter out not-allowed roads
// lanes=3
// turn:lanes=left|through|through|right
// vehicle:lanes=yes|yes|no|yes
// bicycle:lanes=yes|no|designated|yes
OSRM_ATTR_WARN_UNUSED
inline std::string applyAccessTokens(std::string lane_string, const std::string &access_tokens)
{
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
    tokenizer tokens(lane_string, sep);
    tokenizer access(access_tokens, sep);

    // strings don't match, don't do anything
    if (std::distance(std::begin(tokens), std::end(tokens)) !=
        std::distance(std::begin(access), std::end(access)))
        return lane_string;

    std::string result_string = "";
    const static std::string yes = "yes";

    for (auto token_itr = std::begin(tokens), access_itr = std::begin(access);
         token_itr != std::end(tokens);
         ++token_itr, ++access_itr)
    {
        if (*access_itr == yes)
        {
            // we have to add this in front, because the next token could be invalid. Doing this on
            // non-empty strings makes sure that the token string will be valid in the end
            if (!result_string.empty())
                result_string += '|';

            result_string += *token_itr;
        }
    }
    return result_string;
}

inline bool obviousByRoadClass(const RoadClassification in_classification,
                               const RoadClassification obvious_candidate,
                               const RoadClassification compare_candidate)
{
    // lower numbers are of higher priority
    const bool has_high_priority = PRIORITY_DISTINCTION_FACTOR * obvious_candidate.GetPriority() <
                                   compare_candidate.GetPriority();

    const bool continues_on_same_class = in_classification == obvious_candidate;
    return (has_high_priority && continues_on_same_class) ||
           (!obvious_candidate.IsLowPriorityRoadClass() &&
            !in_classification.IsLowPriorityRoadClass() &&
            compare_candidate.IsLowPriorityRoadClass());
}

/* We use the sum of least squares to calculate a linear regression through our
* coordinates.
* This regression gives a good idea of how the road can be perceived and corrects for
* initial and final corrections
*/
inline std::pair<util::Coordinate, util::Coordinate>
leastSquareRegression(const std::vector<util::Coordinate> &coordinates)
{
    BOOST_ASSERT(coordinates.size() >= 2);
    double sum_lon = 0, sum_lat = 0, sum_lon_lat = 0, sum_lon_lon = 0;
    double min_lon = static_cast<double>(toFloating(coordinates.front().lon));
    double max_lon = static_cast<double>(toFloating(coordinates.front().lon));
    for (const auto coord : coordinates)
    {
        min_lon = std::min(min_lon, static_cast<double>(toFloating(coord.lon)));
        max_lon = std::max(max_lon, static_cast<double>(toFloating(coord.lon)));
        sum_lon += static_cast<double>(toFloating(coord.lon));
        sum_lon_lon +=
            static_cast<double>(toFloating(coord.lon)) * static_cast<double>(toFloating(coord.lon));
        sum_lat += static_cast<double>(toFloating(coord.lat));
        sum_lon_lat +=
            static_cast<double>(toFloating(coord.lon)) * static_cast<double>(toFloating(coord.lat));
    }

    const auto dividend = coordinates.size() * sum_lon_lat - sum_lon * sum_lat;
    const auto divisor = coordinates.size() * sum_lon_lon - sum_lon * sum_lon;
    if (std::abs(divisor) < std::numeric_limits<double>::epsilon())
        return std::make_pair(coordinates.front(), coordinates.back());

    // slope of the regression line
    const auto slope = dividend / divisor;
    const auto intercept = (sum_lat - slope * sum_lon) / coordinates.size();

    const auto GetLatAtLon = [intercept,
                              slope](const util::FloatLongitude longitude) -> util::FloatLatitude {
        return {intercept + slope * static_cast<double>((longitude))};
    };

    const util::Coordinate regression_first = {
        toFixed(util::FloatLongitude{min_lon - 1}),
        toFixed(util::FloatLatitude(GetLatAtLon(util::FloatLongitude{min_lon - 1})))};
    const util::Coordinate regression_end = {
        toFixed(util::FloatLongitude{max_lon + 1}),
        toFixed(util::FloatLatitude(GetLatAtLon(util::FloatLongitude{max_lon + 1})))};

    return {regression_first, regression_end};
}

inline std::uint8_t getLaneCountAtIntersection(const NodeID intersection_node,
                                               const util::NodeBasedDynamicGraph &node_based_graph)
{
    std::uint8_t lanes = 0;
    for (const EdgeID onto_edge : node_based_graph.GetAdjacentEdgeRange(intersection_node))
        lanes = std::max(
            lanes, node_based_graph.GetEdgeData(onto_edge).road_classification.GetNumberOfLanes());
    return lanes;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TOOLKIT_HPP_
