#include "util/debug.hpp"
#include "util/for_each_pair.hpp"
#include "util/group_by.hpp"
#include "util/guidance/toolkit.hpp"

#include "extractor/guidance/turn_instruction.hpp"

#include <iterator>
#include <unordered_set>
#include <utility>

using TurnInstruction = osrm::extractor::guidance::TurnInstruction;
namespace TurnType = osrm::extractor::guidance::TurnType;
namespace DirectionModifier = osrm::extractor::guidance::DirectionModifier;

using osrm::util::guidance::isLeftTurn;
using osrm::util::guidance::isRightTurn;

namespace osrm
{
namespace engine
{
namespace guidance
{

std::vector<RouteStep> anticipateLaneChange(std::vector<RouteStep> steps)
{
    util::guidance::print(steps);
    const constexpr auto MIN_DURATION_NEEDED_FOR_LANE_CHANGE = 15.;

    // Lane anticipation works on contiguous ranges of quick steps that have lane information
    const auto is_quick_has_lanes = [&](const RouteStep &step) {
        const auto is_quick = step.duration < MIN_DURATION_NEEDED_FOR_LANE_CHANGE;
        const auto has_lanes = step.maneuver.lanes.lanes_in_turn > 0;
        return has_lanes && is_quick;
    };

    using StepIter = decltype(steps)::iterator;
    using StepIterRange = std::pair<StepIter, StepIter>;

    std::vector<StepIterRange> quick_lanes_ranges;

    const auto range_back_inserter = [&](StepIterRange range) {
        if (std::distance(range.first, range.second) > 1)
            quick_lanes_ranges.push_back(std::move(range));
    };

    util::group_by(begin(steps), end(steps), is_quick_has_lanes, range_back_inserter);

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
        util::for_each_pair(
            rev_first, rev_last, [&](const RouteStep &current, RouteStep &previous) {
                const auto previous_inst = previous.maneuver.instruction;

                const auto current_inst = current.maneuver.instruction;
                const auto current_lanes = current.maneuver.lanes;

                // Constrain the previous turn's lanes
                auto &previous_lanes = previous.maneuver.lanes;

                // Lane mapping (N:M) from previous lanes (N) to current lanes (M), with:
                //  N > M, N > 1   fan-in situation, constrain N lanes to min(N,M) shared lanes
                //  otherwise      nothing to constrain
                const bool lanes_to_constrain = previous_lanes.lanes_in_turn > 1;
                const bool lanes_fan_in =
                    previous_lanes.lanes_in_turn > current_lanes.lanes_in_turn;

                if (!lanes_to_constrain || !lanes_fan_in)
                    return;

                const auto num_shared_lanes = std::min(current_lanes.lanes_in_turn,   //
                                                       previous_lanes.lanes_in_turn); //

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
                    // This implies constraining the rightmost lanes in the previous turn
                    // step.
                    const LaneID shared_lane_delta =
                        previous_lanes.lanes_in_turn - num_shared_lanes;
                    const LaneID previous_adjusted_lanes = std::min(current_lanes.lanes_in_turn, //
                                                                    shared_lane_delta);          //
                    const LaneID constraint_first_lane_from_the_right =
                        previous_lanes.first_lane_from_the_right + previous_adjusted_lanes;

                    previous_lanes = {num_shared_lanes, constraint_first_lane_from_the_right};
                };

                const auto anticipate_for_right_turn = [&] {
                    // Current turn is right turn, already keep right during the previous turn.
                    // This implies constraining the leftmost lanes in the previous turn step.
                    previous_lanes = {num_shared_lanes, previous_lanes.first_lane_from_the_right};
                };

                // 2/ When to anticipate a left, right turn
                if (isLeftTurn(current_inst))
                    anticipate_for_left_turn();
                else if (isRightTurn(current_inst))
                    anticipate_for_right_turn();
                else // keepStraight
                {
                    // Heuristic: we do not have a from-lanes -> to-lanes mapping. What we use
                    // here
                    // instead in addition is the number of all lanes (not only the lanes in a
                    // turn):
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
                    // coming from the right, going to the left -> handle as left turn

                    if (is_straight_left.count(&current) > 0)
                        anticipate_for_left_turn();
                    else if (is_straight_right.count(&current) > 0)
                        anticipate_for_right_turn();
                    else // FIXME: right-sided driving
                        anticipate_for_right_turn();
                }
            });
    };

    std::for_each(begin(quick_lanes_ranges), end(quick_lanes_ranges), constrain_lanes);

    util::guidance::print(steps);
    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
