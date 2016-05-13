#include "extractor/guidance/toolkit.hpp"
#include "extractor/guidance/turn_lane_matcher.hpp"
#include "util/guidance/toolkit.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

// Translate Turn Tags into a Matching Direction Modifier
DirectionModifier::Enum getMatchingModifier(const std::string &tag)
{
    const constexpr char *tag_by_modifier[] = {"reverse",
                                               "sharp_right",
                                               "right",
                                               "slight_right",
                                               "through",
                                               "slight_left",
                                               "left",
                                               "sharp_left",
                                               "merge_to_left",
                                               "merge_to_right"};
    const auto index =
        std::distance(tag_by_modifier, std::find(tag_by_modifier, tag_by_modifier + 10, tag));

    BOOST_ASSERT(index <= 10);

    const constexpr DirectionModifier::Enum modifiers[11] = {
        DirectionModifier::UTurn,
        DirectionModifier::SharpRight,
        DirectionModifier::Right,
        DirectionModifier::SlightRight,
        DirectionModifier::Straight,
        DirectionModifier::SlightLeft,
        DirectionModifier::Left,
        DirectionModifier::SharpLeft,
        DirectionModifier::Straight,
        DirectionModifier::Straight,
        DirectionModifier::UTurn}; // fallback for invalid tags

    return modifiers[index];
}

// check whether a match of a given tag and a turn instruction can be seen as valid
bool isValidMatch(const std::string &tag, const TurnInstruction instruction)
{
    using util::guidance::hasLeftModifier;
    using util::guidance::hasRightModifier;
    const auto isMirroredModifier = [](const TurnInstruction instruction) {
        return instruction.type == TurnType::Merge;
    };

    if (tag == "reverse")
    {
        return hasLeftModifier(instruction) ||
               instruction.direction_modifier == DirectionModifier::UTurn;
    }
    else if (tag == "sharp_right" || tag == "right" || tag == "slight_right")
    {
        if (isMirroredModifier(instruction))
            return hasLeftModifier(instruction);
        else
            // needs to be adjusted for left side driving
            return leavesRoundabout(instruction) || hasRightModifier(instruction);
    }
    else if (tag == "through")
    {
        return instruction.direction_modifier == DirectionModifier::Straight ||
               instruction.type == TurnType::Suppressed || instruction.type == TurnType::NewName ||
               instruction.type == TurnType::StayOnRoundabout || entersRoundabout(instruction) ||
               (instruction.type ==
                    TurnType::Fork && // Forks can be experienced, even for straight segments
                (instruction.direction_modifier == DirectionModifier::SlightLeft ||
                 instruction.direction_modifier == DirectionModifier::SlightRight)) ||
               (instruction.type ==
                    TurnType::Continue && // Forks can be experienced, even for straight segments
                (instruction.direction_modifier == DirectionModifier::SlightLeft ||
                 instruction.direction_modifier == DirectionModifier::SlightRight)) ||
               instruction.type == TurnType::UseLane;
    }
    else if (tag == "slight_left" || tag == "left" || tag == "sharp_left")
    {
        if (isMirroredModifier(instruction))
            return hasRightModifier(instruction);
        else
        {
            // Needs to be fixed for left side driving
            return (instruction.type == TurnType::StayOnRoundabout) || hasLeftModifier(instruction);
        }
    }
    return false;
}

typename Intersection::const_iterator findBestMatch(const std::string &tag,
                                                    const Intersection &intersection)
{
    const constexpr double idealized_turn_angles[] = {0, 35, 90, 135, 180, 225, 270, 315};
    const auto idealized_angle = idealized_turn_angles[getMatchingModifier(tag)];
    return std::min_element(
        intersection.begin(),
        intersection.end(),
        [idealized_angle, &tag](const ConnectedRoad &lhs, const ConnectedRoad &rhs) {
            // prefer valid matches
            if (isValidMatch(tag, lhs.turn.instruction) != isValidMatch(tag, rhs.turn.instruction))
                return isValidMatch(tag, lhs.turn.instruction);
            // if the entry allowed flags don't match, we select the one with
            // entry allowed set to true
            if (lhs.entry_allowed != rhs.entry_allowed)
                return lhs.entry_allowed;

            return angularDeviation(idealized_angle, lhs.turn.angle) <
                   angularDeviation(idealized_angle, rhs.turn.angle);
        });
}

typename Intersection::const_iterator findBestMatchForReverse(const std::string &leftmost_tag,
                                                              const Intersection &intersection)
{
    const auto leftmost_itr = findBestMatch(leftmost_tag, intersection);
    if (leftmost_itr + 1 == intersection.cend())
        return intersection.begin();

    const constexpr double idealized_turn_angles[] = {0, 35, 90, 135, 180, 225, 270, 315};
    const std::string tag = "reverse";
    const auto idealized_angle = idealized_turn_angles[getMatchingModifier(tag)];
    return std::min_element(
        intersection.begin() + std::distance(intersection.begin(), leftmost_itr),
        intersection.end(),
        [idealized_angle, &tag](const ConnectedRoad &lhs, const ConnectedRoad &rhs) {
            // prefer valid matches
            if (isValidMatch(tag, lhs.turn.instruction) != isValidMatch(tag, rhs.turn.instruction))
                return isValidMatch(tag, lhs.turn.instruction);
            // if the entry allowed flags don't match, we select the one with
            // entry allowed set to true
            if (lhs.entry_allowed != rhs.entry_allowed)
                return lhs.entry_allowed;

            return angularDeviation(idealized_angle, lhs.turn.angle) <
                   angularDeviation(idealized_angle, rhs.turn.angle);
        });
}

bool canMatchTrivially(const Intersection &intersection, const LaneDataVector &lane_data)
{
    std::size_t road_index = 1, lane = 0;
    for (; road_index < intersection.size() && lane < lane_data.size(); ++road_index)
    {
        if (intersection[road_index].entry_allowed)
        {
            BOOST_ASSERT(lane_data[lane].from != INVALID_LANEID);
            if (!isValidMatch(lane_data[lane].tag, intersection[road_index].turn.instruction))
                return false;

            if (findBestMatch(lane_data[lane].tag, intersection) !=
                intersection.begin() + road_index)
                return false;
            ++lane;
        }
    }
    return lane == lane_data.size() ||
           (lane + 1 == lane_data.size() && lane_data.back().tag == "reverse");
}

Intersection triviallyMatchLanesToTurns(Intersection intersection,
                                        const LaneDataVector &lane_data,
                                        const util::NodeBasedDynamicGraph &node_based_graph)
{
    std::size_t road_index = 1, lane = 0;
    for (; road_index < intersection.size() && lane < lane_data.size(); ++road_index)
    {
        if (intersection[road_index].entry_allowed)
        {
            BOOST_ASSERT(lane_data[lane].from != INVALID_LANEID);
            BOOST_ASSERT(
                isValidMatch(lane_data[lane].tag, intersection[road_index].turn.instruction));
            BOOST_ASSERT(findBestMatch(lane_data[lane].tag, intersection) ==
                         intersection.begin() + road_index);

            if (TurnType::Suppressed == intersection[road_index].turn.instruction.type)
                intersection[road_index].turn.instruction.type = TurnType::UseLane;

            intersection[road_index].turn.instruction.lane_tupel = {
                LaneID(lane_data[lane].to - lane_data[lane].from + 1), lane_data[lane].from};
            ++lane;
        }
    }

    // handle reverse tag, if present
    if (lane + 1 == lane_data.size() && lane_data.back().tag == "reverse")
    {
        std::size_t u_turn = 0;
        if (node_based_graph.GetEdgeData(intersection[0].turn.eid).reversed)
        {
            if (intersection.back().entry_allowed ||
                intersection.back().turn.instruction.direction_modifier !=
                    DirectionModifier::SharpLeft)
            {
                // cannot match u-turn in a valid way
                return std::move(intersection);
            }
            u_turn = intersection.size() - 1;
        }
        intersection[u_turn].entry_allowed = true;
        intersection[u_turn].turn.instruction.type = TurnType::Turn;
        intersection[u_turn].turn.instruction.direction_modifier = DirectionModifier::UTurn;
        intersection[u_turn].turn.instruction.lane_tupel = {
            LaneID(lane_data.back().to - lane_data.back().from + 1), lane_data.back().from};
    }
    return std::move(intersection);
}

} // namespace lane_matching
} // namespace guidance
} // namespace extractor
} // namespace osrm
