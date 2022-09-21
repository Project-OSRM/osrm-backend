#include "engine/guidance/verbosity_reduction.hpp"
#include "engine/guidance/collapsing_utility.hpp"

#include <boost/assert.hpp>
#include <iterator>

namespace osrm
{
namespace engine
{
namespace guidance
{
using namespace osrm::guidance;

std::vector<RouteStep> suppressShortNameSegments(std::vector<RouteStep> steps)
{
    // guard against empty routes, even though they shouldn't happen
    if (steps.empty())
        return steps;

    // we remove only name changes that don't offer additional information
    const auto name_change_without_lanes = [](const RouteStep &step) {
        return hasTurnType(step, TurnType::NewName) && !hasLanes(step);
    };

    // check if the next step is not important enough to announce
    const auto can_be_extended_to = [](const RouteStep &step) {
        const auto is_not_arrive = !hasWaypointType(step);
        const auto is_silent = !hasTurnType(step) || hasTurnType(step, TurnType::Suppressed);

        return is_not_arrive && is_silent;
    };

    const auto suppress = [](RouteStep &from_step, RouteStep &onto_step) {
        from_step.ElongateBy(onto_step);
        onto_step.Invalidate();
    };

    // suppresses name segments that announce already known names or announce a name that will be
    // only available for a very short time
    const auto reduce_verbosity_if_possible =
        [suppress, can_be_extended_to](RouteStepIterator &current_turn_itr,
                                       RouteStepIterator &previous_turn_itr) {
            if (haveSameName(*previous_turn_itr, *current_turn_itr))
                suppress(*previous_turn_itr, *current_turn_itr);
            else
            {
                // remember the location of the name change so we can advance the previous turn
                const auto location_of_name_change = current_turn_itr;
                auto distance = current_turn_itr->distance;
                // sum up all distances that can be relevant to the name change
                while (can_be_extended_to(*(current_turn_itr + 1)) &&
                       distance < NAME_SEGMENT_CUTOFF_LENGTH)
                {
                    ++current_turn_itr;
                    distance += current_turn_itr->distance;
                }

                if (distance < NAME_SEGMENT_CUTOFF_LENGTH)
                    suppress(*previous_turn_itr, *current_turn_itr);
                else
                    previous_turn_itr = location_of_name_change;
            }
        };

    BOOST_ASSERT(!hasTurnType(steps.back()) && hasWaypointType(steps.back()));
    for (auto previous_turn_itr = steps.begin(), current_turn_itr = std::next(previous_turn_itr);
         !hasWaypointType(*current_turn_itr);
         ++current_turn_itr)
    {
        BOOST_ASSERT(hasTurnType(*current_turn_itr) &&
                     !hasTurnType(*current_turn_itr, TurnType::Suppressed));
        if (name_change_without_lanes(*current_turn_itr) &&
            haveSameMode(*previous_turn_itr, *current_turn_itr))
        {
            // check if the name can be reduced, also sets previous_turn_itr if update is necessary
            reduce_verbosity_if_possible(current_turn_itr, previous_turn_itr);
        }
        else
        {
            // remember the current (non-suppressed) item as a new start of a segment
            previous_turn_itr = current_turn_itr;
        }
    }
    return removeNoTurnInstructions(std::move(steps));
}

} // namespace guidance
} // namespace engine
} // namespace osrm
