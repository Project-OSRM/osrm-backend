#ifndef OSRM_ENGINE_GUIDANCE_COLLAPSE_HPP

#include "engine/guidance/route_step.hpp"
#include "util/attributes.hpp"

#include <type_traits>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

// Multiple possible reasons can result in unnecessary/confusing instructions
// Collapsing such turns into a single turn instruction, we give a clearer
// set of instructions that is not cluttered by unnecessary turns/name changes.
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> collapseTurnInstructions(std::vector<RouteStep> steps);

// Multiple possible reasons can result in unnecessary/confusing instructions
// A prime example would be a segregated intersection. Turning around at this
// intersection would result in two instructions to turn left.
// Collapsing such turns into a single turn instruction, we give a clearer
// set of instructions that is not cluttered by unnecessary turns/name changes.
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> collapseSegregatedTurnInstructions(std::vector<RouteStep> steps);

// A combined turn is a set of two instructions that actually form a single turn, as far as we
// perceive it. A u-turn consisting of two left turns is one such example. But there are also lots
// of other items that influence how we combine turns. This function is an entry point, defining the
// possibility to select one of multiple strategies when combining a turn with another one.
template <typename CombinedTurnStrategy, typename SignageStrategy>
RouteStep combineRouteSteps(const RouteStep &step_at_turn_location,
                            const RouteStep &step_after_turn_location,
                            const CombinedTurnStrategy combined_turn_stragey,
                            const SignageStrategy signage_strategy);

// TAGS
// These tags are used to ensure correct strategy usage. Make sure your new strategy is derived from
// (at least) one of these tags. It can only be used for the intended tags, to ensure we don't
// accidently use a lane strategy to cover signage
struct CombineStrategy
{
};

struct SignageStrategy
{
};

struct LaneStrategy
{
};

// Return the step at the turn location, without modification
struct NoModificationStrategy : CombineStrategy, SignageStrategy, LaneStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// transfer the turn type from the second step
struct TransferTurnTypeStrategy : CombineStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// Combine both turn and turn angle to a common item
struct AdjustToCombinedTurnAngleStrategy : CombineStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// Combine only the turn types
struct AdjustToCombinedTurnStrategy : CombineStrategy
{
    AdjustToCombinedTurnStrategy(const RouteStep &step_prior_to_intersection);
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;

    const RouteStep &step_prior_to_intersection;
};

// Set a fixed instruction type
struct SetFixedInstructionStrategy : CombineStrategy
{
    SetFixedInstructionStrategy(const osrm::guidance::TurnInstruction instruction);
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;

    const osrm::guidance::TurnInstruction instruction;
};

// Handling of staggered intersections
struct StaggeredTurnStrategy : CombineStrategy
{
    StaggeredTurnStrategy(const RouteStep &step_prior_to_intersection);

    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;

    const RouteStep &step_prior_to_intersection;
};

// Handling of consecutive segregated steps
struct CombineSegregatedStepsStrategy : CombineStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// Handling of segregated intersections
struct SegregatedTurnStrategy : CombineStrategy
{
    SegregatedTurnStrategy(const RouteStep &step_prior_to_intersection);

    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;

    const RouteStep &step_prior_to_intersection;
};

// Signage Strategies

// Transfer the signage from the next step onto this step
struct TransferSignageStrategy : SignageStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// Lane Strategies

// Transfer the turn lanes from the intermediate step
struct TransferLanesStrategy : LaneStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// Pattern to combine a route step using the predefined strategies
template <typename CombineStrategyClass, typename SignageStrategyClass, typename LaneStrategyClass>
void combineRouteSteps(RouteStep &step_at_turn_location,
                       RouteStep &step_after_turn_location,
                       CombineStrategyClass combined_turn_stragey,
                       SignageStrategyClass signage_strategy,
                       LaneStrategyClass lane_strategy)
{
    // assign the combined turn type
    static_assert(std::is_base_of<CombineStrategy, CombineStrategyClass>::value,
                  "Supplied Strategy isn't a combine strategy.");
    combined_turn_stragey(step_at_turn_location, step_after_turn_location);

    // assign the combind signage
    static_assert(std::is_base_of<LaneStrategy, LaneStrategyClass>::value,
                  "Supplied Strategy isn't a signage strategy.");
    signage_strategy(step_at_turn_location, step_after_turn_location);

    // assign the desired turn lanes
    static_assert(std::is_base_of<LaneStrategy, LaneStrategyClass>::value,
                  "Supplied Strategy isn't a lane strategy.");
    lane_strategy(step_at_turn_location, step_after_turn_location);

    // further stuff should happen here as well
    step_at_turn_location.ElongateBy(step_after_turn_location);
    step_after_turn_location.Invalidate();
}

// alias for suppressing a step, using CombineRouteStep with NoModificationStrategy only
void suppressStep(RouteStep &step_at_turn_location, RouteStep &step_after_turn_location);

} /* namespace guidance */
} // namespace engine
} /* namespace osrm */

#endif /* OSRM_ENGINE_GUIDANCE_COLLAPSE_HPP_ */
