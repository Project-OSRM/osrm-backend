#ifndef OSRM_GUIDANCE_TOOLKIT_HPP_
#define OSRM_GUIDANCE_TOOLKIT_HPP_

#include "util/attributes.hpp"
#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/typedefs.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/query_node.hpp"

#include "extractor/guidance/discrete_angle.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>
#include <boost/tokenizer.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

using util::guidance::LaneTupelIdPair;
using LaneDataIdMap = std::unordered_map<LaneTupelIdPair, LaneDataID, boost::hash<LaneTupelIdPair>>;

using util::guidance::angularDeviation;
using util::guidance::entersRoundabout;
using util::guidance::leavesRoundabout;

namespace detail
{
const constexpr double DESIRED_SEGMENT_LENGTH = 10.0;

template <typename IteratorType>
util::Coordinate
getCoordinateFromCompressedRange(util::Coordinate current_coordinate,
                                 const IteratorType compressed_geometry_begin,
                                 const IteratorType compressed_geometry_end,
                                 const util::Coordinate final_coordinate,
                                 const std::vector<extractor::QueryNode> &query_nodes)
{
    const auto extractCoordinateFromNode =
        [](const extractor::QueryNode &node) -> util::Coordinate {
        return {node.lon, node.lat};
    };
    double distance_to_current_coordinate = 0;
    double distance_to_next_coordinate = 0;

    // get the length that is missing from the current segment to reach DESIRED_SEGMENT_LENGTH
    const auto getFactor = [](const double first_distance, const double second_distance) {
        BOOST_ASSERT(first_distance < detail::DESIRED_SEGMENT_LENGTH);
        double segment_length = second_distance - first_distance;
        BOOST_ASSERT(segment_length > 0);
        BOOST_ASSERT(second_distance >= detail::DESIRED_SEGMENT_LENGTH);
        double missing_distance = detail::DESIRED_SEGMENT_LENGTH - first_distance;
        return std::max(0., std::min(missing_distance / segment_length, 1.0));
    };

    for (auto compressed_geometry_itr = compressed_geometry_begin;
         compressed_geometry_itr != compressed_geometry_end;
         ++compressed_geometry_itr)
    {
        const auto next_coordinate =
            extractCoordinateFromNode(query_nodes[compressed_geometry_itr->node_id]);
        distance_to_next_coordinate =
            distance_to_current_coordinate +
            util::coordinate_calculation::haversineDistance(current_coordinate, next_coordinate);

        // reached point where coordinates switch between
        if (distance_to_next_coordinate >= detail::DESIRED_SEGMENT_LENGTH)
            return util::coordinate_calculation::interpolateLinear(
                getFactor(distance_to_current_coordinate, distance_to_next_coordinate),
                current_coordinate,
                next_coordinate);

        // prepare for next iteration
        current_coordinate = next_coordinate;
        distance_to_current_coordinate = distance_to_next_coordinate;
    }

    distance_to_next_coordinate =
        distance_to_current_coordinate +
        util::coordinate_calculation::haversineDistance(current_coordinate, final_coordinate);

    // reached point where coordinates switch between
    if (distance_to_current_coordinate < detail::DESIRED_SEGMENT_LENGTH &&
        distance_to_next_coordinate >= detail::DESIRED_SEGMENT_LENGTH)
        return util::coordinate_calculation::interpolateLinear(
            getFactor(distance_to_current_coordinate, distance_to_next_coordinate),
            current_coordinate,
            final_coordinate);
    else
        return final_coordinate;
}
} // namespace detail

// Finds a (potentially interpolated) coordinate that is DESIRED_SEGMENT_LENGTH away
// from the start of an edge
inline util::Coordinate
getRepresentativeCoordinate(const NodeID from_node,
                            const NodeID to_node,
                            const EdgeID via_edge_id,
                            const bool traverse_in_reverse,
                            const extractor::CompressedEdgeContainer &compressed_geometries,
                            const std::vector<extractor::QueryNode> &query_nodes)
{
    const auto extractCoordinateFromNode =
        [](const extractor::QueryNode &node) -> util::Coordinate {
        return {node.lon, node.lat};
    };

    // Uncompressed roads are simple, return the coordinate at the end
    if (!compressed_geometries.HasEntryForID(via_edge_id))
    {
        return extractCoordinateFromNode(traverse_in_reverse ? query_nodes[from_node]
                                                             : query_nodes[to_node]);
    }
    else
    {
        const auto &geometry = compressed_geometries.GetBucketReference(via_edge_id);

        const auto base_node_id = (traverse_in_reverse) ? to_node : from_node;
        const auto base_coordinate = extractCoordinateFromNode(query_nodes[base_node_id]);

        const auto final_node = (traverse_in_reverse) ? from_node : to_node;
        const auto final_coordinate = extractCoordinateFromNode(query_nodes[final_node]);

        if (traverse_in_reverse)
            return detail::getCoordinateFromCompressedRange(
                base_coordinate, geometry.rbegin(), geometry.rend(), final_coordinate, query_nodes);
        else
            return detail::getCoordinateFromCompressedRange(
                base_coordinate, geometry.begin(), geometry.end(), final_coordinate, query_nodes);
    }
}

// To simplify handling of Left/Right hand turns, we can mirror turns and write an intersection
// handler only for one side. The mirror function turns a left-hand turn in a equivalent right-hand
// turn and vice versa.
OSRM_ATTR_WARN_UNUSED
inline ConnectedRoad mirror(ConnectedRoad road)
{
    const constexpr DirectionModifier::Enum mirrored_modifiers[] = {DirectionModifier::UTurn,
                                                                    DirectionModifier::SharpLeft,
                                                                    DirectionModifier::Left,
                                                                    DirectionModifier::SlightLeft,
                                                                    DirectionModifier::Straight,
                                                                    DirectionModifier::SlightRight,
                                                                    DirectionModifier::Right,
                                                                    DirectionModifier::SharpRight};

    if (angularDeviation(road.turn.angle, 0) > std::numeric_limits<double>::epsilon())
    {
        road.turn.angle = 360 - road.turn.angle;
        road.turn.instruction.direction_modifier =
            mirrored_modifiers[road.turn.instruction.direction_modifier];
    }
    return road;
}

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

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TOOLKIT_HPP_
