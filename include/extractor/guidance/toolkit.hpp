#ifndef OSRM_GUIDANCE_TOOLKIT_HPP_
#define OSRM_GUIDANCE_TOOLKIT_HPP_

#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/query_node.hpp"
#include "extractor/suffix_table.hpp"

#include "extractor/guidance/classification_data.hpp"
#include "extractor/guidance/discrete_angle.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

using util::guidance::angularDeviation;

namespace detail
{
const constexpr double DESIRED_SEGMENT_LENGTH = 10.0;
const constexpr bool shiftable_ccw[] = {false, true, true, false, false, true, true, false};
const constexpr bool shiftable_cw[] = {false, false, true, true, false, false, true, true};
const constexpr std::uint8_t modifier_bounds[detail::num_direction_modifiers] = {
    0, 36, 93, 121, 136, 163, 220, 255};

const constexpr double discrete_angle_step_size = 360. / 24;

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
         compressed_geometry_itr != compressed_geometry_end; ++compressed_geometry_itr)
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
                current_coordinate, next_coordinate);

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
            current_coordinate, final_coordinate);
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

// shift an instruction around the degree circle in CCW order
inline DirectionModifier forcedShiftCCW(const DirectionModifier modifier)
{
    return static_cast<DirectionModifier>((static_cast<std::uint32_t>(modifier) + 1) %
                                          detail::num_direction_modifiers);
}

inline DirectionModifier shiftCCW(const DirectionModifier modifier)
{
    if (detail::shiftable_ccw[static_cast<int>(modifier)])
        return forcedShiftCCW(modifier);
    else
        return modifier;
}

// shift an instruction around the degree circle in CW order
inline DirectionModifier forcedShiftCW(const DirectionModifier modifier)
{
    return static_cast<DirectionModifier>(
        (static_cast<std::uint32_t>(modifier) + detail::num_direction_modifiers - 1) %
        detail::num_direction_modifiers);
}

inline DirectionModifier shiftCW(const DirectionModifier modifier)
{
    if (detail::shiftable_cw[static_cast<int>(modifier)])
        return forcedShiftCW(modifier);
    else
        return modifier;
}

inline bool isBasic(const TurnType type)
{
    return type == TurnType::Turn || type == TurnType::EndOfRoad;
}

inline bool isUturn(const TurnInstruction instruction)
{
    return isBasic(instruction.type) && instruction.direction_modifier == DirectionModifier::UTurn;
}

inline bool resolve(TurnInstruction &to_resolve, const TurnInstruction neighbor, bool resolve_cw)
{
    const auto shifted_turn = resolve_cw ? shiftCW(to_resolve.direction_modifier)
                                         : shiftCCW(to_resolve.direction_modifier);
    if (shifted_turn == neighbor.direction_modifier ||
        shifted_turn == to_resolve.direction_modifier)
        return false;

    to_resolve.direction_modifier = shifted_turn;
    return true;
}

inline bool resolveTransitive(TurnInstruction &first,
                              TurnInstruction &second,
                              const TurnInstruction third,
                              bool resolve_cw)
{
    if (resolve(second, third, resolve_cw))
    {
        first.direction_modifier =
            resolve_cw ? shiftCW(first.direction_modifier) : shiftCCW(first.direction_modifier);
        return true;
    }
    return false;
}

inline bool isSlightTurn(const TurnInstruction turn)
{
    return (isBasic(turn.type) || turn.type == TurnType::NoTurn) &&
           (turn.direction_modifier == DirectionModifier::Straight ||
            turn.direction_modifier == DirectionModifier::SlightRight ||
            turn.direction_modifier == DirectionModifier::SlightLeft);
}

inline bool isSlightModifier(const DirectionModifier direction_modifier)
{
    return (direction_modifier == DirectionModifier::Straight ||
            direction_modifier == DirectionModifier::SlightRight ||
            direction_modifier == DirectionModifier::SlightLeft);
}

inline bool isSharpTurn(const TurnInstruction turn)
{
    return isBasic(turn.type) && (turn.direction_modifier == DirectionModifier::SharpLeft ||
                                  turn.direction_modifier == DirectionModifier::SharpRight);
}

inline bool isStraight(const TurnInstruction turn)
{
    return (isBasic(turn.type) || turn.type == TurnType::NoTurn) &&
           turn.direction_modifier == DirectionModifier::Straight;
}

inline bool isConflict(const TurnInstruction first, const TurnInstruction second)
{
    return (first.type == second.type && first.direction_modifier == second.direction_modifier) ||
           (isStraight(first) && isStraight(second));
}

inline DiscreteAngle discretizeAngle(const double angle)
{
    BOOST_ASSERT(angle >= 0. && angle <= 360.);
    return DiscreteAngle(static_cast<std::uint8_t>((angle + 0.5 *detail::discrete_angle_step_size) / detail::discrete_angle_step_size));
}

inline double angleFromDiscreteAngle(const DiscreteAngle angle)
{
    return static_cast<double>(angle) * detail::discrete_angle_step_size + 0.5 * detail::discrete_angle_step_size;
}

inline double getAngularPenalty(const double angle, DirectionModifier modifier)
{
    // these are not aligned with getTurnDirection but represent an ideal center
    const double center[] = {0, 45, 90, 135, 180, 225, 270, 315};
    return angularDeviation(center[static_cast<int>(modifier)], angle);
}

inline double getTurnConfidence(const double angle, TurnInstruction instruction)
{

    // special handling of U-Turns and Roundabout
    if (!isBasic(instruction.type) || instruction.direction_modifier == DirectionModifier::UTurn)
        return 1.0;

    const double deviations[] = {0, 45, 50, 30, 20, 30, 50, 45};
    const double difference = getAngularPenalty(angle, instruction.direction_modifier);
    const double max_deviation = deviations[static_cast<int>(instruction.direction_modifier)];
    return 1.0 - (difference / max_deviation) * (difference / max_deviation);
}

// swaps left <-> right modifier types
inline DirectionModifier mirrorDirectionModifier(const DirectionModifier modifier)
{
    const constexpr DirectionModifier results[] = {
        DirectionModifier::UTurn,      DirectionModifier::SharpLeft, DirectionModifier::Left,
        DirectionModifier::SlightLeft, DirectionModifier::Straight,  DirectionModifier::SlightRight,
        DirectionModifier::Right,      DirectionModifier::SharpRight};
    return results[modifier];
}

inline bool canBeSuppressed(const TurnType type)
{
    if (type == TurnType::Turn)
        return true;
    return false;
}

inline bool isLowPriorityRoadClass(const FunctionalRoadClass road_class)
{
    return road_class == FunctionalRoadClass::LOW_PRIORITY_ROAD ||
           road_class == FunctionalRoadClass::SERVICE;
}

inline bool isDistinct(const DirectionModifier first, const DirectionModifier second)
{
    if ((first + 1) % detail::num_direction_modifiers == second)
        return false;

    if ((second + 1) % detail::num_direction_modifiers == first)
        return false;

    return true;
}

inline std::pair<std::string, std::string> getPrefixAndSuffix(const std::string &data)
{
    const auto suffix_pos = data.find_last_of(' ');
    if (suffix_pos == std::string::npos)
        return {};

    const auto prefix_pos = data.find_first_of(' ');
    auto result = std::make_pair(data.substr(0, prefix_pos), data.substr(suffix_pos + 1));
    boost::to_lower(result.first);
    boost::to_lower(result.second);
    return result;
}

inline bool requiresNameAnnounced(const std::string &from,
                                  const std::string &to,
                                  const SuffixTable &suffix_table)
{
    //first is empty and the second is not
    if(from.empty() && !to.empty())
        return true;

    // FIXME, handle in profile to begin with?
    // this uses the encoding of references in the profile, which is very BAD
    // Input for this function should be a struct separating streetname, suffix (e.g. road,
    // boulevard, North, West ...), and a list of references
    std::string from_name;
    std::string from_ref;
    std::string to_name;
    std::string to_ref;

    // Split from the format "{name} ({ref})" -> name, ref
    auto split = [](const std::string &name, std::string &out_name, std::string &out_ref) {
        const auto ref_begin = name.find_first_of('(');
        if (ref_begin != std::string::npos)
        {
            if (ref_begin != 0)
                out_name = name.substr(0, ref_begin - 1);
            const auto ref_end = name.find_first_of(')');
            out_ref = name.substr(ref_begin + 1, ref_end - ref_begin - 1);
        }
        else
        {
            out_name = name;
        }
    };

    split(from, from_name, from_ref);
    split(to, to_name, to_ref);

    // check similarity of names
    const auto names_are_empty = from_name.empty() && to_name.empty();
    const auto name_is_contained =
        boost::starts_with(from_name, to_name) || boost::starts_with(to_name, from_name);

    const auto checkForPrefixOrSuffixChange =
        [](const std::string &first, const std::string &second, const SuffixTable &suffix_table) {

            const auto first_prefix_and_suffixes = getPrefixAndSuffix(first);
            const auto second_prefix_and_suffixes = getPrefixAndSuffix(second);
            // reverse strings, get suffices and reverse them to get prefixes
            const auto checkTable = [&](const std::string str) {
                return str.empty() || suffix_table.isSuffix(str);
            };

            const bool is_prefix_change = [&]() -> bool {
                if (!checkTable(first_prefix_and_suffixes.first))
                    return false;
                if (!checkTable(first_prefix_and_suffixes.first))
                    return false;
                return !first.compare(first_prefix_and_suffixes.first.length(), std::string::npos,
                                     second, second_prefix_and_suffixes.first.length(),
                                     std::string::npos);
            }();

            const bool is_suffix_change = [&]() -> bool {
                if (!checkTable(first_prefix_and_suffixes.second))
                    return false;
                if (!checkTable(first_prefix_and_suffixes.second))
                    return false;
                return !first.compare(0, first.length() - first_prefix_and_suffixes.second.length(),
                                     second, 0, second.length() - second_prefix_and_suffixes.second.length());
            }();

            return is_prefix_change || is_suffix_change;
        };

    const auto is_suffix_change = checkForPrefixOrSuffixChange(from_name, to_name, suffix_table);
    const auto names_are_equal = from_name == to_name || name_is_contained || is_suffix_change;
    const auto name_is_removed = !from_name.empty() && to_name.empty();
    // references are contained in one another
    const auto refs_are_empty = from_ref.empty() && to_ref.empty();
    const auto ref_is_contained =
        from_ref.empty() || to_ref.empty() ||
        (from_ref.find(to_ref) != std::string::npos || to_ref.find(from_ref) != std::string::npos);
    const auto ref_is_removed = !from_ref.empty() && to_ref.empty();

    const auto obvious_change = (names_are_empty && refs_are_empty) ||
                                (names_are_equal && ref_is_contained) ||
                                (names_are_equal && refs_are_empty) || name_is_removed ||
                                ref_is_removed || is_suffix_change;

    return !obvious_change;
}

inline int getPriority(const FunctionalRoadClass road_class)
{
    // The road priorities indicate which roads can bee seen as more or less equal.
    // They are used in Fork-Discovery. Possibly should be moved to profiles post v5?
    // A fork can happen between road types that are at most 1 priority apart from each other
    const constexpr int road_priority[] = {10, 0, 10, 2,  10, 4,  10, 6,
                                           10, 8, 10, 11, 10, 12, 10, 14};
    return road_priority[static_cast<int>(road_class)];
}

inline bool canBeSeenAsFork(const FunctionalRoadClass first, const FunctionalRoadClass second)
{
    // forks require similar road categories
    // Based on the priorities assigned above, we can set forks only if the road priorities match
    // closely.
    // Potentially we could include features like number of lanes here and others?
    // Should also be moved to profiles
    return std::abs(getPriority(first) - getPriority(second)) <= 1;
}

// To simplify handling of Left/Right hand turns, we can mirror turns and write an intersection
// handler only for one side. The mirror function turns a left-hand turn in a equivalent right-hand
// turn and vice versa.
inline ConnectedRoad mirror(ConnectedRoad road)
{
    const constexpr DirectionModifier mirrored_modifiers[] = {
        DirectionModifier::UTurn,      DirectionModifier::SharpLeft, DirectionModifier::Left,
        DirectionModifier::SlightLeft, DirectionModifier::Straight,  DirectionModifier::SlightRight,
        DirectionModifier::Right,      DirectionModifier::SharpRight};

    if (angularDeviation(road.turn.angle, 0) > std::numeric_limits<double>::epsilon())
    {
        road.turn.angle = 360 - road.turn.angle;
        road.turn.instruction.direction_modifier =
            mirrored_modifiers[road.turn.instruction.direction_modifier];
    }
    return road;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TOOLKIT_HPP_
