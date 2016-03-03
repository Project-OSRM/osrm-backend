#ifndef OSRM_GUIDANCE_TOOLKIT_HPP_
#define OSRM_GUIDANCE_TOOLKIT_HPP_

#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/query_node.hpp"

#include "extractor/guidance/discrete_angle.hpp"
#include "extractor/guidance/classification_data.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include <map>
#include <cmath>

namespace osrm
{
namespace extractor
{
namespace guidance
{

namespace detail
{
const constexpr double DESIRED_SEGMENT_LENGTH = 10.0;
const constexpr bool shiftable_ccw[] = {false, true, true, false, false, true, true, false};
const constexpr bool shiftable_cw[] = {false, false, true, true, false, false, true, true};
const constexpr uint8_t modifier_bounds[detail::num_direction_modifiers] = {0,   36,  93,  121,
                                                                            136, 163, 220, 255};
const constexpr double discrete_angle_step_size = 360. / 256.;

template <typename IteratorType>
util::Coordinate
getCoordinateFromCompressedRange(util::Coordinate current_coordinate,
                                 IteratorType compressed_geometry_begin,
                                 const IteratorType compressed_geometry_end,
                                 const util::Coordinate final_coordinate,
                                 const std::vector<extractor::QueryNode> &query_nodes)
{
    const auto extractCoordinateFromNode = [](const extractor::QueryNode &node) -> util::Coordinate
    {
        return {node.lon, node.lat};
    };
    double distance_to_current_coordinate = 0;
    double distance_to_next_coordinate = 0;

    // get the length that is missing from the current segment to reach DESIRED_SEGMENT_LENGTH
    const auto getFactor = [](const double first_distance, const double second_distance)
    {
        BOOST_ASSERT(first_distance < detail::DESIRED_SEGMENT_LENGTH);
        double segment_length = second_distance - first_distance;
        BOOST_ASSERT(segment_length > 0);
        BOOST_ASSERT(second_distance >= detail::DESIRED_SEGMENT_LENGTH);
        double missing_distance = detail::DESIRED_SEGMENT_LENGTH - first_distance;
        return missing_distance / segment_length;
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
    if (distance_to_next_coordinate >= detail::DESIRED_SEGMENT_LENGTH)
        return util::coordinate_calculation::interpolateLinear(
            getFactor(distance_to_current_coordinate, distance_to_next_coordinate),
            current_coordinate, final_coordinate);
    else
        return final_coordinate;
}
} // namespace detail

// Finds a (potentially inteprolated) coordinate that is DESIRED_SEGMENT_LENGTH away
// from the start of an edge
inline util::Coordinate
getRepresentativeCoordinate(const NodeID from_node,
                            const NodeID to_node,
                            const EdgeID via_edge_id,
                            const bool traverse_in_reverse,
                            const extractor::CompressedEdgeContainer &compressed_geometries,
                            const std::vector<extractor::QueryNode> &query_nodes)
{
    const auto extractCoordinateFromNode = [](const extractor::QueryNode &node) -> util::Coordinate
    {
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
    return static_cast<DirectionModifier>((static_cast<uint32_t>(modifier) + 1) %
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
        (static_cast<uint32_t>(modifier) + detail::num_direction_modifiers - 1) %
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
    return DiscreteAngle(static_cast<uint8_t>(angle / detail::discrete_angle_step_size));
}

inline double angleFromDiscreteAngle(const DiscreteAngle angle)
{
    return static_cast<double>(angle) * detail::discrete_angle_step_size;
}

inline double angularDeviation(const double angle, const double from)
{
    const double deviation = std::abs(angle - from);
    return std::min(360 - deviation, deviation);
}

inline double getAngularPenalty(const double angle, TurnInstruction instruction)
{
    const double center[] = {0, 45, 90, 135, 180, 225, 270, 315};
    return angularDeviation(center[static_cast<int>(instruction.direction_modifier)], angle);
}

inline double getTurnConfidence(const double angle, TurnInstruction instruction)
{

    // special handling of U-Turns and Roundabout
    if (!isBasic(instruction.type) || instruction.direction_modifier == DirectionModifier::UTurn)
        return 1.0;

    const double deviations[] = {0, 45, 50, 35, 10, 35, 50, 45};
    const double difference = getAngularPenalty(angle, instruction);
    const double max_deviation = deviations[static_cast<int>(instruction.direction_modifier)];
    return 1.0 - (difference / max_deviation) * (difference / max_deviation);
}

// Translates between angles and their human-friendly directional representation
inline DirectionModifier getTurnDirection(const double angle)
{
    // An angle of zero is a u-turn
    // 180 goes perfectly straight
    // 0-180 are right turns
    // 180-360 are left turns
    if (angle > 0 && angle < 60)
        return DirectionModifier::SharpRight;
    if (angle >= 60 && angle < 140)
        return DirectionModifier::Right;
    if (angle >= 140 && angle < 170)
        return DirectionModifier::SlightRight;
    if (angle >= 170 && angle <= 190)
        return DirectionModifier::Straight;
    if (angle > 190 && angle <= 220)
        return DirectionModifier::SlightLeft;
    if (angle > 220 && angle <= 300)
        return DirectionModifier::Left;
    if (angle > 300 && angle < 360)
        return DirectionModifier::SharpLeft;
    return DirectionModifier::UTurn;
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

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TOOLKIT_HPP_
