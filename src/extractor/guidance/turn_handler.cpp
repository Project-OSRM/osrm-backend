#include "extractor/guidance/turn_handler.hpp"
#include "extractor/guidance/constants.hpp"

#include "util/bearing.hpp"
#include "util/guidance/name_announcements.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include <boost/assert.hpp>
#include <boost/optional.hpp>

using osrm::extractor::guidance::getTurnDirection;
using osrm::util::angularDeviation;

namespace
{

using namespace osrm::extractor::guidance;
// given two adjacent roads in clockwise order and `road1` being a candidate for a fork,
// return false, if next road `road2` is also a fork candidate or
// return true, if `road2` is not a suitable fork candidate and thus, `road1` the outermost fork
bool isOutermostForkCandidate(const ConnectedRoad &road1, const ConnectedRoad &road2)
{
    const auto angle_between_next_road_and_straight = angularDeviation(road2.angle, STRAIGHT_ANGLE);
    const auto angle_between_prev_road_and_next = angularDeviation(road1.angle, road2.angle);
    const auto angle_between_prev_road_and_straight = angularDeviation(road1.angle, STRAIGHT_ANGLE);

    // a road is a fork candidate if it is close to straight or
    // close to a street that goes close to straight
    // (reverse to find fork non-candidate)
    if (angle_between_next_road_and_straight > NARROW_TURN_ANGLE)
    {
        if (angle_between_prev_road_and_next > NARROW_TURN_ANGLE ||
            angle_between_prev_road_and_straight > GROUP_ANGLE)
        {
            return true;
        }
    }
    return false;
}

bool isEndOfRoad(const ConnectedRoad &,
                 const ConnectedRoad &possible_right_turn,
                 const ConnectedRoad &possible_left_turn)
{
    return angularDeviation(possible_right_turn.angle, 90) < NARROW_TURN_ANGLE &&
           angularDeviation(possible_left_turn.angle, 270) < NARROW_TURN_ANGLE &&
           angularDeviation(possible_right_turn.angle, possible_left_turn.angle) >
               2 * NARROW_TURN_ANGLE;
}

template <typename InputIt>
InputIt findOutermostForkCandidate(const InputIt begin, const InputIt end)
{
    static_assert(std::is_base_of<std::input_iterator_tag,
                                  typename std::iterator_traits<InputIt>::iterator_category>::value,
                  "findOutermostForkCandidate() only accepts input iterators");
    const auto outermost = std::adjacent_find(begin, end, isOutermostForkCandidate);
    if (outermost != end)
    {
        return outermost;
    }
    // if all roads are part of a fork, set `candidate` to the last road
    else
    {
        return outermost - 1;
    }
}
}

namespace osrm
{
namespace extractor
{
namespace guidance
{

// a wrapper to handle road indices of forks at intersections
TurnHandler::Fork::Fork(const Intersection::iterator intersection_base,
                        const Intersection::iterator begin,
                        const Intersection::iterator end)
    : intersection_base(intersection_base), begin(begin), end(end), size(std::distance(begin, end))
{
    BOOST_ASSERT(begin < end);
    BOOST_ASSERT(size == 2 || size == 3);
}

ConnectedRoad &TurnHandler::Fork::getRight() { return *begin; }
ConnectedRoad &TurnHandler::Fork::getLeft() { return *(end - 1); }
ConnectedRoad &TurnHandler::Fork::getMiddle()
{
    BOOST_ASSERT(size == 3);
    return *(begin + 1);
}
ConnectedRoad &TurnHandler::Fork::getRight() const { return *begin; }
ConnectedRoad &TurnHandler::Fork::getLeft() const { return *(end - 1); }
ConnectedRoad &TurnHandler::Fork::getMiddle() const
{
    BOOST_ASSERT(size == 3);
    return *(begin + 1);
}
std::size_t TurnHandler::Fork::getRightIndex() const
{
    return std::distance(intersection_base, begin);
}
std::size_t TurnHandler::Fork::getLeftIndex() const
{
    return std::distance(intersection_base, end) - 1;
}

TurnHandler::TurnHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                         const std::vector<QueryNode> &node_info_list,
                         const util::NameTable &name_table,
                         const SuffixTable &street_name_suffix_table,
                         const IntersectionGenerator &intersection_generator)
    : IntersectionHandler(node_based_graph,
                          node_info_list,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator)
{
}

bool TurnHandler::canProcess(const NodeID, const EdgeID, const Intersection &) const
{
    return true;
}

// Handles and processes possible turns
// Input parameters describe an intersection as described in
// #IntersectionExplanation@intersection_handler.hpp
Intersection TurnHandler::
operator()(const NodeID, const EdgeID via_edge, Intersection intersection) const
{
    if (intersection.size() == 1)
        return handleOneWayTurn(std::move(intersection));

    // if u-turn is allowed, set the turn type of intersection[0] to its basic type and u-turn
    if (intersection[0].entry_allowed)
    {
        intersection[0].instruction = {findBasicTurnType(via_edge, intersection[0]),
                                       DirectionModifier::UTurn};
    }

    if (intersection.size() == 2)
        return handleTwoWayTurn(via_edge, std::move(intersection));

    if (intersection.size() == 3)
        return handleThreeWayTurn(via_edge, std::move(intersection));

    return handleComplexTurn(via_edge, std::move(intersection));
}

Intersection TurnHandler::handleOneWayTurn(Intersection intersection) const
{
    BOOST_ASSERT(intersection[0].angle < 0.001);
    return intersection;
}

Intersection TurnHandler::handleTwoWayTurn(const EdgeID via_edge, Intersection intersection) const
{
    BOOST_ASSERT(intersection[0].angle < 0.001);
    intersection[1].instruction =
        getInstructionForObvious(intersection.size(), via_edge, false, intersection[1]);

    return intersection;
}

// checks whether it is obvious to turn on `road` coming from `via_edge` while there is an`other`
// road at the same intersection
bool TurnHandler::isObviousOfTwo(const EdgeID via_edge,
                                 const ConnectedRoad &road,
                                 const ConnectedRoad &other) const
{
    const auto &via_data = node_based_graph.GetEdgeData(via_edge);
    const auto &road_data = node_based_graph.GetEdgeData(road.eid);
    const auto &other_data = node_based_graph.GetEdgeData(other.eid);
    const auto &via_classification = via_data.road_classification;
    const auto &road_classification = road_data.road_classification;
    const auto &other_classification = other_data.road_classification;

    // if one of the given roads is obvious by class, obviousness is trivial
    if (obviousByRoadClass(via_classification, road_classification, other_classification))
    {
        return true;
    }
    else if (obviousByRoadClass(via_classification, other_classification, road_classification))
    {
        return false;
    }

    const bool turn_is_perfectly_straight =
        angularDeviation(road.angle, STRAIGHT_ANGLE) < std::numeric_limits<double>::epsilon();
    if (via_data.name_id != EMPTY_NAMEID)
    {
        const auto same_name = !util::guidance::requiresNameAnnounced(
            via_data.name_id, road_data.name_id, name_table, street_name_suffix_table);
        if (turn_is_perfectly_straight && same_name)
        {
            return true;
        }
    }

    const bool is_much_narrower_than_other =
        angularDeviation(other.angle, STRAIGHT_ANGLE) /
                angularDeviation(road.angle, STRAIGHT_ANGLE) >
            INCREASES_BY_FOURTY_PERCENT &&
        angularDeviation(angularDeviation(other.angle, STRAIGHT_ANGLE),
                         angularDeviation(road.angle, STRAIGHT_ANGLE)) > FUZZY_ANGLE_DIFFERENCE;

    return is_much_narrower_than_other;
}

bool TurnHandler::hasObvious(const EdgeID &via_edge, const Fork &fork) const
{
    auto obvious_road =
        std::adjacent_find(fork.begin, fork.end, [&, this](const auto &a, const auto &b) {
            return this->isObviousOfTwo(via_edge, a, b) || this->isObviousOfTwo(via_edge, b, a);
        });
    // return whether an obvious road was found
    return obvious_road != fork.end;
}

// handles a turn at a three-way intersection _coming from_ `via_edge`
// with `intersection` as described as in #IntersectionExplanation@intersection_handler.hpp
Intersection TurnHandler::handleThreeWayTurn(const EdgeID via_edge, Intersection intersection) const
{
    BOOST_ASSERT(intersection.size() == 3);
    const auto obvious_index = findObviousTurn(via_edge, intersection);
    BOOST_ASSERT(intersection[0].angle < 0.001);
    /* Two nearly straight turns -> FORK
               OOOOOOO
             /
      IIIIII
             \
               OOOOOOO
     */

    auto fork = findFork(via_edge, intersection);
    if (fork && obvious_index == 0)
    {
        assignFork(via_edge, fork->getLeft(), fork->getRight());
    }

    /*  T Intersection

        OOOOOOO T OOOOOOOO
                I
                I
                I
     */
    else if (isEndOfRoad(intersection[0], intersection[1], intersection[2]) && obvious_index == 0)
    {
        if (intersection[1].entry_allowed)
        {
            if (TurnType::OnRamp != findBasicTurnType(via_edge, intersection[1]))
                intersection[1].instruction = {TurnType::EndOfRoad, DirectionModifier::Right};
            else
                intersection[1].instruction = {TurnType::OnRamp, DirectionModifier::Right};
        }
        if (intersection[2].entry_allowed)
        {
            if (TurnType::OnRamp != findBasicTurnType(via_edge, intersection[2]))
                intersection[2].instruction = {TurnType::EndOfRoad, DirectionModifier::Left};
            else
                intersection[2].instruction = {TurnType::OnRamp, DirectionModifier::Left};
        }
    }
    else if (obvious_index != 0) // has an obvious continuing road/obvious turn
    {
        const auto direction_at_one = getTurnDirection(intersection[1].angle);
        const auto direction_at_two = getTurnDirection(intersection[2].angle);
        if (obvious_index == 1)
        {
            intersection[1].instruction = getInstructionForObvious(
                3, via_edge, isThroughStreet(1, intersection), intersection[1]);
            const auto second_direction = (direction_at_one == direction_at_two &&
                                           direction_at_two == DirectionModifier::Straight)
                                              ? DirectionModifier::SlightLeft
                                              : direction_at_two;

            intersection[2].instruction = {findBasicTurnType(via_edge, intersection[2]),
                                           second_direction};
        }
        else
        {
            BOOST_ASSERT(obvious_index == 2);
            intersection[2].instruction = getInstructionForObvious(
                3, via_edge, isThroughStreet(2, intersection), intersection[2]);
            const auto first_direction = (direction_at_one == direction_at_two &&
                                          direction_at_one == DirectionModifier::Straight)
                                             ? DirectionModifier::SlightRight
                                             : direction_at_one;

            intersection[1].instruction = {findBasicTurnType(via_edge, intersection[1]),
                                           first_direction};
        }
    }
    else // basic turn assignment
    {
        intersection[1].instruction = {findBasicTurnType(via_edge, intersection[1]),
                                       getTurnDirection(intersection[1].angle)};
        intersection[2].instruction = {findBasicTurnType(via_edge, intersection[2]),
                                       getTurnDirection(intersection[2].angle)};
    }
    return intersection;
}

Intersection TurnHandler::handleComplexTurn(const EdgeID via_edge, Intersection intersection) const
{
    const std::size_t obvious_index = findObviousTurn(via_edge, intersection);
    const auto fork = findFork(via_edge, intersection);

    const auto straightmost = intersection.findClosestTurn(STRAIGHT_ANGLE);
    const auto straightmost_index = std::distance(intersection.begin(), straightmost);
    const auto straightmost_angle_dev = angularDeviation(straightmost->angle, STRAIGHT_ANGLE);

    // check whether the obvious choice is actually a through street
    if (obvious_index != 0)
    {
        intersection[obvious_index].instruction =
            getInstructionForObvious(intersection.size(),
                                     via_edge,
                                     isThroughStreet(obvious_index, intersection),
                                     intersection[obvious_index]);

        // assign left/right turns
        intersection = assignLeftTurns(via_edge, std::move(intersection), obvious_index);
        intersection = assignRightTurns(via_edge, std::move(intersection), obvious_index);
    }
    else if (fork) // found fork
    {
        if (fork->size == 2)
        {
            const auto left_classification =
                node_based_graph.GetEdgeData(fork->getLeft().eid).road_classification;
            const auto right_classification =
                node_based_graph.GetEdgeData(fork->getRight().eid).road_classification;
            if (canBeSeenAsFork(left_classification, right_classification))
            {
                assignFork(via_edge, fork->getLeft(), fork->getRight());
            }
            else if (left_classification.GetPriority() > right_classification.GetPriority())
            {
                fork->getRight().instruction = getInstructionForObvious(
                    intersection.size(), via_edge, false, fork->getRight());
                fork->getLeft().instruction = {findBasicTurnType(via_edge, fork->getLeft()),
                                               DirectionModifier::SlightLeft};
            }
            else
            {
                fork->getLeft().instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, fork->getLeft());
                fork->getRight().instruction = {findBasicTurnType(via_edge, fork->getRight()),
                                                DirectionModifier::SlightRight};
            }
        }
        else if (fork->size == 3)
        {
            assignFork(via_edge,
                       fork->getLeft(),
                       // middle fork road
                       fork->getMiddle(),
                       fork->getRight());
        }

        intersection = assignLeftTurns(via_edge, std::move(intersection), fork->getLeftIndex());
        intersection = assignRightTurns(via_edge, std::move(intersection), fork->getRightIndex());
    }
    else if (straightmost_angle_dev < FUZZY_ANGLE_DIFFERENCE && !straightmost->entry_allowed)
    {
        // invalid straight turn
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_index);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_index);
    }
    // no straight turn
    else if (straightmost->angle > 180)
    {
        // at most three turns on either side
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_index - 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_index);
    }
    else if (straightmost->angle < 180)
    {
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_index);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_index + 1);
    }
    else
    {
        assignTrivialTurns(via_edge, intersection, 1, intersection.size());
    }
    return intersection;
}

// Assignment of left turns hands of to right turns.
// To do so, we mirror every road segment and reverse the order.
// After the mirror and reversal / we assign right turns and
// mirror again and restore the original order.
Intersection TurnHandler::assignLeftTurns(const EdgeID via_edge,
                                          Intersection intersection,
                                          const std::size_t starting_at) const
{
    BOOST_ASSERT(starting_at < intersection.size());
    const auto switch_left_and_right = [](Intersection &intersection) {
        BOOST_ASSERT(!intersection.empty());

        for (auto &road : intersection)
            road.mirror();

        std::reverse(intersection.begin() + 1, intersection.end());
    };

    switch_left_and_right(intersection);
    // account for the u-turn in the beginning
    const auto count = intersection.size() - starting_at;
    intersection = assignRightTurns(via_edge, std::move(intersection), count);
    switch_left_and_right(intersection);

    return intersection;
}

// can only assign three turns
Intersection TurnHandler::assignRightTurns(const EdgeID via_edge,
                                           Intersection intersection,
                                           const std::size_t up_to) const
{
    BOOST_ASSERT(up_to <= intersection.size());
    const auto count_valid = [&intersection, up_to]() {
        std::size_t count = 0;
        for (std::size_t i = 1; i < up_to; ++i)
            if (intersection[i].entry_allowed)
                ++count;
        return count;
    };
    if (up_to <= 1 || count_valid() == 0)
        return intersection;
    // handle single turn
    if (up_to == 2)
    {
        assignTrivialTurns(via_edge, intersection, 1, up_to);
    }
    // Handle Turns 1-3
    else if (up_to == 3)
    {
        const auto first_direction = getTurnDirection(intersection[1].angle);
        const auto second_direction = getTurnDirection(intersection[2].angle);
        if (first_direction == second_direction)
        {
            // conflict
            handleDistinctConflict(via_edge, intersection[2], intersection[1]);
        }
        else
        {
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
    }
    // Handle Turns 1-4
    else if (up_to == 4)
    {
        const auto first_direction = getTurnDirection(intersection[1].angle);
        const auto second_direction = getTurnDirection(intersection[2].angle);
        const auto third_direction = getTurnDirection(intersection[3].angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            // due to the circular order, the turn directions are unique
            // first_direction != third_direction is implied
            BOOST_ASSERT(first_direction != third_direction);
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
        else if (2 >= (intersection[1].entry_allowed + intersection[2].entry_allowed +
                       intersection[3].entry_allowed))
        {
            // at least a single invalid
            if (!intersection[3].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[2], intersection[1]);
            }
            else if (!intersection[1].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[3], intersection[2]);
            }
            else // handles one-valid as well as two valid (1,3)
            {
                handleDistinctConflict(via_edge, intersection[3], intersection[1]);
            }
        }
        // From here on out, intersection[1-3].entry_allowed has to be true (Otherwise we would have
        // triggered 2>= ...)
        //
        // Conflicting Turns, but at least farther than what we call a narrow turn
        else if (angularDeviation(intersection[1].angle, intersection[2].angle) >=
                     NARROW_TURN_ANGLE &&
                 angularDeviation(intersection[2].angle, intersection[3].angle) >=
                     NARROW_TURN_ANGLE)
        {
            BOOST_ASSERT(intersection[1].entry_allowed && intersection[2].entry_allowed &&
                         intersection[3].entry_allowed);

            intersection[1].instruction = {findBasicTurnType(via_edge, intersection[1]),
                                           DirectionModifier::SharpRight};
            intersection[2].instruction = {findBasicTurnType(via_edge, intersection[2]),
                                           DirectionModifier::Right};
            intersection[3].instruction = {findBasicTurnType(via_edge, intersection[3]),
                                           DirectionModifier::SlightRight};
        }
        else if (((first_direction == second_direction && second_direction == third_direction) ||
                  (first_direction == second_direction &&
                   angularDeviation(intersection[2].angle, intersection[3].angle) < GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].angle, intersection[2].angle) < GROUP_ANGLE)))
        {
            BOOST_ASSERT(intersection[1].entry_allowed && intersection[2].entry_allowed &&
                         intersection[3].entry_allowed);
            // count backwards from the slightest turn
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
        else if (((first_direction == second_direction &&
                   angularDeviation(intersection[2].angle, intersection[3].angle) >= GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].angle, intersection[2].angle) >= GROUP_ANGLE)))
        {
            BOOST_ASSERT(intersection[1].entry_allowed && intersection[2].entry_allowed &&
                         intersection[3].entry_allowed);

            if (angularDeviation(intersection[2].angle, intersection[3].angle) >= GROUP_ANGLE)
            {
                handleDistinctConflict(via_edge, intersection[2], intersection[1]);
                intersection[3].instruction = {findBasicTurnType(via_edge, intersection[3]),
                                               third_direction};
            }
            else
            {
                intersection[1].instruction = {findBasicTurnType(via_edge, intersection[1]),
                                               first_direction};
                handleDistinctConflict(via_edge, intersection[3], intersection[2]);
            }
        }
        else
        {
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
    }
    else
    {
        assignTrivialTurns(via_edge, intersection, 1, up_to);
    }
    return intersection;
}

// finds a fork candidate by just looking at the geometry and angle of an intersection
boost::optional<TurnHandler::Fork>
TurnHandler::findForkCandidatesByGeometry(Intersection &intersection) const
{
    if (intersection.size() >= 3)
    {
        const auto straightmost = intersection.findClosestTurn(STRAIGHT_ANGLE);
        const auto straightmost_index = std::distance(intersection.begin(), straightmost);
        const auto straightmost_angle_dev = angularDeviation(straightmost->angle, STRAIGHT_ANGLE);

        // Forks can only happen when two or more roads have a pretty narrow angle between each
        // other and are close to going straight
        //
        //
        //     left   right          left   right
        //        \   /                 \ | /
        //         \ /                   \|/
        //          |                     |
        //          |                     |
        //          |                     |
        //
        //   possibly a fork         possibly a fork
        //
        //
        //           left             left
        //            /                 \ 
        //           /____ right         \ ______ right
        //          |                     |
        //          |                     |
        //          |                     |
        //
        //   not a fork cause      not a fork cause
        //    it's not going       angle is too wide
        //     straigthish
        //
        //
        // left and right will be indices of the leftmost and rightmost connected roads that are
        // fork candidates

        if (straightmost_angle_dev <= NARROW_TURN_ANGLE)
        {
            // find the rightmost road that might be part of a fork
            const auto right = findOutermostForkCandidate(
                intersection.rend() - straightmost_index - 1, intersection.rend());
            const std::size_t right_index = intersection.rend() - right - 1;
            const auto forward_right = intersection.begin() + right_index;
            // find the leftmost road that might be part of a fork
            const auto left = findOutermostForkCandidate(straightmost, intersection.end());

            // if the leftmost and rightmost roads with the conditions above are the same
            // or if there are more than three fork candidates
            // they cannot be fork candidates
            if (forward_right < left && left - forward_right < 3)
            {
                return Fork(intersection.begin(), forward_right, left + 1);
            }
        }
    }
    return boost::none;
}

// check if the fork candidates (all roads between left and right) and the
// incoming edge are compatible by class
bool TurnHandler::isCompatibleByRoadClass(const Intersection &intersection, const Fork fork) const
{
    const auto via_class = node_based_graph.GetEdgeData(intersection[0].eid).road_classification;

    // if any of the considered roads is a link road, it cannot be a fork
    // except if rightmost fork candidate is also a link road
    const auto is_right_link_class =
        node_based_graph.GetEdgeData(fork.getRight().eid).road_classification.IsLinkClass();
    if (!std::all_of(fork.begin + 1, fork.end, [&](ConnectedRoad &road) {
            return is_right_link_class ==
                   node_based_graph.GetEdgeData(road.eid).road_classification.IsLinkClass();
        }))
    {
        return false;
    }

    return std::all_of(fork.begin, fork.end, [&](ConnectedRoad &base) {
        const auto base_class = node_based_graph.GetEdgeData(base.eid).road_classification;
        // check that there is no turn obvious == check that all turns are non-onvious
        return std::all_of(fork.begin, fork.end, [&](ConnectedRoad &compare) {
            const auto compare_class =
                node_based_graph.GetEdgeData(compare.eid).road_classification;
            return compare.eid == base.eid ||
                   !(obviousByRoadClass(via_class, base_class, compare_class));
        });
    });
}

// Checks whether a three-way-intersection coming from `via_edge` is a fork
// with `intersection` as described as in #IntersectionExplanation@intersection_handler.hpp
boost::optional<TurnHandler::Fork> TurnHandler::findFork(const EdgeID via_edge,
                                                         Intersection &intersection) const
{
    const auto fork = findForkCandidatesByGeometry(intersection);
    if (fork)
    {
        // makes sure that the fork is isolated from other neighbouring streets on the left and
        // right side
        const auto next = fork->end == intersection.end() ? intersection.begin() : (fork->end);
        const bool separated_at_left_side =
            angularDeviation(fork->getLeft().angle, next->angle) >= GROUP_ANGLE;
        BOOST_ASSERT((fork->begin - 1) >= intersection.begin());
        const bool separated_at_right_side =
            angularDeviation(fork->getRight().angle, (fork->begin - 1)->angle) >= GROUP_ANGLE;

        // check whether there is an obvious turn to take; forks are never obvious - if there is an
        // obvious turn, it's not a fork
        const bool has_obvious = hasObvious(via_edge, *fork);

        // A fork can only happen between edges of similar types where none of the ones is obvious
        const bool has_compatible_classes = isCompatibleByRoadClass(intersection, *fork);

        // check if all entries in the fork range allow entry
        const bool only_valid_entries = intersection.hasAllValidEntries(fork->begin, fork->end);

        const auto has_compatible_modes =
            std::all_of(fork->begin, fork->end, [&](const auto &road) {
                return node_based_graph.GetEdgeData(road.eid).travel_mode ==
                       node_based_graph.GetEdgeData(via_edge).travel_mode;
            });

        if (separated_at_left_side && separated_at_right_side && !has_obvious &&
            has_compatible_classes && only_valid_entries && has_compatible_modes)
        {
            return fork;
        }
    }

    return boost::none;
}

void TurnHandler::handleDistinctConflict(const EdgeID via_edge,
                                         ConnectedRoad &left,
                                         ConnectedRoad &right) const
{
    // single turn of both is valid (don't change the valid one)
    // or multiple identical angles -> bad OSM intersection
    if ((!left.entry_allowed || !right.entry_allowed) || (left.angle == right.angle))
    {
        if (left.entry_allowed)
            left.instruction = {findBasicTurnType(via_edge, left), getTurnDirection(left.angle)};
        if (right.entry_allowed)
            right.instruction = {findBasicTurnType(via_edge, right), getTurnDirection(right.angle)};
        return;
    }

    if (getTurnDirection(left.angle) == DirectionModifier::Straight ||
        getTurnDirection(left.angle) == DirectionModifier::SlightLeft ||
        getTurnDirection(right.angle) == DirectionModifier::SlightRight)
    {
        const auto left_classification = node_based_graph.GetEdgeData(left.eid).road_classification;
        const auto right_classification =
            node_based_graph.GetEdgeData(right.eid).road_classification;
        if (canBeSeenAsFork(left_classification, right_classification))
            assignFork(via_edge, left, right);
        else if (left_classification.GetPriority() > right_classification.GetPriority())
        {
            // FIXME this should possibly know about the actual roads?
            // here we don't know about the intersection size. To be on the save side,
            // we declare it
            // as complex (at least size 4)
            right.instruction = getInstructionForObvious(4, via_edge, false, right);
            left.instruction = {findBasicTurnType(via_edge, left), DirectionModifier::SlightLeft};
        }
        else
        {
            // FIXME this should possibly know about the actual roads?
            // here we don't know about the intersection size. To be on the save side,
            // we declare it
            // as complex (at least size 4)
            left.instruction = getInstructionForObvious(4, via_edge, false, left);
            right.instruction = {findBasicTurnType(via_edge, right),
                                 DirectionModifier::SlightRight};
        }
    }
    const auto left_type = findBasicTurnType(via_edge, left);
    const auto right_type = findBasicTurnType(via_edge, right);
    // Two Right Turns
    if (angularDeviation(left.angle, 90) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep left perfect, shift right
        left.instruction = {left_type, DirectionModifier::Right};
        right.instruction = {right_type, DirectionModifier::SharpRight};
        return;
    }
    if (angularDeviation(right.angle, 90) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep Right perfect, shift left
        left.instruction = {left_type, DirectionModifier::SlightRight};
        right.instruction = {right_type, DirectionModifier::Right};
        return;
    }
    // Two Right Turns
    if (angularDeviation(left.angle, 270) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep left perfect, shift right
        left.instruction = {left_type, DirectionModifier::Left};
        right.instruction = {right_type, DirectionModifier::SlightLeft};
        return;
    }
    if (angularDeviation(right.angle, 270) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep Right perfect, shift left
        left.instruction = {left_type, DirectionModifier::SharpLeft};
        right.instruction = {right_type, DirectionModifier::Left};
        return;
    }
    // Shift the lesser penalty
    if (getTurnDirection(left.angle) == DirectionModifier::SharpLeft)
    {
        left.instruction = {left_type, DirectionModifier::SharpLeft};
        right.instruction = {right_type, DirectionModifier::Left};
        return;
    }
    if (getTurnDirection(right.angle) == DirectionModifier::SharpRight)
    {
        left.instruction = {left_type, DirectionModifier::Right};
        right.instruction = {right_type, DirectionModifier::SharpRight};
        return;
    }

    if (getTurnDirection(left.angle) == DirectionModifier::Right)
    {
        if (angularDeviation(left.angle, 85) >= angularDeviation(right.angle, 85))
        {
            left.instruction = {left_type, DirectionModifier::Right};
            right.instruction = {right_type, DirectionModifier::SharpRight};
        }
        else
        {
            left.instruction = {left_type, DirectionModifier::SlightRight};
            right.instruction = {right_type, DirectionModifier::Right};
        }
    }
    else
    {
        if (angularDeviation(left.angle, 265) >= angularDeviation(right.angle, 265))
        {
            left.instruction = {left_type, DirectionModifier::SharpLeft};
            right.instruction = {right_type, DirectionModifier::Left};
        }
        else
        {
            left.instruction = {left_type, DirectionModifier::Left};
            right.instruction = {right_type, DirectionModifier::SlightLeft};
        }
    }
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
