#include "extractor/guidance/turn_lane_augmentation.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "util/simple_logger.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <cstddef>
#include <utility>

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

namespace
{

const constexpr TurnLaneType::Mask tag_by_modifier[] = {TurnLaneType::uturn,
                                                        TurnLaneType::sharp_right,
                                                        TurnLaneType::right,
                                                        TurnLaneType::slight_right,
                                                        TurnLaneType::straight,
                                                        TurnLaneType::slight_left,
                                                        TurnLaneType::left,
                                                        TurnLaneType::sharp_left};

std::size_t getNumberOfTurns(const Intersection &intersection)
{
    return std::count_if(intersection.begin(), intersection.end(), [](const ConnectedRoad &road) {
        return road.entry_allowed;
    });
}

LaneDataVector augmentMultiple(const std::size_t none_index,
                               const std::size_t connection_count,
                               LaneDataVector lane_data,
                               const Intersection &intersection)
{

    // a none-turn is allowing multiple turns. we have to add a lane-data entry for
    // every possible turn. This should, hopefully, only be the case for single lane
    // entries?

    // looking at the left side first
    const auto range = [&]() {
        if (none_index == 0)
        {
            // find first connection_count - lane_data.size() valid turns
            std::size_t count = 0;
            for (std::size_t intersection_index = 1; intersection_index < intersection.size();
                 ++intersection_index)
            {

                count += static_cast<int>(intersection[intersection_index].entry_allowed);
                if (count > connection_count - lane_data.size())
                    return std::make_pair(std::size_t{1}, intersection_index + 1);
            }
        }
        else if (none_index + 1 == lane_data.size())
        {
            BOOST_ASSERT(!lane_data.empty());
            // find last connection-count - last_data.size() valid turns
            std::size_t count = 0;
            for (std::size_t intersection_index = intersection.size() - 1; intersection_index > 0;
                 --intersection_index)
            {
                count += static_cast<int>(intersection[intersection_index].entry_allowed);
                if (count > connection_count - lane_data.size())
                    return std::make_pair(intersection_index, intersection.size());
            }
        }
        else
        {
            // skip the first #index valid turns, find next connection_count -
            // lane_data.size() valid ones

            std::size_t begin = 1, count = 0, intersection_index;
            for (intersection_index = 1; intersection_index < intersection.size();
                 ++intersection_index)
            {
                count += static_cast<int>(intersection[intersection_index].entry_allowed);
                // if we reach the amount of
                if (count >= none_index)
                {
                    begin = intersection_index + 1;
                    break;
                }
            }

            // reset count to find the number of necessary entries
            count = 0;
            for (intersection_index = begin; intersection_index < intersection.size();
                 ++intersection_index)
            {
                count += static_cast<int>(intersection[intersection_index].entry_allowed);
                if (count > connection_count - lane_data.size())
                {
                    return std::make_pair(begin, intersection_index + 1);
                }
            }
        }
        // this should, theoretically, never be reached
        util::SimpleLogger().Write(logWARNING) << "Failed lane assignment. Reached bad situation.";
        return std::make_pair(std::size_t{0}, std::size_t{0});
    }();
    for (auto intersection_index = range.first; intersection_index < range.second;
         ++intersection_index)
    {
        if (intersection[intersection_index].entry_allowed)
        {
            // FIXME this probably can be only a subset of these turns here?
            lane_data.push_back({tag_by_modifier[intersection[intersection_index]
                                                     .turn.instruction.direction_modifier],
                                 lane_data[none_index].from,
                                 lane_data[none_index].to});
        }
    }
    lane_data.erase(lane_data.begin() + none_index);
    return lane_data;
}

// Merging none-tag into its neighboring fields
// This handles situations like "left | | | right".
LaneDataVector mergeNoneTag(const std::size_t none_index, LaneDataVector lane_data)
{
    if (none_index == 0 || none_index + 1 == lane_data.size())
    {
        if (none_index == 0)
        {
            lane_data[1].from = lane_data[0].from;
        }
        else
        {
            lane_data[none_index - 1].to = lane_data[none_index].to;
        }
        lane_data.erase(lane_data.begin() + none_index);
    }
    else if (lane_data[none_index].to - lane_data[none_index].from <= 1)
    {
        lane_data[none_index - 1].to = lane_data[none_index].from;
        lane_data[none_index + 1].from = lane_data[none_index].to;

        lane_data.erase(lane_data.begin() + none_index);
    }
    return lane_data;
}

LaneDataVector handleRenamingSituations(const std::size_t none_index,
                                        LaneDataVector lane_data,
                                        const Intersection &intersection)
{
    bool has_right = false;
    bool has_through = false;
    bool has_left = false;
    for (const auto &road : intersection)
    {
        if (!road.entry_allowed)
            continue;

        const auto modifier = road.turn.instruction.direction_modifier;
        has_right |= modifier == DirectionModifier::Right;
        has_right |= modifier == DirectionModifier::SlightRight;
        has_right |= modifier == DirectionModifier::SharpRight;
        has_through |= modifier == DirectionModifier::Straight;
        has_left |= modifier == DirectionModifier::Left;
        has_left |= modifier == DirectionModifier::SlightLeft;
        has_left |= modifier == DirectionModifier::SharpLeft;
    }

    // find missing tag and augment neighboring, if possible
    if (none_index == 0)
    {
        if (has_right &&
            (lane_data.size() == 1 || (lane_data[none_index + 1].tag != TurnLaneType::sharp_right &&
                                       lane_data[none_index + 1].tag != TurnLaneType::right)))
        {
            lane_data[none_index].tag = TurnLaneType::right;
            if (lane_data.size() > 1 && lane_data[none_index + 1].tag == TurnLaneType::straight)
            {
                lane_data[none_index + 1].from = lane_data[none_index].from;
                // turning right through a possible through lane is not possible
                lane_data[none_index].to = lane_data[none_index].from;
            }
        }
        else if (has_through &&
                 (lane_data.size() == 1 || lane_data[none_index + 1].tag != TurnLaneType::straight))
        {
            lane_data[none_index].tag = TurnLaneType::straight;
        }
    }
    else if (none_index + 1 == lane_data.size())
    {
        if (has_left && ((lane_data[none_index - 1].tag != TurnLaneType::sharp_left &&
                          lane_data[none_index - 1].tag != TurnLaneType::left)))
        {
            lane_data[none_index].tag = TurnLaneType::left;
            if (lane_data[none_index - 1].tag == TurnLaneType::straight)
            {
                lane_data[none_index - 1].to = lane_data[none_index].to;
                // turning left through a possible through lane is not possible
                lane_data[none_index].from = lane_data[none_index].to;
            }
        }
        else if (has_through && lane_data[none_index - 1].tag != TurnLaneType::straight)
        {
            lane_data[none_index].tag = TurnLaneType::straight;
        }
    }
    else
    {
        if ((lane_data[none_index + 1].tag == TurnLaneType::left ||
             lane_data[none_index + 1].tag == TurnLaneType::slight_left ||
             lane_data[none_index + 1].tag == TurnLaneType::sharp_left) &&
            (lane_data[none_index - 1].tag == TurnLaneType::right ||
             lane_data[none_index - 1].tag == TurnLaneType::slight_right ||
             lane_data[none_index - 1].tag == TurnLaneType::sharp_right))
        {
            lane_data[none_index].tag = TurnLaneType::straight;
        }
    }
    return lane_data;
}

} // namespace

/*
   Lanes can have the tag none. While a nice feature for visibility, it is a terrible feature
   for parsing. None can be part of neighboring turns, or not. We have to look at both the
   intersection and the lane data to see what turns we have to augment by the none-lanes
 */
LaneDataVector handleNoneValueAtSimpleTurn(LaneDataVector lane_data,
                                           const Intersection &intersection)
{
    const bool needs_no_processing =
        (intersection.empty() || lane_data.empty() || !hasTag(TurnLaneType::none, lane_data));

    if (needs_no_processing)
        return lane_data;

    // FIXME all this needs to consider the number of lanes at the target to ensure that we
    // augment lanes correctly, if the target lane allows for more turns
    //
    // -----------------
    //
    // -----        ----
    //  -v          |
    // -----        |
    //      |   |   |
    //
    // A situation like this would allow a right turn from the through lane.
    //
    // -----------------
    //
    // -----    --------
    //  -v      |
    // -----    |
    //      |   |
    //
    // Here, the number of lanes in the right road would not allow turns from both lanes, but
    // only from the right one.

    const std::size_t connection_count =
        getNumberOfTurns(intersection) -
        ((intersection[0].entry_allowed && lane_data.back().tag != TurnLaneType::uturn) ? 1 : 0);

    // TODO check for impossible turns to see whether the turn lane is at the correct place
    const std::size_t none_index = std::distance(lane_data.begin(), findTag(TurnLaneType::none, lane_data));
    BOOST_ASSERT(none_index != lane_data.size());
    // we have to create multiple turns
    if (connection_count > lane_data.size())
    {
        lane_data =
            augmentMultiple(none_index, connection_count, std::move(lane_data), intersection);
    }
    // we have to reduce it, assigning it to neighboring turns
    else if (connection_count < lane_data.size())
    {
        // a pgerequisite is simple turns. Larger differences should not end up here
        // an additional line at the side is only reasonable if it is targeting public
        // service vehicles. Otherwise, we should not have it
        //
        // TODO(mokob): #2730 have a look please
        // BOOST_ASSERT(connection_count + 1 == lane_data.size());
        //
        if (connection_count + 1 != lane_data.size())
        {
            goto these_intersections_are_clearly_broken_at_the_moment;
        }

        lane_data = mergeNoneTag(none_index, std::move(lane_data));
    }
    // we have to rename and possibly augment existing ones. The pure count remains the
    // same.
    else
    {
        lane_data = handleRenamingSituations(none_index, std::move(lane_data), intersection);
    }

these_intersections_are_clearly_broken_at_the_moment:

    // finally make sure we are still sorted
    std::sort(lane_data.begin(), lane_data.end());
    return lane_data;
}

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm
