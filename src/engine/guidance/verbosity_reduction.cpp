#include "engine/guidance/verbosity_reduction.hpp"
#include "engine/guidance/collapsing_utility.hpp"

namespace osrm
{
namespace engine
{
namespace guidance
{
std::vector<RouteStep> suppressShortNameSegments(std::vector<RouteStep> steps)
{
    // guard against empty routes, even though they shouldn't happen
    if (steps.empty())
        return steps;

    const auto suppress = [](RouteStep &from_step, RouteStep &onto_step) {
        if ((onto_step.maneuver.instruction.type == TurnType::NewName) &&
            haveSameMode(from_step, onto_step))
        {
            from_step.ElongateBy(onto_step);
            onto_step.Invalidate();
            return true;
        }
        else
        {
            return false;
        }
    };

    for (auto itr = steps.begin(); itr + 1 != steps.end();)
    {
        // suppress all new names along the path
        auto next = itr + 1;
        while (suppress(*itr, *next))
            ++next;
        itr = next;
    }

    return removeNoTurnInstructions(std::move(steps));
}

} // namespace guidance
} // namespace engine
} // namespace osrm
