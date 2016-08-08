#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/turn_discovery.hpp"
#include "extractor/guidance/turn_lane_augmentation.hpp"
#include "extractor/guidance/turn_lane_handler.hpp"
#include "extractor/guidance/turn_lane_matcher.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"

#include <cstdint>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/numeric/conversion/cast.hpp>

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
std::size_t getNumberOfTurns(const Intersection &intersection)
{
    return std::count_if(intersection.begin(), intersection.end(), [](const ConnectedRoad &road) {
        return road.entry_allowed;
    });
}
} // namespace

TurnLaneHandler::TurnLaneHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                 const std::vector<std::uint32_t> &turn_lane_offsets,
                                 const std::vector<TurnLaneType::Mask> &turn_lane_masks,
                                 const std::vector<QueryNode> &node_info_list,
                                 const TurnAnalysis &turn_analysis)
    : node_based_graph(node_based_graph), turn_lane_offsets(turn_lane_offsets),
      turn_lane_masks(turn_lane_masks), node_info_list(node_info_list), turn_analysis(turn_analysis)
{
}

/*
   Turn lanes are given in the form of strings that closely correspond to the direction modifiers
   we use for our turn types. However, we still cannot simply perform a 1:1 assignment.

   This function parses the turn_lane_descriptions of a format that describes an intersection as:

    ----------
    A -^
    ----------
    B -> -v
    ----------
    C -v
    ----------

    witch is the result of a string like looking |left|through;right|right| and performs an
    assignment onto the turns.
    For example: (130, turn slight right), (180, ramp straight), (320, turn sharp left).
 */
Intersection TurnLaneHandler::assignTurnLanes(const NodeID at,
                                              const EdgeID via_edge,
                                              Intersection intersection,
                                              LaneDataIdMap &id_map) const
{
    //if only a uturn exists, there is nothing we can do
    if( intersection.size() == 1 )
        return intersection;

    const auto &data = node_based_graph.GetEdgeData(via_edge);
    // Extract a lane description for the ID

    const auto turn_lane_description =
        data.lane_description_id != INVALID_LANE_DESCRIPTIONID
            ? TurnLaneDescription(
                  turn_lane_masks.begin() + turn_lane_offsets[data.lane_description_id],
                  turn_lane_masks.begin() + turn_lane_offsets[data.lane_description_id + 1])
            : TurnLaneDescription();

    BOOST_ASSERT( turn_lane_description.empty() || turn_lane_description.size() == (turn_lane_offsets[data.lane_description_id+1] - turn_lane_offsets[data.lane_description_id]));

    // going straight, due to traffic signals, we can have uncompressed geometry
    if (intersection.size() == 2 &&
        ((data.lane_description_id != INVALID_LANE_DESCRIPTIONID &&
          data.lane_description_id ==
              node_based_graph.GetEdgeData(intersection[1].turn.eid).lane_description_id) ||
         angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE))
        return intersection;

    auto lane_data = laneDataFromDescription(turn_lane_description);

    // if we see an invalid conversion, we stop immediately
    if (!turn_lane_description.empty() && lane_data.empty())
        return intersection;

    // might be reasonable to handle multiple turns, if we know of a sequence of lanes
    // e.g. one direction per lane, if three lanes and right, through, left available
    if (!turn_lane_description.empty() && lane_data.size() == 1 &&
        lane_data[0].tag == TurnLaneType::none)
        return intersection;

    const std::size_t possible_entries = getNumberOfTurns(intersection);

    // merge does not justify an instruction
    const bool has_merge_lane =
        hasTag(TurnLaneType::merge_to_left | TurnLaneType::merge_to_right, lane_data);

    // Dead end streets that don't have any left-tag. This can happen due to the fallbacks for
    // broken data/barriers.
    const bool has_non_usable_u_turn = (intersection[0].entry_allowed &&
                                        !hasTag(TurnLaneType::none | TurnLaneType::left |
                                                    TurnLaneType::sharp_left | TurnLaneType::uturn,
                                                lane_data) &&
                                        lane_data.size() + 1 == possible_entries);

    if (has_merge_lane || has_non_usable_u_turn)
        return intersection;

    if (!lane_data.empty() && canMatchTrivially(intersection, lane_data) &&
        lane_data.size() !=
            static_cast<std::size_t>((
                !hasTag(TurnLaneType::uturn, lane_data) && intersection[0].entry_allowed ? 1 : 0)) +
                possible_entries &&
        intersection[0].entry_allowed && !hasTag(TurnLaneType::none, lane_data))
        lane_data.push_back({TurnLaneType::uturn, lane_data.back().to, lane_data.back().to});

    bool is_simple = isSimpleIntersection(lane_data, intersection);
    // simple intersections can be assigned directly
    if (is_simple)
    {
        lane_data = handleNoneValueAtSimpleTurn(std::move(lane_data), intersection);
        return simpleMatchTuplesToTurns(
            std::move(intersection), lane_data, data.lane_description_id, id_map);
    }
    // if the intersection is not simple but we have lane data, we check for intersections with
    // middle islands. We have two cases. The first one is providing lane data on the current
    // segment and we only need to consider the part of the current segment. In this case we
    // partition the data and only consider the first part.
    else if (!lane_data.empty())
    {
        if (lane_data.size() >= possible_entries)
        {
            lane_data = partitionLaneData(node_based_graph.GetTarget(via_edge),
                                          std::move(lane_data),
                                          intersection)
                            .first;

            // check if we were successfull in trimming
            if (lane_data.size() == possible_entries &&
                isSimpleIntersection(lane_data, intersection))
            {
                lane_data = handleNoneValueAtSimpleTurn(std::move(lane_data), intersection);
                return simpleMatchTuplesToTurns(
                    std::move(intersection), lane_data, data.lane_description_id, id_map);
            }
        }
    }
    // The second part does not provide lane data on the current segment, but on the segment prior
    // to the turn. We try to partition the data and only consider the second part.
    else if (turn_lane_description.empty())
    {
        // acquire the lane data of a previous segment and, if possible, use it for the current
        // intersection.
        return handleTurnAtPreviousIntersection(at, via_edge, std::move(intersection), id_map);
    }

    return intersection;
}

// At segregated intersections, turn lanes will often only be specified up until the first turn. To
// actually take the turn, we need to look back to the edge we drove onto the intersection with.
Intersection TurnLaneHandler::handleTurnAtPreviousIntersection(const NodeID at,
                                                               const EdgeID via_edge,
                                                               Intersection intersection,
                                                               LaneDataIdMap &id_map) const
{
    NodeID previous_node = SPECIAL_NODEID;
    Intersection previous_intersection;
    EdgeID previous_id = SPECIAL_EDGEID;
    LaneDataVector lane_data;

    // Get the previous lane string. We only accept strings that stem from a not-simple intersection
    // and are not empty.
    const auto previous_lane_description = [&]() -> TurnLaneDescription {
        if (!findPreviousIntersection(at,
                                      via_edge,
                                      intersection,
                                      turn_analysis,
                                      node_based_graph,
                                      previous_node,
                                      previous_id,
                                      previous_intersection))
            return {};

        BOOST_ASSERT(previous_id != SPECIAL_EDGEID);

        const auto &previous_edge_data = node_based_graph.GetEdgeData(previous_id);
        // TODO access correct data
        const auto previous_description =
            previous_edge_data.lane_description_id != INVALID_LANE_DESCRIPTIONID
                ? TurnLaneDescription(
                      turn_lane_masks.begin() +
                          turn_lane_offsets[previous_edge_data.lane_description_id],
                      turn_lane_masks.begin() +
                          turn_lane_offsets[previous_edge_data.lane_description_id + 1])
                : TurnLaneDescription();
        if (previous_description.empty())
            return previous_description;

        previous_intersection = turn_analysis.assignTurnTypes(
            previous_node, previous_id, std::move(previous_intersection));

        lane_data = laneDataFromDescription(previous_description);

        if (isSimpleIntersection(lane_data, previous_intersection))
            return {};
        else
            return previous_description;
    }();

    // no lane string, no problems
    if (previous_lane_description.empty())
        return intersection;

    // stop on invalid lane data conversion
    if (lane_data.empty())
        return intersection;

    const auto &previous_data = node_based_graph.GetEdgeData(previous_id);
    const auto is_simple = isSimpleIntersection(lane_data, intersection);
    if (is_simple)
    {
        lane_data = handleNoneValueAtSimpleTurn(std::move(lane_data), intersection);
        return simpleMatchTuplesToTurns(
            std::move(intersection), lane_data, previous_data.lane_description_id, id_map);
    }
    else
    {
        if (lane_data.size() >= getNumberOfTurns(previous_intersection) &&
            previous_intersection.size() != 2)
        {
            lane_data = partitionLaneData(node_based_graph.GetTarget(previous_id),
                                          std::move(lane_data),
                                          previous_intersection)
                            .second;

            std::sort(lane_data.begin(), lane_data.end());

            // check if we were successfull in trimming
            if (lane_data.size() == getNumberOfTurns(intersection) &&
                isSimpleIntersection(lane_data, intersection))
            {
                lane_data = handleNoneValueAtSimpleTurn(std::move(lane_data), intersection);
                return simpleMatchTuplesToTurns(
                    std::move(intersection), lane_data, previous_data.lane_description_id, id_map);
            }
        }
    }
    return intersection;
}

/* A simple intersection does not depend on the next intersection coming up. This is important
 * for turn lanes, since traffic signals and/or segregated a intersection can influence the
 * interpretation of turn-lanes at a given turn.
 *
 * Here we check for a simple intersection. A simple intersection has a long enough segment
 * followin the turn, offers no straight turn, or only non-trivial turn operations.
 */
bool TurnLaneHandler::isSimpleIntersection(const LaneDataVector &lane_data,
                                           const Intersection &intersection) const
{
    if (lane_data.empty())
        return false;
    // if we are on a straight road, turn lanes are only reasonable in connection to the next
    // intersection, or in case of a merge. If not all but one (straight) are merges, we don't
    // consider the intersection simple
    if (intersection.size() == 2)
    {
        return std::count_if(
                   lane_data.begin(),
                   lane_data.end(),
                   [](const TurnLaneData &data) {
                       return ((data.tag & TurnLaneType::merge_to_left) != TurnLaneType::empty) ||
                              ((data.tag & TurnLaneType::merge_to_right) != TurnLaneType::empty);
                   }) +
                   std::size_t{1} >=
               lane_data.size();
    }

    // in case an intersection offers far more lane data items than actual turns, some of them
    // have
    // to be for another intersection. A single additional item can be for an invalid bus lane.
    const auto num_turns = [&]() {
        auto count = getNumberOfTurns(intersection);
        if (count < lane_data.size() && !intersection[0].entry_allowed &&
            lane_data.back().tag == TurnLaneType::uturn)
            return count + 1;
        return count;
    }();

    // more than two additional lane data entries -> lanes target a different intersection
    if (num_turns + std::size_t{2} <= lane_data.size())
    {
        return false;
    }

    // single additional lane data entry is alright, if it is none at the side. This usually
    // refers to a bus-lane
    if (num_turns + std::size_t{1} == lane_data.size() &&
        lane_data.front().tag != TurnLaneType::none && lane_data.back().tag != TurnLaneType::none)
    {
        return false;
    }

    // more turns than lane data
    if (num_turns > lane_data.size() &&
        lane_data.end() ==
            std::find_if(lane_data.begin(), lane_data.end(), [](const TurnLaneData &data) {
                return data.tag == TurnLaneType::none;
            }))
    {
        return false;
    }

    if (num_turns > lane_data.size() && intersection[0].entry_allowed &&
        !(hasTag(TurnLaneType::uturn, lane_data) ||
          (lane_data.back().tag != TurnLaneType::left &&
           lane_data.back().tag != TurnLaneType::sharp_left)))
    {
        return false;
    }

    // check if we can find a valid 1:1 mapping in a straightforward manner
    bool all_simple = true;
    bool has_none = false;
    std::unordered_set<std::size_t> matched_indices;
    for (const auto &data : lane_data)
    {
        if (data.tag == TurnLaneType::none)
        {
            has_none = true;
            continue;
        }

        const auto best_match = [&]() {
            if (data.tag != TurnLaneType::uturn || lane_data.size() == 1)
                return findBestMatch(data.tag, intersection);

            // lane_data.size() > 1
            if (lane_data.back().tag == TurnLaneType::uturn)
                return findBestMatchForReverse(lane_data[lane_data.size() - 2].tag, intersection);

            BOOST_ASSERT(lane_data.front().tag == TurnLaneType::uturn);
            return findBestMatchForReverse(lane_data[1].tag, intersection);
        }();
        std::size_t match_index = std::distance(intersection.begin(), best_match);
        all_simple &= (matched_indices.count(match_index) == 0);
        matched_indices.insert(match_index);
        // in case of u-turns, we might need to activate them first
        all_simple &= (best_match->entry_allowed || match_index == 0);
        all_simple &= isValidMatch(data.tag, best_match->turn.instruction);
    }

    // either all indices are matched, or we have a single none-value
    if (all_simple && (matched_indices.size() == lane_data.size() ||
                       (matched_indices.size() + 1 == lane_data.size() && has_none)))
        return true;

    // better save than sorry
    return false;
}

std::pair<LaneDataVector, LaneDataVector> TurnLaneHandler::partitionLaneData(
    const NodeID at, LaneDataVector turn_lane_data, const Intersection &intersection) const
{
    BOOST_ASSERT(turn_lane_data.size() >= getNumberOfTurns(intersection));
    /*
     * A Segregated intersection can provide turn lanes for turns that are not yet possible.
     * The straightforward example would be coming up to the following situation:
     *         (1)             (2)
     *        | A |           | A |
     *        | | |           | ^ |
     *        | v |           | | |
     * -------     -----------     ------
     *  B ->-^                        B
     * -------     -----------     ------
     *  B ->-v                        B
     * -------     -----------     ------
     *        | A |           | A |
     *
     * Traveling on road B, we have to pass A at (1) to turn left onto A at (2). The turn
     * lane itself may only be specified prior to (1) and/or could be repeated between (1)
     * and (2). To make sure to announce the lane correctly, we need to treat the (in this
     * case left) turn lane as if it were to continue straight onto the intersection and
     * look back between (1) and (2) to make sure we find the correct lane for the left-turn.
     *
     * Intersections like these have two parts. Turns that can be made at the first intersection and
     * turns that have to be made at the second. The partitioning returns the lane data split into
     * two parts, one for the first and one for the second intersection.
     */

    // Try and maitch lanes to available turns. For Turns that are not directly matchable, check
    // whether we can match them at the upcoming intersection.

    const auto straightmost = findClosestTurn(intersection, STRAIGHT_ANGLE);

    BOOST_ASSERT(straightmost < intersection.cend());

    // we need to be able to enter the straightmost turn
    if (!straightmost->entry_allowed)
        return {turn_lane_data, {}};

    std::vector<bool> matched_at_first(turn_lane_data.size(), false);
    std::vector<bool> matched_at_second(turn_lane_data.size(), false);

    // find out about the next intersection. To check for valid matches, we also need the turn types
    auto next_intersection = turn_analysis.getIntersection(at, straightmost->turn.eid);
    next_intersection =
        turn_analysis.assignTurnTypes(at, straightmost->turn.eid, std::move(next_intersection));

    // check where we can match turn lanes
    std::size_t straightmost_tag_index = turn_lane_data.size();
    for (std::size_t lane = 0; lane < turn_lane_data.size(); ++lane)
    {
        if ((turn_lane_data[lane].tag & (TurnLaneType::none | TurnLaneType::uturn)) !=
            TurnLaneType::empty)
            continue;

        const auto best_match = findBestMatch(turn_lane_data[lane].tag, intersection);
        if (isValidMatch(turn_lane_data[lane].tag, best_match->turn.instruction))
        {
            matched_at_first[lane] = true;

            if (straightmost == best_match)
                straightmost_tag_index = lane;
        }

        const auto best_match_at_next_intersection =
            findBestMatch(turn_lane_data[lane].tag, next_intersection);
        if (isValidMatch(turn_lane_data[lane].tag,
                         best_match_at_next_intersection->turn.instruction))
            matched_at_second[lane] = true;

        // we need to match all items to either the current or the next intersection
        if (!(matched_at_first[lane] || matched_at_second[lane]))
            return {turn_lane_data, {}};
    }

    std::size_t none_index =
        std::distance(turn_lane_data.begin(), findTag(TurnLaneType::none, turn_lane_data));

    // if the turn lanes are pull forward, we might have to add an additional straight tag
    // did we find something that matches against the straightmost road?
    if (straightmost_tag_index == turn_lane_data.size())
    {
        if (none_index != turn_lane_data.size())
            straightmost_tag_index = none_index;
    }

    // TODO handle reverse

    // handle none values
    if (none_index != turn_lane_data.size())
    {
        if (static_cast<std::size_t>(
                std::count(matched_at_first.begin(), matched_at_first.end(), true)) <=
            getNumberOfTurns(intersection))
            matched_at_first[none_index] = true;

        if (static_cast<std::size_t>(
                std::count(matched_at_second.begin(), matched_at_second.end(), true)) <=
            getNumberOfTurns(next_intersection))
            matched_at_second[none_index] = true;
    }

    const auto augmentEntry = [&](TurnLaneData &data) {
        for (std::size_t lane = 0; lane < turn_lane_data.size(); ++lane)
            if (matched_at_second[lane])
            {
                data.from = std::min(turn_lane_data[lane].from, data.from);
                data.to = std::max(turn_lane_data[lane].to, data.to);
            }

    };

    LaneDataVector first, second;
    for (std::size_t lane = 0; lane < turn_lane_data.size(); ++lane)
    {

        if (matched_at_second[lane])
            second.push_back(turn_lane_data[lane]);

        // augment straightmost at this intersection to match all turns that happen at the next
        if (lane == straightmost_tag_index)
        {
            augmentEntry(turn_lane_data[straightmost_tag_index]);
        }

        if (matched_at_first[lane])
            first.push_back(turn_lane_data[lane]);
    }

    if (straightmost_tag_index == turn_lane_data.size() &&
        static_cast<std::size_t>(
            std::count(matched_at_second.begin(), matched_at_second.end(), true)) ==
            getNumberOfTurns(next_intersection))
    {
        TurnLaneData data = {TurnLaneType::straight, 255, 0};
        augmentEntry(data);
        first.push_back(data);
        std::sort(first.begin(), first.end());
    }

    // TODO augment straightmost turn
    return {std::move(first), std::move(second)};
}

Intersection TurnLaneHandler::simpleMatchTuplesToTurns(Intersection intersection,
                                                       const LaneDataVector &lane_data,
                                                       const LaneDescriptionID lane_description_id,
                                                       LaneDataIdMap &id_map) const
{
    if (lane_data.empty() || !canMatchTrivially(intersection, lane_data))
        return intersection;

    BOOST_ASSERT(
        !hasTag(TurnLaneType::none | TurnLaneType::merge_to_left | TurnLaneType::merge_to_right,
                lane_data));

    return triviallyMatchLanesToTurns(
        std::move(intersection), lane_data, node_based_graph, lane_description_id, id_map);
}

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm
