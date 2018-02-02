#include "guidance/turn_lane_matcher.hpp"
#include "util/bearing.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <functional>

using osrm::util::angularDeviation;

namespace osrm
{
namespace guidance
{
namespace lanes
{
namespace TurnLaneType = extractor::TurnLaneType;

// Translate Turn Tags into a Matching Direction Modifier
DirectionModifier::Enum getMatchingModifier(const TurnLaneType::Mask tag)
{
    const constexpr TurnLaneType::Mask tag_by_modifier[] = {TurnLaneType::uturn,
                                                            TurnLaneType::sharp_right,
                                                            TurnLaneType::right,
                                                            TurnLaneType::slight_right,
                                                            TurnLaneType::straight,
                                                            TurnLaneType::slight_left,
                                                            TurnLaneType::left,
                                                            TurnLaneType::sharp_left,
                                                            TurnLaneType::merge_to_left,
                                                            TurnLaneType::merge_to_right};
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
bool isValidMatch(const TurnLaneType::Mask tag, const TurnInstruction instruction)
{
    const auto isMirroredModifier = [](const TurnInstruction instruction) {
        return instruction.type == TurnType::Merge;
    };

    if (tag == TurnLaneType::uturn)
    {
        return hasLeftModifier(instruction) ||
               instruction.direction_modifier == DirectionModifier::UTurn;
    }
    else if (tag == TurnLaneType::sharp_right || tag == TurnLaneType::right ||
             tag == TurnLaneType::slight_right)
    {
        if (isMirroredModifier(instruction))
            return hasLeftModifier(instruction);
        else
            // needs to be adjusted for left side driving
            return leavesRoundabout(instruction) || hasRightModifier(instruction);
    }
    else if (tag == TurnLaneType::straight)
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
                 instruction.direction_modifier == DirectionModifier::SlightRight));
    }
    else if (tag == TurnLaneType::slight_left || tag == TurnLaneType::left ||
             tag == TurnLaneType::sharp_left)
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

double getMatchingQuality(const TurnLaneType::Mask tag, const ConnectedRoad &road)
{
    const constexpr double idealized_turn_angles[] = {0, 35, 90, 135, 180, 225, 270, 315};
    const auto modifier = getMatchingModifier(tag);
    BOOST_ASSERT(static_cast<std::size_t>(modifier) <
                 sizeof(idealized_turn_angles) / sizeof(*idealized_turn_angles));
    const auto idealized_angle = idealized_turn_angles[modifier];
    return angularDeviation(idealized_angle, road.angle);
}

// Every tag is somewhat idealized in form of the expected angle. A through lane should go straight
// (or follow a 180 degree turn angle between in/out segments.) The following function tries to find
// the best possible match for every tag in a given intersection, considering a few corner cases
// introduced to OSRM handling u-turns
typename Intersection::const_iterator findBestMatch(const TurnLaneType::Mask tag,
                                                    const Intersection &intersection)
{
    return std::min_element(intersection.begin(),
                            intersection.end(),
                            [tag](const ConnectedRoad &lhs, const ConnectedRoad &rhs) {
                                // prefer valid matches
                                if (isValidMatch(tag, lhs.instruction) !=
                                    isValidMatch(tag, rhs.instruction))
                                    return isValidMatch(tag, lhs.instruction);

                                // if the entry allowed flags don't match, we select the one with
                                // entry allowed set to true
                                if (lhs.entry_allowed != rhs.entry_allowed)
                                    return lhs.entry_allowed;

                                return getMatchingQuality(tag, lhs) < getMatchingQuality(tag, rhs);
                            });
}

// Reverse is a special case, because it requires access to the leftmost tag. It has its own
// matching function as a result of that. The leftmost tag is required, since u-turns are disabled
// by default in OSRM. Therefor we cannot check whether a turn is allowed, since it could be
// possible that it is forbidden. In addition, the best u-turn angle does not necessarily represent
// the u-turn, since it could be a sharp-left turn instead on a road with a middle island.
typename Intersection::const_iterator findBestMatchForReverse(const TurnLaneType::Mask neighbor_tag,
                                                              const Intersection &intersection)
{
    const auto neighbor_itr = findBestMatch(neighbor_tag, intersection);
    if (neighbor_itr + 1 == intersection.cend())
        return intersection.begin();

    return std::min_element(
        intersection.begin() + std::distance(intersection.begin(), neighbor_itr),
        intersection.end(),
        [](const ConnectedRoad &lhs, const ConnectedRoad &rhs) {
            const TurnLaneType::Mask tag = TurnLaneType::uturn;
            // prefer valid matches
            if (isValidMatch(tag, lhs.instruction) != isValidMatch(tag, rhs.instruction))
                return isValidMatch(tag, lhs.instruction);

            // if the entry allowed flags don't match, we select the one with
            // entry allowed set to true
            if (lhs.entry_allowed != rhs.entry_allowed)
                return lhs.entry_allowed;

            return getMatchingQuality(tag, lhs) < getMatchingQuality(tag, rhs);
        });
}

// a match is trivial if all turns can be associated with their best match in a valid way and the
// matches occur in order
bool canMatchTrivially(const Intersection &intersection, const LaneDataVector &lane_data)
{
    std::size_t road_index = 1, lane = 0;
    if (!lane_data.empty() && lane_data.front().tag == TurnLaneType::uturn)
    {
        // the very first is a u-turn to the right
        if (intersection[0].entry_allowed)
            lane = 1;
    }
    for (; road_index < intersection.size() && lane < lane_data.size(); ++road_index)
    {
        if (intersection[road_index].entry_allowed)
        {
            BOOST_ASSERT(lane_data[lane].from != INVALID_LANEID);
            if (!isValidMatch(lane_data[lane].tag, intersection[road_index].instruction))
                return false;

            if (findBestMatch(lane_data[lane].tag, intersection) !=
                intersection.begin() + road_index)
                return false;

            ++lane;
        }
    }
    return lane == lane_data.size() ||
           (lane + 1 == lane_data.size() && lane_data.back().tag == TurnLaneType::uturn);
}

Intersection triviallyMatchLanesToTurns(Intersection intersection,
                                        const LaneDataVector &lane_data,
                                        const util::NodeBasedDynamicGraph &node_based_graph,
                                        const LaneDescriptionID lane_string_id,
                                        util::guidance::LaneDataIdMap &lane_data_to_id)
{
    std::size_t road_index = 1, lane = 0;

    const auto matchRoad = [&](ConnectedRoad &road, const TurnLaneData &data) {
        util::guidance::LaneTupleIdPair key{{LaneID(data.to - data.from + 1), data.from},
                                            lane_string_id};

        road.lane_data_id = lane_data_to_id.ConcurrentFindOrAdd(key);
    };

    if (!lane_data.empty() && lane_data.front().tag == TurnLaneType::uturn)
    {
        // the very first is a u-turn to the right
        if (intersection[0].entry_allowed)
        {
            std::size_t u_turn = 0;
            if (node_based_graph.GetEdgeData(intersection[0].eid).reversed)
            {
                if (intersection.size() <= 1 || !intersection[1].entry_allowed ||
                    intersection[1].instruction.direction_modifier != DirectionModifier::SharpRight)
                {
                    // cannot match u-turn in a valid way
                    return intersection;
                }
                u_turn = 1;
                road_index = 2;
            }
            intersection[u_turn].instruction.type = TurnType::Continue;
            intersection[u_turn].instruction.direction_modifier = DirectionModifier::UTurn;

            matchRoad(intersection[u_turn], lane_data.back());
            // continue with the first lane
            lane = 1;
        }
        else
            return intersection;
    }

    for (; road_index < intersection.size() && lane < lane_data.size(); ++road_index)
    {
        if (intersection[road_index].entry_allowed)
        {
            BOOST_ASSERT(lane_data[lane].from != INVALID_LANEID);
            BOOST_ASSERT(isValidMatch(lane_data[lane].tag, intersection[road_index].instruction));
            BOOST_ASSERT(findBestMatch(lane_data[lane].tag, intersection) ==
                         intersection.begin() + road_index);

            matchRoad(intersection[road_index], lane_data[lane]);
            ++lane;
        }
    }

    // handle reverse tag, if present
    if (lane + 1 == lane_data.size() && lane_data.back().tag == TurnLaneType::uturn)
    {
        std::size_t u_turn = 0;
        if (node_based_graph.GetEdgeData(intersection[0].eid).reversed)
        {
            if (!intersection.back().entry_allowed ||
                intersection.back().instruction.direction_modifier != DirectionModifier::SharpLeft)
            {
                // cannot match u-turn in a valid way
                return intersection;
            }
            u_turn = intersection.size() - 1;
        }
        intersection[u_turn].instruction.type = TurnType::Continue;
        intersection[u_turn].instruction.direction_modifier = DirectionModifier::UTurn;

        matchRoad(intersection[u_turn], lane_data.back());
    }
    return intersection;
}

} // namespace lane_matching
} // namespace guidance
} // namespace osrm
