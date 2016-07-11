#include "util/debug.hpp"

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

const constexpr char *TurnLaneHandler::scenario_names[TurnLaneScenario::NUM_SCENARIOS];

TurnLaneHandler::TurnLaneHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                 std::vector<std::uint32_t> &turn_lane_offsets,
                                 std::vector<TurnLaneType::Mask> &turn_lane_masks,
                                 LaneDescriptionMap &lane_description_map,
                                 const std::vector<QueryNode> &node_info_list,
                                 const TurnAnalysis &turn_analysis,
                                 LaneDataIdMap &id_map)
    : node_based_graph(node_based_graph), turn_lane_offsets(turn_lane_offsets),
      turn_lane_masks(turn_lane_masks), lane_description_map(lane_description_map),
      node_info_list(node_info_list), turn_analysis(turn_analysis), id_map(id_map)
{
    count_handled = new unsigned;
    count_called = new unsigned;
    *count_handled = *count_called = 0;
}

TurnLaneHandler::~TurnLaneHandler()
{
    std::cout << "Handled: " << *count_handled << " of " << *count_called
              << " lanes: " << (double)(*count_handled * 100) / (*count_called) << " %."
              << std::endl;
    delete count_called;
    delete count_handled;
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
Intersection
TurnLaneHandler::assignTurnLanes(const NodeID at, const EdgeID via_edge, Intersection intersection)
{
    //if only a uturn exists, there is nothing we can do
    if( intersection.size() == 1 )
        return intersection;

    // A list of output parameters to be filled in during deduceScenario.
    // Data for the current intersection
    LaneDescriptionID lane_description_id = INVALID_LANE_DESCRIPTIONID;
    LaneDataVector lane_data;
    // Data for the previous intersection
    NodeID previous_node = SPECIAL_NODEID;
    EdgeID previous_via_edge = SPECIAL_EDGEID;
    Intersection previous_intersection;
    LaneDataVector previous_lane_data;
    LaneDescriptionID previous_description_id = INVALID_LANE_DESCRIPTIONID;

    const auto scenario = deduceScenario(at,
                                         via_edge,
                                         intersection,
                                         lane_description_id,
                                         lane_data,
                                         previous_node,
                                         previous_via_edge,
                                         previous_intersection,
                                         previous_lane_data,
                                         previous_description_id);

    if (scenario != TurnLaneHandler::NONE)
        (*count_called)++;

    switch (scenario)
    {
    // A turn based on current lane data
    case TurnLaneScenario::SIMPLE:
    case TurnLaneScenario::PARTITION_LOCAL:
        lane_data = handleNoneValueAtSimpleTurn(std::move(lane_data), intersection);
        return simpleMatchTuplesToTurns(std::move(intersection), lane_data, lane_description_id);

    // Cases operating on data carried over from a previous lane
    case TurnLaneScenario::SIMPLE_PREVIOUS:
    case TurnLaneScenario::PARTITION_PREVIOUS:
        previous_lane_data =
            handleNoneValueAtSimpleTurn(std::move(previous_lane_data), intersection);
        return simpleMatchTuplesToTurns(
            std::move(intersection), previous_lane_data, previous_description_id);

    // Sliproads-turns that are to be handled as a single entity
    case TurnLaneScenario::SLIPROAD:
        return handleSliproadTurn(std::move(intersection),
                                  lane_description_id,
                                  std::move(lane_data),
                                  previous_intersection,
                                  previous_description_id,
                                  previous_lane_data);
    case TurnLaneScenario::MERGE:
        return intersection;
    default:
        // All different values that we cannot handle. For some me might want to print debug output
        // later on, when the handling is actually improved to work in close to all cases.
        // case TurnLaneScenario::UNKNOWN:
        // case TurnLaneScenario::NONE:
        // case TurnLaneScenario::INVALID:
        {
            static int print_count = 0;
            if (TurnLaneScenario::NONE != scenario && print_count++ < 10)
            {
                std::cout << "[Unhandled] " << (int)lane_description_id << " -- "
                          << (int)previous_description_id << "\n"
                          << std::endl;
                util::guidance::printTurnAssignmentData(
                    at, lane_data, intersection, node_info_list);

                if (previous_node != SPECIAL_NODEID)
                    util::guidance::printTurnAssignmentData(
                        previous_node, previous_lane_data, previous_intersection, node_info_list);
            }
        }
        return intersection;
    }
}

// Find out which scenario we have to handle
TurnLaneHandler::TurnLaneScenario
TurnLaneHandler::deduceScenario(const NodeID at,
                                const EdgeID via_edge,
                                const Intersection &intersection,
                                // Output Variables
                                LaneDescriptionID &lane_description_id,
                                LaneDataVector &lane_data,
                                NodeID &previous_node,
                                EdgeID &previous_via_edge,
                                Intersection &previous_intersection,
                                LaneDataVector &previous_lane_data,
                                LaneDescriptionID &previous_description_id)
{
    std::cout << "Checking For Lanes" << std::endl;
    // if only a uturn exists, there is nothing we can do
    if (intersection.size() == 1)
        return TurnLaneHandler::NONE;

    // Sliproads are handled using the previous intersection mechanig
    for (const auto &road : intersection)
        if (road.turn.instruction.type == TurnType::Sliproad)
            return TurnLaneHandler::NONE;

    extractLaneData(via_edge, lane_description_id, lane_data);

    // traffic lights are not compressed during our preprocessing. Due to this *shortcoming*, we can
    // get to the following situation:
    //
    //         d
    // a - b - c
    //         e
    //
    // with a traffic light at b and a-b as well as b-c offering the same turn lanes.
    // In such a situation, we don't need to handle the lanes at a-b, since we will get the same
    // information at b-c, where the actual turns are taking place.
    const bool is_going_straight_and_turns_continue =
        (intersection.size() == 2 &&
         ((lane_description_id != INVALID_LANE_DESCRIPTIONID &&
           lane_description_id ==
               node_based_graph.GetEdgeData(intersection[1].turn.eid).lane_description_id) ||
          angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE));

    if (is_going_straight_and_turns_continue)
        return TurnLaneHandler::NONE;

    // if we see an invalid conversion, we stop immediately
    if (lane_description_id != INVALID_LANE_DESCRIPTIONID && lane_data.empty())
        return TurnLaneScenario::INVALID;

    // might be reasonable to handle multiple turns, if we know of a sequence of lanes
    // e.g. one direction per lane, if three lanes and right, through, left available
    if (lane_description_id != INVALID_LANE_DESCRIPTIONID && lane_data.size() == 1 &&
        lane_data[0].tag == TurnLaneType::none)
        return TurnLaneScenario::INVALID;

    // Due to sliproads, we might need access to the previous intersection at this point already;
    previous_node = SPECIAL_NODEID;
    previous_via_edge = SPECIAL_EDGEID;
    if (findPreviousIntersection(at,
                                 via_edge,
                                 intersection,
                                 turn_analysis,
                                 node_based_graph,
                                 previous_node,
                                 previous_via_edge,
                                 previous_intersection))
    {
        extractLaneData(previous_via_edge, previous_description_id, previous_lane_data);
        for (const auto &road : previous_intersection)
            if (road.turn.instruction.type == TurnType::Sliproad)
                return TurnLaneScenario::SLIPROAD;
    }

    const std::size_t possible_entries = getNumberOfTurns(intersection);

    // merge does not justify an instruction
    const bool has_merge_lane =
        hasTag(TurnLaneType::merge_to_left | TurnLaneType::merge_to_right, lane_data);
    if (has_merge_lane)
        return TurnLaneScenario::MERGE;

    // Dead end streets that don't have any left-tag. This can happen due to the fallbacks for
    // broken data/barriers.
    const bool has_non_usable_u_turn = (intersection[0].entry_allowed &&
                                        !hasTag(TurnLaneType::none | TurnLaneType::left |
                                                    TurnLaneType::sharp_left | TurnLaneType::uturn,
                                                lane_data) &&
                                        lane_data.size() + 1 == possible_entries);

    if (has_non_usable_u_turn)
        return TurnLaneScenario::INVALID;

    // Handle possible u-turn scenarios
    if (!lane_data.empty() && canMatchTrivially(intersection, lane_data) &&
        lane_data.size() !=
            static_cast<std::size_t>(
                lane_data.back().tag != TurnLaneType::uturn && intersection[0].entry_allowed ? 1
                                                                                             : 0) +
                possible_entries &&
        intersection[0].entry_allowed && !hasTag(TurnLaneType::none, lane_data))
        lane_data.push_back({TurnLaneType::uturn, lane_data.back().to, lane_data.back().to});

    bool is_simple = isSimpleIntersection(lane_data, intersection);

    std::cout << "Check for simple: " << (is_simple ? "true" : "false") << std::endl;

    if (is_simple)
        return TurnLaneScenario::SIMPLE;

    // In case of intersections that don't offer all turns right away, we have to account for
    // *delayed* turns. Consider the following example:
    //
    //         e
    // a - b - c - f
    //     d
    //
    // With lanes on a-b indicating: | left | through | right |.
    // While right obviously refers to a-b-d, through and left refer to a-b-c-f and a-b-c-e
    // respectively. While we are at a-b, we have to consider the right turn only.
    // The turn a-b-c gets assigned the lanes of both *left* and *through*.
    // At b-c, we get access to either a new set of lanes, or -- via the previous intersection -- to
    // the second part of | left | through | right |. Lane anticipation can then deduce which lanes
    // correspond to what and suppress unnecessary instructions.
    //
    // For our initial case, we consider only the turns that are available at the current location,
    // which are given by partitioning the lane data and selecting the first part.
    if (!lane_data.empty())
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
                return TurnLaneScenario::PARTITION_LOCAL;
        }
        // If partitioning doesn't solve the problem, we don't know how to handle it right now
        return TurnLaneScenario::UNKNOWN;
    }

    if (lane_description_id != INVALID_LANE_DESCRIPTIONID)
        return TurnLaneScenario::UNKNOWN;

    // acquire the lane data of a previous segment and, if possible, use it for the current
    // intersection.
    if (previous_via_edge == SPECIAL_EDGEID)
        return TurnLaneScenario::NONE;

    if (previous_lane_data.empty())
        return TurnLaneScenario::NONE;

    const bool previous_has_merge_lane =
        hasTag(TurnLaneType::merge_to_left | TurnLaneType::merge_to_right, previous_lane_data);
    if (previous_has_merge_lane)
        return TurnLaneScenario::MERGE;

    std::cout << "Checking Previous" << std::endl;
    const auto is_simple_previous = isSimpleIntersection(previous_lane_data, intersection);
    if (is_simple_previous)
        return TurnLaneScenario::SIMPLE_PREVIOUS;

    // This is the second part of the previously described partitioning scenario.
    if (previous_lane_data.size() >= getNumberOfTurns(previous_intersection) &&
        previous_intersection.size() != 2)
    {
        std::cout << "Checking Partition" << std::endl;
        previous_lane_data = partitionLaneData(node_based_graph.GetTarget(previous_via_edge),
                                               std::move(previous_lane_data),
                                               previous_intersection)
                                 .second;

        std::sort(previous_lane_data.begin(), previous_lane_data.end());

        std::cout << "Checking Intersection " << previous_lane_data.size()
                  << " Entries: " << possible_entries
                  << " Simple: " << isSimpleIntersection(previous_lane_data, intersection)
                  << std::endl;
        // check if we were successfull in trimming
        if ((previous_lane_data.size() == possible_entries) &&
            isSimpleIntersection(previous_lane_data, intersection))
            return TurnLaneScenario::PARTITION_PREVIOUS;
    }

    return TurnLaneScenario::UNKNOWN;
}

void TurnLaneHandler::extractLaneData(const EdgeID via_edge,
                                      LaneDescriptionID &lane_description_id,
                                      LaneDataVector &lane_data) const
{
    const auto &edge_data = node_based_graph.GetEdgeData(via_edge);
    // TODO access correct data
    lane_description_id = edge_data.lane_description_id;
    const auto lane_description =
        lane_description_id != INVALID_LANE_DESCRIPTIONID
            ? TurnLaneDescription(turn_lane_masks.begin() + turn_lane_offsets[lane_description_id],
                                  turn_lane_masks.begin() +
                                      turn_lane_offsets[lane_description_id + 1])
            : TurnLaneDescription();

    if (!lane_description.empty())
        lane_data = laneDataFromDescription(lane_description);

    BOOST_ASSERT(lane_description.empty() ||
                 lane_description.size() == (turn_lane_offsets[lane_description_id + 1] -
                                             turn_lane_offsets[lane_description_id]));
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
     * Intersections like these have two parts. Turns that can be made at the first intersection
     * and
     * turns that have to be made at the second. The partitioning returns the lane data split
     * into
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

    // find out about the next intersection. To check for valid matches, we also need the turn
    // types
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
        if (best_match->entry_allowed &&
            isValidMatch(turn_lane_data[lane].tag, best_match->turn.instruction))
        {
            matched_at_first[lane] = true;

            if (straightmost == best_match)
                straightmost_tag_index = lane;
        }

        const auto best_match_at_next_intersection =
            findBestMatch(turn_lane_data[lane].tag, next_intersection);
        if (best_match_at_next_intersection->entry_allowed &&
            isValidMatch(turn_lane_data[lane].tag,
                         best_match_at_next_intersection->turn.instruction))
        {
            if (!matched_at_first[lane] || turn_lane_data[lane].tag == TurnLaneType::straight ||
                getMatchingQuality(turn_lane_data[lane].tag, *best_match) >
                    getMatchingQuality(turn_lane_data[lane].tag, *best_match_at_next_intersection))
            {
                if (turn_lane_data[lane].tag != TurnLaneType::straight)
                    matched_at_first[lane] = false;

                matched_at_second[lane] = true;
            }
        }

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
        std::cout << "Lane: " << matched_at_first[lane] << " " << matched_at_second[lane]
                  << std::endl;
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
                                                       const LaneDescriptionID lane_description_id)
{
    std::cout << "Assigning" << std::endl;
    if (lane_data.empty() || !canMatchTrivially(intersection, lane_data))
        return intersection;

    util::guidance::printTurnAssignmentData(node_based_graph.GetTarget(intersection[0].turn.eid),
                                            lane_data,
                                            intersection,
                                            node_info_list);

    BOOST_ASSERT(
        !hasTag(TurnLaneType::none | TurnLaneType::merge_to_left | TurnLaneType::merge_to_right,
                lane_data));

    (*count_handled)++;

    return triviallyMatchLanesToTurns(
        std::move(intersection), lane_data, node_based_graph, lane_description_id, id_map);
}

Intersection
TurnLaneHandler::handleSliproadTurn(Intersection intersection,
                                    const LaneDescriptionID lane_description_id,
                                    LaneDataVector lane_data,
                                    const Intersection &previous_intersection,
                                    const LaneDescriptionID &previous_lane_description_id,
                                    const LaneDataVector &previous_lane_data)
{
    if (isSubsetOf(lane_data, previous_lane_data) && isSimpleIntersection(lane_data, intersection))
    {
        // Adjust lane_data to represent the turn lanes at the previous intersection
        for (auto &entry : lane_data)
        {
            const auto match = findTag(entry.tag, previous_lane_data);
            BOOST_ASSERT(match != previous_lane_data.end());
            entry.from = match->from;
            entry.to = match->to;
        }
        return simpleMatchTuplesToTurns(
            std::move(intersection), lane_data, previous_lane_description_id);
    }
    else if (previous_lane_description_id == INVALID_LANE_DESCRIPTIONID &&
             isSimpleIntersection(lane_data, intersection))
    {
        // check if we can combine lanes
        TurnLaneDescription combined_description;
        for (auto rev_road_itr = previous_intersection.rbegin();
             rev_road_itr != previous_intersection.rend();
             rev_road_itr = std::next(rev_road_itr))
        {
            const auto &previous_road = *rev_road_itr;

            if (!previous_road.entry_allowed)
                continue;

            const auto &edge_data = node_based_graph.GetEdgeData(previous_road.turn.eid);
            const auto lane_description_id = edge_data.lane_description_id;
            if (lane_description_id == INVALID_LANE_DESCRIPTIONID)
            {
                // TODO here we might want to get back to a normal scenario?
                // if not all turns offer lanes, we cannot handle the situation
                return intersection;
            }
            // add data to combined description
            combined_description.insert(
                combined_description.end(),
                turn_lane_masks.begin() + turn_lane_offsets[lane_description_id],
                turn_lane_masks.begin() + turn_lane_offsets[lane_description_id + 1]);
        }
        auto combined_data = laneDataFromDescription(combined_description);
        for (auto &entry : lane_data)
        {
            const auto match = findTag(entry.tag, combined_data);
            BOOST_ASSERT(match != combined_data.end());
            entry.from = match->from;
            entry.to = match->to;
        }

        const auto combined_id = [&]() {
            auto itr = lane_description_map.find(combined_description);
            if (lane_description_map.find(combined_description) == lane_description_map.end())
            {
                const auto new_id =
                    boost::numeric_cast<LaneDescriptionID>(lane_description_map.size());
                lane_description_map[combined_description] = new_id;
                return new_id;
            }
            else
            {
                return itr->second;
            }
        }();
        return simpleMatchTuplesToTurns(std::move(intersection), lane_data, combined_id);
    }
    return intersection;
}

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm
