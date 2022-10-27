#include "util/for_each_pair.hpp"
#include "util/group_by.hpp"

#include "guidance/turn_instruction.hpp"
#include "engine/guidance/collapsing_utility.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <utility>

namespace osrm
{
namespace engine
{
namespace guidance
{
using namespace osrm::guidance;

std::vector<RouteStep> anticipateLaneChange(std::vector<RouteStep> steps,
                                            const double min_distance_needed_for_lane_change)
{
    // Lane anticipation works on contiguous ranges of short steps that have lane information
    const auto is_short_has_lanes = [&](const RouteStep &step) {
        const auto has_lanes = step.intersections.front().lanes.lanes_in_turn > 0;

        if (!has_lanes)
            return false;

        // The more unused lanes to the left and right of the turn there are, the higher
        // the chance the user is driving on one of those and has to cross lanes.
        // Scale threshold for these cases to be adaptive to the situation's complexity.
        //
        // Note: what we could do instead: do Lane Anticipation on all step pairs and then scale
        // the threshold based on the lanes we're constraining the user to. Would need a re-write
        // since at the moment we first group-by and only then do Lane Anticipation selectively.
        //
        // We do not have a source-target lane mapping, assume worst case for lanes to cross.
        const auto to_cross = std::max(step.NumLanesToTheRight(), step.NumLanesToTheLeft());
        const auto scale = 1 + to_cross;
        const auto threshold = scale * min_distance_needed_for_lane_change;

        const auto is_short = step.distance < threshold;
        return is_short;
    };

    using StepIter = decltype(steps)::iterator;
    using StepIterRange = std::pair<StepIter, StepIter>;

    std::vector<StepIterRange> quick_lanes_ranges;

    const auto range_back_inserter = [&](StepIterRange range) {
        if (std::distance(range.first, range.second) > 1)
            quick_lanes_ranges.push_back(std::move(range));
    };

    util::group_by(begin(steps), end(steps), is_short_has_lanes, range_back_inserter);

    // The lanes for a keep straight depend on the next left/right turn. Tag them in advance.
    std::unordered_set<const RouteStep *> is_straight_left;
    std::unordered_set<const RouteStep *> is_straight_right;

    // Walk backwards over all turns, constraining possible turn lanes.
    // Later turn lanes constrain earlier ones: we have to anticipate lane changes.
    const auto constrain_lanes = [&](const StepIterRange &turns) {
        const std::reverse_iterator<StepIter> rev_first{turns.second};
        const std::reverse_iterator<StepIter> rev_last{turns.first};

        // We're walking backwards over all adjacent turns:
        // the current turn lanes constrain the lanes we have to take in the previous turn.

        // state for the lamda
        // the number of lanes we have to change depends on the number of lanes that are allowed for
        // a turn (in general) and the set of lanes which would allow for us to do the turn without
        // a problem. In a sequence of turns, we have to look at how much distance we need to switch
        // the
        // sequence. Given the turns in between, we would expect a bit longer than on a straight
        // segment for a lane switch, but the total distance shouldn't be unlimited.
        double distance_to_constrained = 0.0;

        util::for_each_pair(rev_first, rev_last, [&](RouteStep &current, RouteStep &previous) {
            const auto current_inst = current.maneuver.instruction;
            const auto current_lanes = current.intersections.front().lanes;

            // Constrain the previous turn's lanes
            auto &previous_lanes = previous.intersections.front().lanes;
            const auto previous_inst = previous.maneuver.instruction;

            // Lane mapping (N:M) from previous lanes (N) to current lanes (M), with:
            //  N > M, N > 1   fan-in situation, constrain N lanes to min(N,M) shared lanes
            //  otherwise      nothing to constrain
            const bool lanes_to_constrain = previous_lanes.lanes_in_turn > 1;
            const bool lanes_fan_in = previous_lanes.lanes_in_turn > current_lanes.lanes_in_turn;

            // only prevent use lanes due to making all turns. don't make turns during curvy
            // segments
            if (previous_inst.type == TurnType::Suppressed)
                distance_to_constrained += previous.distance;
            else
                distance_to_constrained = 0.;

            const auto lane_delta = previous_lanes.lanes_in_turn - current_lanes.lanes_in_turn;
            const auto can_make_all_turns =
                distance_to_constrained > lane_delta * min_distance_needed_for_lane_change;

            if (!lanes_to_constrain || !lanes_fan_in || can_make_all_turns)
                return;

            // We do not have a mapping from lanes to lanes. All we have is the lanes in the turn
            // and all the lanes at that situation. To perfectly handle lane anticipation in cases
            // where lanes in the turn fan in but for example the overall lanes at that location
            // fan out, we would have to know the asymmetric mapping of lanes. This is currently
            // not possible at the moment. In the following we implement a heuristic instead.
            const LaneID current_num_lanes_right_of_turn = current.NumLanesToTheRight();
            const LaneID current_num_lanes_left_of_turn = current.NumLanesToTheLeft();

            // 0/ Tag keep straight with the next turn's direction if available
            const auto previous_is_straight =
                !isLeftTurn(previous_inst) && !isRightTurn(previous_inst);

            if (previous_is_straight)
            {
                if (isLeftTurn(current_inst) || is_straight_left.count(&current) > 0)
                    is_straight_left.insert(&previous);
                else if (isRightTurn(current_inst) || is_straight_right.count(&current) > 0)
                    is_straight_right.insert(&previous);
            }

            // 1/ How to anticipate left, right:
            const auto anticipate_for_left_turn = [&] {
                // Current turn is left turn, already keep left during previous turn.
                // This implies constraining the rightmost lanes in previous step.
                LaneID new_first_lane_from_the_right =
                    previous_lanes.first_lane_from_the_right // start from rightmost lane
                    + previous_lanes.lanes_in_turn           // one past leftmost lane
                    - current_lanes.lanes_in_turn;           // back number of new lanes

                // The leftmost target lanes might not be involved in the turn. Figure out
                // how many lanes are to the left and not in the turn.
                new_first_lane_from_the_right -=
                    std::min(current_num_lanes_left_of_turn, current_lanes.lanes_in_turn);

                previous_lanes = {current_lanes.lanes_in_turn, new_first_lane_from_the_right};
            };

            const auto anticipate_for_right_turn = [&] {
                // Current turn is right turn, already keep right during the previous turn.
                // This implies constraining the leftmost lanes in the previous turn step.
                LaneID new_first_lane_from_the_right = previous_lanes.first_lane_from_the_right;

                // The rightmost target lanes might not be involved in the turn. Figure out
                // how many lanes are to the right and not in the turn.
                new_first_lane_from_the_right +=
                    std::min(current_num_lanes_right_of_turn, current_lanes.lanes_in_turn);

                previous_lanes = {current_lanes.lanes_in_turn, new_first_lane_from_the_right};
            };

            // 2/ When to anticipate a left, right turn
            if (isLeftTurn(current_inst))
                anticipate_for_left_turn();
            else if (isRightTurn(current_inst))
                anticipate_for_right_turn();
            else // keepStraight
            {
                // Heuristic: we do not have a from-lanes -> to-lanes mapping. What we use
                // here instead in addition is the number of all lanes (not only the lanes
                // in a turn):
                //
                // -v-v v-v-        straight follows
                //  | | | |
                // <- v v ->        keep straight here
                //    | |
                //  <-| |->
                //
                // A route from the top left to the bottom right here goes over a keep
                // straight. If we handle all keep straights as right turns (in right-sided
                // driving), we wrongly guide the user to the rightmost lanes in the first turn.
                // Not only is this wrong but the opposite of what we expect.
                //
                // The following implements a heuristic to determine a keep straight's
                // direction in relation to the next step. In the above example we would get:
                //
                // coming from right, going to left (in direction of way) -> handle as left turn

                if (is_straight_left.count(&current) > 0)
                    anticipate_for_left_turn();
                else if (is_straight_right.count(&current) > 0)
                    anticipate_for_right_turn();
                else // FIXME: right-sided driving
                    anticipate_for_right_turn();
            }

            if (previous_inst.type == TurnType::Suppressed &&
                current_inst.type == TurnType::Suppressed && previous.mode == current.mode &&
                previous_lanes == current_lanes)
            {
                previous.ElongateBy(current);
                current.Invalidate();
            }
        });
    };

    std::for_each(begin(quick_lanes_ranges), end(quick_lanes_ranges), constrain_lanes);

    // Lane Anticipation might have collapsed steps after constraining lanes. Remove invalid steps.
    steps = removeNoTurnInstructions(std::move(steps));

    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
