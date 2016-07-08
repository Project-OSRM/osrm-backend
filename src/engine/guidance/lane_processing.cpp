#include "util/for_each_pair.hpp"
#include "util/group_by.hpp"
#include "util/guidance/toolkit.hpp"

#include "extractor/guidance/turn_instruction.hpp"

#include <iterator>
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

    // Walk backwards over all turns, constraining possible turn lanes.
    // Later turn lanes constrain earlier ones: we have to anticipate lane changes.
    const auto constrain_lanes = [](const StepIterRange &turns) {
        const std::reverse_iterator<StepIter> rev_first{turns.second};
        const std::reverse_iterator<StepIter> rev_last{turns.first};

        // We're walking backwards over all adjacent turns:
        // the current turn lanes constrain the lanes we have to take in the previous turn.
        util::for_each_pair(rev_first, rev_last, [](const RouteStep &current, RouteStep &previous) {
            const auto current_inst = current.maneuver.instruction;
            const auto current_lanes = current.maneuver.lanes;

            // Constrain the previous turn's lanes
            auto &previous_lanes = previous.maneuver.lanes;

            // Lane mapping (N:M) from previous lanes (N) to current lanes (M), with:
            //  N > M, N > 1   fan-in situation, constrain N lanes to min(N,M) shared lanes
            //  otherwise      nothing to constrain
            const bool lanes_to_constrain = previous_lanes.lanes_in_turn > 1;
            const bool lanes_fan_in = previous_lanes.lanes_in_turn > current_lanes.lanes_in_turn;

            if (!lanes_to_constrain || !lanes_fan_in)
                return;

            const auto num_shared_lanes = std::min(current_lanes.lanes_in_turn,   //
                                                   previous_lanes.lanes_in_turn); //

            // FIXME: assumes right-sided driving: if it's not a left turn it is either a right turn
            // or a keep straight. Straight behave the same as right turns in right-sided driving.
            if (isLeftTurn(current_inst))
            {
                // Current turn is left turn, already keep left during previous turn.
                // This implies constraining the rightmost lanes in the previous turn step.
                const LaneID shared_lane_delta = previous_lanes.lanes_in_turn - num_shared_lanes;
                const LaneID previous_adjusted_lanes = std::min(current_lanes.lanes_in_turn, //
                                                                shared_lane_delta);          //
                const LaneID constraint_first_lane_from_the_right =
                    previous_lanes.first_lane_from_the_right + previous_adjusted_lanes;

                previous_lanes = {num_shared_lanes, constraint_first_lane_from_the_right};
            }
            else // FIXME: isRightTurn or isStraightTurn, assumes right-sided driving.
            {
                // Current turn is right turn, already keep right during the previous turn.
                // This implies constraining the leftmost lanes in the previous turn step.
                previous_lanes = {num_shared_lanes, previous_lanes.first_lane_from_the_right};
            }
        });
    };

    std::for_each(begin(quick_lanes_ranges), end(quick_lanes_ranges), constrain_lanes);

    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
