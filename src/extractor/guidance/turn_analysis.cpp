#include "extractor/guidance/turn_analysis.hpp"

#include "util/simple_logger.hpp"
#include "util/coordinate.hpp"

#include <cstddef>
#include <limits>
#include <iomanip>

namespace osrm
{
namespace extractor
{
namespace guidance
{
// configuration of turn classification
const bool constexpr INVERT = true;
const bool constexpr RESOLVE_TO_RIGHT = true;
const bool constexpr RESOLVE_TO_LEFT = false;

// what angle is interpreted as going straight
const double constexpr STRAIGHT_ANGLE = 180.;
// if a turn deviates this much from going straight, it will be kept straight
const double constexpr MAXIMAL_ALLOWED_NO_TURN_DEVIATION = 2.;
// angle that lies between two nearly indistinguishable roads
const double constexpr NARROW_TURN_ANGLE = 35.;
const double constexpr WELL_DISTINCT_ANGLE = 50;
// angle difference that can be classified as straight, if its the only narrow turn
const double constexpr FUZZY_ANGLE_DIFFERENCE = 15.;
const double constexpr DISTINCTION_RATIO = 2;
const unsigned constexpr INVALID_NAME_ID = 0;

using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

bool requiresAnnouncedment(const EdgeData &from, const EdgeData &to)
{
    return !from.IsCompatibleTo(to);
}

struct Localizer
{
    const std::vector<QueryNode> *node_info_list = nullptr;

    util::Coordinate operator()(const NodeID nid)
    {
        if (node_info_list)
        {
            return {(*node_info_list)[nid].lon, (*node_info_list)[nid].lat};
        }
        return {};
    }
};

static Localizer localizer;

#define PRINT_DEBUG_CANDIDATES 0
std::vector<TurnCandidate>
getTurns(const NodeID from,
         const EdgeID via_edge,
         const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph,
         const std::vector<QueryNode> &node_info_list,
         const std::shared_ptr<RestrictionMap const> restriction_map,
         const std::unordered_set<NodeID> &barrier_nodes,
         const CompressedEdgeContainer &compressed_edge_container)
{
    localizer.node_info_list = &node_info_list;
    auto turn_candidates =
        detail::getTurnCandidates(from, via_edge, node_based_graph, node_info_list, restriction_map,
                                  barrier_nodes, compressed_edge_container);

    const auto &in_edge_data = node_based_graph->GetEdgeData(via_edge);

    // main priority: roundabouts
    bool on_roundabout = in_edge_data.roundabout;
    bool can_enter_roundabout = false;
    bool can_exit_roundabout = false;
    for (const auto &candidate : turn_candidates)
    {
        if (node_based_graph->GetEdgeData(candidate.eid).roundabout)
        {
            can_enter_roundabout = true;
        }
        else
        {
            can_exit_roundabout = true;
        }
    }
    if (on_roundabout || can_enter_roundabout)
    {
        return detail::handleRoundabouts(from, via_edge, on_roundabout, can_enter_roundabout,
                                         can_exit_roundabout, std::move(turn_candidates),
                                         node_based_graph);
    }

    // set initial defaults for normal turns and modifier based on angle
    turn_candidates =
        detail::setTurnTypes(from, via_edge, std::move(turn_candidates), node_based_graph);

    if (detail::isMotorwayJunction(from, via_edge, turn_candidates, node_based_graph))
    {
        // std::cout << "Handling Motorway Junction at " << from << " (" << node_info_list[from].lat
        // << ", " << node_info_list[from].lon << ")" << " and " <<
        // node_info_list[node_based_graph->GetTarget(via_edge)].lat << " " <<
        // node_info_list[node_based_graph->GetTarget(via_edge)].lon << std::endl;
        return detail::handleMotorwayJunction(from, via_edge, std::move(turn_candidates),
                                              node_based_graph);
    }

    if (detail::isBasicJunction(from, via_edge, turn_candidates, node_based_graph) &&
        turn_candidates.size() <= 4) // TODO change when larger junctions are handled
    {
        if (turn_candidates.size() == 1)
        {
            turn_candidates = detail::handleOneWayTurn(from, via_edge, std::move(turn_candidates),
                                                       node_based_graph);
        }
        if (turn_candidates.size() == 2)
        {
            turn_candidates = detail::handleTwoWayTurn(from, via_edge, std::move(turn_candidates),
                                                       node_based_graph);
        }
        if (turn_candidates.size() == 3)
        {
            detail::handleThreeWayTurn(from, via_edge, std::move(turn_candidates),
                                       node_based_graph);
        }
        if (turn_candidates.size() == 4)
        {
            detail::handleFourWayTurn(from, via_edge, std::move(turn_candidates), node_based_graph);
        }
        else
        {
            turn_candidates = detail::handleComplexTurn(from, via_edge, std::move(turn_candidates),
                                                        node_based_graph);
        }
        // complex intersection, potentially requires conflict resolution
        return detail::handleConflicts(from, via_edge, std::move(turn_candidates),
                                       node_based_graph);
    }

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Initial Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    turn_candidates = detail::optimizeCandidates(via_edge, std::move(turn_candidates),
                                                 node_based_graph, node_info_list);
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Optimized Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << std::endl;
#endif
    turn_candidates = detail::suppressTurns(via_edge, std::move(turn_candidates), node_based_graph);
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Suppressed Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << std::endl;
#endif
    return turn_candidates;
}

namespace detail
{

inline unsigned countValid(const std::vector<TurnCandidate> &turn_candidates)
{
    unsigned count = 0;
    for (const auto &candidate : turn_candidates)
    {
        if (candidate.valid)
            ++count;
    }
    return count;
};

std::vector<TurnCandidate>
handleRoundabouts(const NodeID from,
                  const EdgeID via_edge,
                  const bool on_roundabout,
                  const bool can_enter_roundabout,
                  const bool can_exit_roundabout,
                  std::vector<TurnCandidate> turn_candidates,
                  const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    (void)from;
    // TODO requires differentiation between roundabouts and rotaries
    // detect via radius (get via circle through three vertices)
    NodeID node_v = node_based_graph->GetTarget(via_edge);
    if (on_roundabout)
    {
        // Shoule hopefully have only a single exit and continue
        // at least for cars. How about bikes?
        for (auto &candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
            if (out_data.roundabout)
            {
                // TODO can forks happen in roundabouts? E.g. required lane changes
                if (1 == node_based_graph->GetDirectedOutDegree(node_v))
                {
                    // No turn possible.
                    candidate.instruction = TurnInstruction::NO_TURN();
                }
                else
                {
                    candidate.instruction =
                        TurnInstruction::REMAIN_ROUNDABOUT(getTurnDirection(candidate.angle));
                }
            }
            else
            {
                candidate.instruction =
                    TurnInstruction::EXIT_ROUNDABOUT(getTurnDirection(candidate.angle));
            }
        }
#if PRINT_DEBUG_CANDIDATES
        std::cout << "On Roundabout Candidates:\n";
        for (auto tc : turn_candidates)
            std::cout << "\t" << tc.toString() << " "
                      << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                      << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
        return turn_candidates;
    }
    else
    {
        (void)can_enter_roundabout;
        BOOST_ASSERT(can_enter_roundabout);
        for (auto &candidate : turn_candidates)
        {
            if (!candidate.valid)
                continue;
            const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
            if (out_data.roundabout)
            {
                candidate.instruction =
                    TurnInstruction::ENTER_ROUNDABOUT(getTurnDirection(candidate.angle));
                if (can_exit_roundabout)
                {
                    if (candidate.instruction.type == TurnType::EnterRotary)
                        candidate.instruction.type = TurnType::EnterRotaryAtExit;
                    if (candidate.instruction.type == TurnType::EnterRoundabout)
                        candidate.instruction.type = TurnType::EnterRoundaboutAtExit;
                }
            }
            else
            {
                candidate.instruction = {TurnType::EnterAndExitRoundabout,
                                         getTurnDirection(candidate.angle)};
            }
        }
#if PRINT_DEBUG_CANDIDATES
        std::cout << "Into Roundabout Candidates:\n";
        for (auto tc : turn_candidates)
            std::cout << "\t" << tc.toString() << " "
                      << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                      << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
        return turn_candidates;
    }
}

inline bool isMotorwayClass(FunctionalRoadClass road_class)
{
    return road_class == FunctionalRoadClass::MOTORWAY || road_class == FunctionalRoadClass::TRUNK;
}

inline bool
isMotorwayClass(EdgeID eid,
                const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    return isMotorwayClass(node_based_graph->GetEdgeData(eid).road_classification.road_class);
}

inline bool isRampClass(EdgeID eid,
                        const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    return isRampClass(node_based_graph->GetEdgeData(eid).road_classification.road_class);
}

inline std::vector<TurnCandidate> fallbackTurnAssignmentMotorway(
    std::vector<TurnCandidate> turn_candidates,
    const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    for (auto &candidate : turn_candidates)
    {
        const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);

        util::SimpleLogger().Write(logWARNING)
            << "Candidate: " << candidate.toString() << " Name: " << out_data.name_id
            << " Road Class: " << (int)out_data.road_classification.road_class
            << " At: " << localizer(node_based_graph->GetTarget(candidate.eid));

        if (!candidate.valid)
            continue;

        const auto type = isMotorwayClass(out_data.road_classification.road_class) ? TurnType::Merge
                                                                                   : TurnType::Turn;
        if (angularDeviation(candidate.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
            candidate.instruction = {type, DirectionModifier::Straight};
        else
        {
            candidate.instruction = {type,
                                     candidate.angle > STRAIGHT_ANGLE
                                         ? DirectionModifier::SlightLeft
                                         : DirectionModifier::SlightRight};
        }
    }
    return turn_candidates;
}

std::vector<TurnCandidate>
handleFromMotorway(const NodeID from,
                   const EdgeID via_edge,
                   std::vector<TurnCandidate> turn_candidates,
                   const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    (void)from;
    const auto &in_data = node_based_graph->GetEdgeData(via_edge);
    BOOST_ASSERT(isMotorwayClass(in_data.road_classification.road_class));

    const auto countExitingMotorways =
        [node_based_graph](const std::vector<TurnCandidate> &turn_candidates)
    {
        unsigned count = 0;
        for (const auto &candidate : turn_candidates)
        {
            if (candidate.valid && isMotorwayClass(candidate.eid, node_based_graph))
                ++count;
        }
        return count;
    };

    // find the angle that continues on our current highway
    const auto getContinueAngle =
        [in_data, node_based_graph](const std::vector<TurnCandidate> &turn_candidates)
    {
        for (const auto &candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
            if (candidate.angle != 0 && in_data.name_id == out_data.name_id &&
                in_data.name_id != 0 && isMotorwayClass(out_data.road_classification.road_class))
                return candidate.angle;
        }
        return turn_candidates[0].angle;
    };

    const auto getMostLikelyContinue =
        [in_data, node_based_graph](const std::vector<TurnCandidate> &turn_candidates)
    {
        double angle = turn_candidates[0].angle;
        double best = 180;
        for (const auto &candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
            if (isMotorwayClass(out_data.road_classification.road_class) &&
                angularDeviation(candidate.angle, STRAIGHT_ANGLE) < best)
            {
                best = angularDeviation(candidate.angle, STRAIGHT_ANGLE);
                angle = candidate.angle;
            }
        }
        return angle;
    };

    const auto findBestContinue = [&]()
    {
        const double continue_angle = getContinueAngle(turn_candidates);
        if (continue_angle != turn_candidates[0].angle)
            return continue_angle;
        else
            return getMostLikelyContinue(turn_candidates);
    };

    // find continue angle
    const double continue_angle = findBestContinue();

    // highway does not continue and has no obvious choice
    if (continue_angle == turn_candidates[0].angle)
    {
        if (turn_candidates.size() == 2)
        {
            // do not announce ramps at the end of a highway
            turn_candidates[1].instruction = {TurnType::NoTurn,
                                              getTurnDirection(turn_candidates[1].angle)};
        }
        else if (turn_candidates.size() == 3)
        {
            // splitting ramp at the end of a highway
            if (turn_candidates[1].valid && turn_candidates[2].valid)
            {
                assignFork(via_edge, turn_candidates[2], turn_candidates[1], node_based_graph);
            }
            else
            {
                // ending in a passing ramp
                if (turn_candidates[1].valid)
                    turn_candidates[1].instruction = {TurnType::NoTurn,
                                                      getTurnDirection(turn_candidates[1].angle)};
                else
                    turn_candidates[2].instruction = {TurnType::NoTurn,
                                                      getTurnDirection(turn_candidates[2].angle)};
            }
        }
        else if (countValid(turn_candidates)) // check whether turns exist at all
        {
            // FALLBACK, this should hopefully never be reached
            auto coord = localizer(node_based_graph->GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Fallback reached from motorway at " << std::setprecision(12)
                << toFloating(coord.lat) << " " << toFloating(coord.lon) << ", no continue angle, "
                << turn_candidates.size() << " candidates, " << countValid(turn_candidates)
                << " valid ones.";
            fallbackTurnAssignmentMotorway(turn_candidates, node_based_graph);
        }
    }
    else
    {
        const unsigned exiting_motorways = countExitingMotorways(turn_candidates);

        if (exiting_motorways == 0)
        {
            // Ending in Ramp
            for (auto &candidate : turn_candidates)
            {
                if (candidate.valid)
                {
                    BOOST_ASSERT(isRampClass(candidate.eid, node_based_graph));
                    candidate.instruction =
                        TurnInstruction::SUPPRESSED(getTurnDirection(candidate.angle));
                }
            }
        }
        else if (exiting_motorways == 1)
        {
            // normal motorway passing some ramps or mering onto another motorway
            if (turn_candidates.size() == 2)
            {
                BOOST_ASSERT(!isRampClass(turn_candidates[1].eid, node_based_graph));

                turn_candidates[1].instruction =
                    noTurnOrNewName(from, via_edge, turn_candidates[1], node_based_graph);
            }
            else
            {
                // continue on the same highway
                bool continues = (getContinueAngle(turn_candidates) != turn_candidates[0].angle);
                // Normal Highway exit or merge
                for (auto &candidate : turn_candidates)
                {
                    // ignore invalid uturns/other
                    if (!candidate.valid)
                        continue;

                    if (candidate.angle == continue_angle)
                    {
                        if (continues)
                            candidate.instruction =
                                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
                        else // TODO handle turn direction correctly
                            candidate.instruction = {TurnType::Merge, DirectionModifier::Straight};
                    }
                    else if (candidate.angle < continue_angle)
                    {
                        candidate.instruction = {
                            isRampClass(candidate.eid, node_based_graph) ? TurnType::Ramp
                                                                         : TurnType::Turn,
                            (candidate.angle < 145) ? DirectionModifier::Right
                                                    : DirectionModifier::SlightRight};
                    }
                    else if (candidate.angle > continue_angle)
                    {
                        candidate.instruction = {
                            isRampClass(candidate.eid, node_based_graph) ? TurnType::Ramp
                                                                         : TurnType::Turn,
                            (candidate.angle > 215) ? DirectionModifier::Left
                                                    : DirectionModifier::SlightLeft};
                    }
                }
            }
        }
        // handle motorway forks
        else if (exiting_motorways > 1)
        {
            if (exiting_motorways == 2 && turn_candidates.size() == 2)
            {
                turn_candidates[1].instruction =
                    getInstructionForObvious(from, via_edge, turn_candidates[1], node_based_graph);
                util::SimpleLogger().Write(logWARNING)
                    << "Disabled U-Turn on a freeway at "
                    << localizer(node_based_graph->GetTarget(via_edge));
                turn_candidates[0].valid = false; // UTURN on the freeway
            }
            else if (exiting_motorways == 2)
            {
                // standard fork
                std::size_t first_valid = std::numeric_limits<std::size_t>::max(),
                            second_valid = std::numeric_limits<std::size_t>::max();
                for (std::size_t i = 0; i < turn_candidates.size(); ++i)
                {
                    if (turn_candidates[i].valid &&
                        isMotorwayClass(turn_candidates[i].eid, node_based_graph))
                    {
                        if (first_valid < turn_candidates.size())
                        {
                            second_valid = i;
                            break;
                        }
                        else
                        {
                            first_valid = i;
                        }
                    }
                }
                assignFork(via_edge, turn_candidates[second_valid], turn_candidates[first_valid],
                           node_based_graph);
            }
            else if (exiting_motorways == 3)
            {
                // triple fork
                std::size_t first_valid = std::numeric_limits<std::size_t>::max(),
                            second_valid = std::numeric_limits<std::size_t>::max(),
                            third_valid = std::numeric_limits<std::size_t>::max();
                for (std::size_t i = 0; i < turn_candidates.size(); ++i)
                {
                    if (turn_candidates[i].valid &&
                        isMotorwayClass(turn_candidates[i].eid, node_based_graph))
                    {
                        if (second_valid < turn_candidates.size())
                        {
                            third_valid = i;
                            break;
                        }
                        else if (first_valid < turn_candidates.size())
                        {
                            second_valid = i;
                        }
                        else
                        {
                            first_valid = i;
                        }
                    }
                }
                assignFork(via_edge, turn_candidates[third_valid], turn_candidates[second_valid],
                           turn_candidates[first_valid], node_based_graph);
            }
            else
            {
                auto coord = localizer(node_based_graph->GetTarget(via_edge));
                util::SimpleLogger().Write(logWARNING)
                    << "Found motorway junction with more than "
                       "2 exiting motorways or additional ramps at " << std::setprecision(12)
                    << toFloating(coord.lat) << " " << toFloating(coord.lon);
                fallbackTurnAssignmentMotorway(turn_candidates, node_based_graph);
            }
        } // done for more than one highway exit
    }

#if PRINT_DEBUG_CANDIDATES
    std::cout << "From Motorway Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
handleMotorwayRamp(const NodeID from,
                   const EdgeID via_edge,
                   std::vector<TurnCandidate> turn_candidates,
                   const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    unsigned num_valid_turns = countValid(turn_candidates);
    // ramp straight into a motorway/ramp
    if (turn_candidates.size() == 2 && num_valid_turns == 1)
    {
        BOOST_ASSERT(!turn_candidates[0].valid);
        BOOST_ASSERT(isMotorwayClass(turn_candidates[1].eid, node_based_graph));

        turn_candidates[1].instruction =
            noTurnOrNewName(from, via_edge, turn_candidates[1], node_based_graph);
    }
    else if (turn_candidates.size() == 3)
    {
        // merging onto a passing highway / or two ramps merging onto the same highway
        if (num_valid_turns == 1)
        {
            BOOST_ASSERT(!turn_candidates[0].valid);
            // check order of highways
            //          4
            //     5         3
            //
            //   6              2
            //
            //     7         1
            //          0
            if (turn_candidates[1].valid)
            {
                if (isMotorwayClass(turn_candidates[1].eid, node_based_graph))
                {
                    // circular order indicates a merge to the left (0-3 onto 4
                    if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) <
                        NARROW_TURN_ANGLE)
                        turn_candidates[1].instruction = {TurnType::Merge,
                                                          DirectionModifier::SlightLeft};
                    else // fallback
                        turn_candidates[1].instruction = {
                            TurnType::Merge, getTurnDirection(turn_candidates[1].angle)};
                }
                else // passing by the end of a motorway
                    turn_candidates[1].instruction =
                        noTurnOrNewName(from, via_edge, turn_candidates[1], node_based_graph);
            }
            else
            {
                BOOST_ASSERT(turn_candidates[2].valid);
                if (isMotorwayClass(turn_candidates[2].eid, node_based_graph))
                {
                    // circular order (5-0) onto 4
                    if (angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) <
                        NARROW_TURN_ANGLE)
                        turn_candidates[2].instruction = {TurnType::Merge,
                                                          DirectionModifier::SlightRight};
                    else // fallback
                        turn_candidates[2].instruction = {
                            TurnType::Merge, getTurnDirection(turn_candidates[2].angle)};
                }
                else // passing the end of a highway
                    turn_candidates[1].instruction =
                        noTurnOrNewName(from, via_edge, turn_candidates[1], node_based_graph);
            }
        }
        else
        {
            BOOST_ASSERT(num_valid_turns == 2);
            // UTurn on ramps is not possible
            BOOST_ASSERT(!turn_candidates[0].valid);
            BOOST_ASSERT(turn_candidates[1].valid);
            BOOST_ASSERT(turn_candidates[2].valid);
            // two motorways starting at end of ramp (fork)
            //  M       M
            //    \   /
            //      |
            //      R
            if (isMotorwayClass(turn_candidates[1].eid, node_based_graph) &&
                isMotorwayClass(turn_candidates[2].eid, node_based_graph))
            {
                assignFork(via_edge, turn_candidates[2], turn_candidates[1], node_based_graph);
            }
            else
            {
                // continued ramp passing motorway entry
                //      M  R
                //      M  R
                //      | /
                //      R
                if (isMotorwayClass(node_based_graph->GetEdgeData(turn_candidates[1].eid)
                                        .road_classification.road_class))
                {
                    turn_candidates[1].instruction = {TurnType::Merge,
                                                      DirectionModifier::SlightRight};
                    turn_candidates[2].instruction = {TurnType::Fork,
                                                      DirectionModifier::SlightLeft};
                }
                else
                {
                    turn_candidates[1].instruction = {TurnType::Fork,
                                                      DirectionModifier::SlightRight};
                    turn_candidates[2].instruction = {TurnType::Merge,
                                                      DirectionModifier::SlightLeft};
                }
            }
        }
    }
    // On - Off Ramp on passing Motorway, Ramp onto Fork(?)
    else if (turn_candidates.size() == 4)
    {
        bool passed_highway_entry = false;
        for (auto &candidate : turn_candidates)
        {
            const auto &edge_data = node_based_graph->GetEdgeData(candidate.eid);
            if (!candidate.valid && isMotorwayClass(edge_data.road_classification.road_class))
            {
                passed_highway_entry = true;
            }
            else if (isMotorwayClass(edge_data.road_classification.road_class))
            {
                candidate.instruction = {TurnType::Merge,
                                         passed_highway_entry ? DirectionModifier::SlightRight
                                                              : DirectionModifier::SlightLeft};
            }
            else
            {
                BOOST_ASSERT(isRampClass(edge_data.road_classification.road_class));
                candidate.instruction = {TurnType::Ramp, getTurnDirection(candidate.angle)};
            }
        }
    }
    else
    { // FALLBACK, hopefully this should never been reached
        util::SimpleLogger().Write(logWARNING) << "Reached fallback on motorway ramp with "
                                               << turn_candidates.size() << " candidates and "
                                               << countValid(turn_candidates) << " valid turns.";
        fallbackTurnAssignmentMotorway(turn_candidates, node_based_graph);
    }

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Onto Motorway Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
handleMotorwayJunction(const NodeID from,
                       const EdgeID via_edge,
                       std::vector<TurnCandidate> turn_candidates,
                       const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    (void)from;
    // BOOST_ASSERT(!turn_candidates[0].valid); //This fails due to @themarex handling of dead end
    // streets
    const auto &in_data = node_based_graph->GetEdgeData(via_edge);

    // coming from motorway
    if (isMotorwayClass(in_data.road_classification.road_class))
    {
        return handleFromMotorway(from, via_edge, std::move(turn_candidates), node_based_graph);
    }
    else // coming from a ramp
    {
        return handleMotorwayRamp(from, via_edge, std::move(turn_candidates), node_based_graph);
        // ramp merging straight onto motorway
    }
}

bool isBasicJunction(const NodeID from,
                     const EdgeID via_edge,
                     const std::vector<TurnCandidate> &turn_candidates,
                     const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    (void)from, (void)turn_candidates;

    for (const auto &candidate : turn_candidates)
    {
        const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);

        if (out_data.road_classification.road_class == FunctionalRoadClass::MOTORWAY ||
            out_data.road_classification.road_class == FunctionalRoadClass::TRUNK)
            return false;
    }

    const auto &in_data = node_based_graph->GetEdgeData(via_edge);
    return in_data.road_classification.road_class != FunctionalRoadClass::MOTORWAY &&
           in_data.road_classification.road_class != FunctionalRoadClass::TRUNK;
    /*
    bool on_ramp = false;
    if (isRampClass(in_data.road_classification.road_class))
        on_ramp = true;

    std::size_t ramp_count = 0;
    for (const auto &candidate : turn_candidates)
    {
        const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
        if (isRampClass(out_data.road_classification.road_class))
            ramp_count++;
    }

    return (on_ramp && ramp_count == turn_candidates.size()) || (!on_ramp && ramp_count == 0);
    */
}

bool isMotorwayJunction(const NodeID from,
                        const EdgeID via_edge,
                        const std::vector<TurnCandidate> &turn_candidates,
                        const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    (void)from;

    bool has_motorway = false;
    bool has_normal_roads = false;

    for (const auto &candidate : turn_candidates)
    {
        const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
        // not merging or forking?
        if ((angularDeviation(candidate.angle, 0) > 35 &&
             angularDeviation(candidate.angle, 180) > 35) ||
            (candidate.valid && angularDeviation(candidate.angle, 0) < 35))
            return false;
        else if (out_data.road_classification.road_class == FunctionalRoadClass::MOTORWAY ||
                 out_data.road_classification.road_class == FunctionalRoadClass::TRUNK)
        {
            if (candidate.valid)
                has_motorway = true;
        }
        else if (!isRampClass(out_data.road_classification.road_class))
            has_normal_roads = true;
    }

    if (has_normal_roads)
        return false;

    const auto &in_data = node_based_graph->GetEdgeData(via_edge);
    return has_motorway ||
           in_data.road_classification.road_class == FunctionalRoadClass::MOTORWAY ||
           in_data.road_classification.road_class == FunctionalRoadClass::TRUNK;
}

TurnType
findBasicTurnType(const NodeID from,
                  const EdgeID via_edge,
                  const TurnCandidate &candidate,
                  const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    const auto &in_data = node_based_graph->GetEdgeData(via_edge);
    const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);

    bool on_ramp = isRampClass(in_data.road_classification.road_class);

    bool onto_ramp = isRampClass(out_data.road_classification.road_class);

    if (!onto_ramp && onto_ramp)
        return TurnType::Ramp;

    if (in_data.name_id == out_data.name_id && in_data.name_id != INVALID_NAME_ID)
    {
        return TurnType::Continue;
    }

    return TurnType::Turn;
}

TurnInstruction
noTurnOrNewName(const NodeID from,
                const EdgeID via_edge,
                const TurnCandidate &candidate,
                const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{

    (void)from;
    const auto &in_data = node_based_graph->GetEdgeData(via_edge);
    const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
    if (in_data.name_id == out_data.name_id)
    {
        if (angularDeviation(candidate.angle, 0) > 0.01)
            return TurnInstruction::SUPPRESSED(getTurnDirection(candidate.angle));

        return {TurnType::Turn, DirectionModifier::UTurn};
    }
    else
    {
        return {TurnType::NewName, getTurnDirection(candidate.angle)};
    }
}

TurnInstruction
getInstructionForObvious(const NodeID from,
                         const EdgeID via_edge,
                         const TurnCandidate &candidate,
                         const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{

    if (findBasicTurnType(from, via_edge, candidate, node_based_graph) == TurnType::Turn)
    {
        return noTurnOrNewName(from, via_edge, candidate, node_based_graph);
    }
    else
    {
        return {TurnType::Ramp, getTurnDirection(candidate.angle)};
    }
}

std::vector<TurnCandidate>
handleOneWayTurn(const NodeID from,
                 const EdgeID via_edge,
                 std::vector<TurnCandidate> turn_candidates,
                 const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    BOOST_ASSERT(turn_candidates[0].angle < 0.001);
    (void)from, (void)via_edge, (void)node_based_graph;
    if (!turn_candidates[0].valid)
    {
        util::SimpleLogger().Write(logWARNING)
            << "Graph Broken. Dead end without exit found or missing reverse edge.";
    }

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic (one) Turn Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
handleTwoWayTurn(const NodeID from,
                 const EdgeID via_edge,
                 std::vector<TurnCandidate> turn_candidates,
                 const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    BOOST_ASSERT(turn_candidates[0].angle < 0.001);

    turn_candidates[1].instruction =
        getInstructionForObvious(from, via_edge, turn_candidates[1], node_based_graph);

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic Two Turns Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
handleThreeWayTurn(const NodeID from,
                   const EdgeID via_edge,
                   std::vector<TurnCandidate> turn_candidates,
                   const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    BOOST_ASSERT(turn_candidates[0].angle < 0.001);
    const auto isObviousOfTwo = [](const TurnCandidate turn, const TurnCandidate other)
    {
        return (angularDeviation(turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
                angularDeviation(other.angle, STRAIGHT_ANGLE) > 85) ||
               (angularDeviation(other.angle, STRAIGHT_ANGLE) /
                    angularDeviation(turn.angle, STRAIGHT_ANGLE) >
                1.4);
    };
    // Two nearly straight turns -> FORK
    //          OOOOOOO
    //        /
    // IIIIII
    //        \
    //          OOOOOOO
    if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
        angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
    {
        if (turn_candidates[1].valid && turn_candidates[2].valid)
        {
            if (TurnType::Turn ==
                findBasicTurnType(from, via_edge, turn_candidates[1], node_based_graph))
            {
                if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) <
                    MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
                {
                    turn_candidates[1].instruction = getInstructionForObvious(
                        from, via_edge, turn_candidates[1], node_based_graph);
                    if (turn_candidates[1].instruction.type == TurnType::Turn)
                        turn_candidates[1].instruction = {TurnType::Fork,
                                                          DirectionModifier::SlightRight};
                }
                else
                    turn_candidates[1].instruction = {TurnType::Fork,
                                                      DirectionModifier::SlightRight};
            }
            else
                turn_candidates[1].instruction = {TurnType::Ramp, DirectionModifier::SlightRight};

            if (TurnType::Turn ==
                findBasicTurnType(from, via_edge, turn_candidates[2], node_based_graph))
            {
                if (angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) <
                    MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
                {
                    turn_candidates[2].instruction = getInstructionForObvious(
                        from, via_edge, turn_candidates[2], node_based_graph);
                    if (turn_candidates[2].instruction.type == TurnType::Turn)
                        turn_candidates[2].instruction = {TurnType::Fork,
                                                          DirectionModifier::SlightRight};
                }
                else

                    turn_candidates[2].instruction = {TurnType::Fork,
                                                      DirectionModifier::SlightLeft};
            }
            else
                turn_candidates[2].instruction = {TurnType::Ramp, DirectionModifier::SlightLeft};
        }
        else
        {
            if (turn_candidates[1].valid)
                turn_candidates[1].instruction =
                    getInstructionForObvious(from, via_edge, turn_candidates[1], node_based_graph);
            if (turn_candidates[2].valid)
                turn_candidates[2].instruction =
                    getInstructionForObvious(from, via_edge, turn_candidates[2], node_based_graph);
        }
    }
    //  T Intersection
    //
    //  OOOOOOO T OOOOOOOO
    //          I
    //          I
    //          I
    else if (angularDeviation(turn_candidates[1].angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[2].angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >
                 NARROW_TURN_ANGLE)
    {
        if (turn_candidates[1].valid)
        {
            if (TurnType::Ramp !=
                findBasicTurnType(from, via_edge, turn_candidates[1], node_based_graph))
                turn_candidates[1].instruction = {TurnType::EndOfRoad, DirectionModifier::Right};
            else
                turn_candidates[1].instruction = {TurnType::Ramp, DirectionModifier::Right};
        }
        if (turn_candidates[2].valid)
        {
            if (TurnType::Ramp !=
                findBasicTurnType(from, via_edge, turn_candidates[2], node_based_graph))
                turn_candidates[2].instruction = {TurnType::EndOfRoad, DirectionModifier::Left};
            else
                turn_candidates[2].instruction = {TurnType::Ramp, DirectionModifier::Left};
        }
    }
    // T Intersection, Cross left
    //          O
    //          O
    //          O
    // IIIIIIII - OOOOOOOOOO
    else if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[2].angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >
                 NARROW_TURN_ANGLE)
    {
        if (turn_candidates[1].valid)
        {
            if (TurnType::Ramp !=
                findBasicTurnType(from, via_edge, turn_candidates[1], node_based_graph))
                turn_candidates[1].instruction =
                    getInstructionForObvious(from, via_edge, turn_candidates[1], node_based_graph);
            else
                turn_candidates[1].instruction = {TurnType::Ramp, DirectionModifier::Straight};
        }
        if (turn_candidates[2].valid)
        {
            turn_candidates[2].instruction = {
                findBasicTurnType(from, via_edge, turn_candidates[2], node_based_graph),
                DirectionModifier::Left};
        }
    }
    // T Intersection, Cross right
    //
    // IIIIIIII T OOOOOOOOOO
    //          O
    //          O
    //          O
    else if (angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >
                 NARROW_TURN_ANGLE)
    {
        if (turn_candidates[2].valid)
            turn_candidates[2].instruction =
                getInstructionForObvious(from, via_edge, turn_candidates[2], node_based_graph);
        if (turn_candidates[1].valid)
            turn_candidates[1].instruction = {
                findBasicTurnType(from, via_edge, turn_candidates[1], node_based_graph),
                DirectionModifier::Right};
    }
    // merge onto a through street
    else if (INVALID_NAME_ID != node_based_graph->GetEdgeData(turn_candidates[1].eid).name_id &&
             node_based_graph->GetEdgeData(turn_candidates[1].eid).name_id ==
                 node_based_graph->GetEdgeData(turn_candidates[2].eid).name_id)
    {
        const auto findTurn = [isObviousOfTwo](const TurnCandidate turn, const TurnCandidate other)
                                  -> TurnInstruction
        {
            return {isObviousOfTwo(turn, other) ? TurnType::Merge : TurnType::Turn,
                    getTurnDirection(turn.angle)};
        };
        turn_candidates[1].instruction = findTurn(turn_candidates[1], turn_candidates[2]);
        turn_candidates[2].instruction = findTurn(turn_candidates[2], turn_candidates[1]);
    }

    // other street merges from the left
    else if (INVALID_NAME_ID != node_based_graph->GetEdgeData(via_edge).name_id &&
             node_based_graph->GetEdgeData(via_edge).name_id ==
                 node_based_graph->GetEdgeData(turn_candidates[1].eid).name_id)
    {
        if (isObviousOfTwo(turn_candidates[1], turn_candidates[2]))
        {
            turn_candidates[1].instruction =
                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
        }
        else
        {
            turn_candidates[1].instruction = {TurnType::Continue,
                                              getTurnDirection(turn_candidates[1].angle)};
        }
        turn_candidates[2].instruction = {TurnType::Turn,
                                          getTurnDirection(turn_candidates[2].angle)};
    }
    // other street merges from the right
    else if (INVALID_NAME_ID != node_based_graph->GetEdgeData(via_edge).name_id &&
             node_based_graph->GetEdgeData(via_edge).name_id ==
                 node_based_graph->GetEdgeData(turn_candidates[2].eid).name_id)
    {
        if (isObviousOfTwo(turn_candidates[2], turn_candidates[1]))
        {
            turn_candidates[2].instruction =
                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
        }
        else
        {
            turn_candidates[2].instruction = {TurnType::Continue,
                                              getTurnDirection(turn_candidates[2].angle)};
        }
        turn_candidates[1].instruction = {TurnType::Turn,
                                          getTurnDirection(turn_candidates[1].angle)};
    }
    else
    {
        const unsigned in_name_id = node_based_graph->GetEdgeData(via_edge).name_id;
        const unsigned out_names[2] = {
            node_based_graph->GetEdgeData(turn_candidates[1].eid).name_id,
            node_based_graph->GetEdgeData(turn_candidates[2].eid).name_id};
        if (isObviousOfTwo(turn_candidates[1], turn_candidates[2]))
        {
            turn_candidates[1].instruction = {
                (in_name_id != INVALID_NAME_ID || out_names[0] != INVALID_NAME_ID)
                    ? TurnType::NewName
                    : TurnType::NoTurn,
                getTurnDirection(turn_candidates[1].angle)};
        }
        else
        {
            turn_candidates[1].instruction = {TurnType::Turn,
                                              getTurnDirection(turn_candidates[1].angle)};
        }

        if (isObviousOfTwo(turn_candidates[2], turn_candidates[1]))
        {
            turn_candidates[2].instruction = {
                (in_name_id != INVALID_NAME_ID || out_names[1] != INVALID_NAME_ID)
                    ? TurnType::NewName
                    : TurnType::NoTurn,
                getTurnDirection(turn_candidates[2].angle)};
        }
        else
        {
            turn_candidates[2].instruction = {TurnType::Turn,
                                              getTurnDirection(turn_candidates[2].angle)};
        }
    }
// unnamed intersections or basic three way turn

// remain at basic turns
// TODO handle obviousness, Handle Merges

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic Turn Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
handleFourWayTurn(const NodeID from,
                  const EdgeID via_edge,
                  std::vector<TurnCandidate> turn_candidates,
                  const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    static int fallback_count = 0;
    // basic turn, or slightly rotated basic turn, has straight ANGLE
    if (angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE &&
        angularDeviation(turn_candidates[0].angle, turn_candidates[1].angle) > NARROW_TURN_ANGLE &&
        angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) > NARROW_TURN_ANGLE &&
        angularDeviation(turn_candidates[2].angle, turn_candidates[3].angle) > NARROW_TURN_ANGLE &&
        angularDeviation(turn_candidates[3].angle, turn_candidates[0].angle) > NARROW_TURN_ANGLE)
    {
        { // Right
            const auto type =
                findBasicTurnType(from, via_edge, turn_candidates[1], node_based_graph);
            turn_candidates[1].instruction = {type, DirectionModifier::Right};
        }
        { // Straight
            turn_candidates[2].instruction =
                getInstructionForObvious(from, via_edge, turn_candidates[2], node_based_graph);
        }
        { // Left
            const auto type =
                findBasicTurnType(from, via_edge, turn_candidates[3], node_based_graph);
            turn_candidates[3].instruction = {type, DirectionModifier::Left};
        }
    }
    // well differentiated turns
    else if (angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >
                 WELL_DISTINCT_ANGLE &&
             angularDeviation(turn_candidates[2].angle, turn_candidates[3].angle) >
                 WELL_DISTINCT_ANGLE)
    {
        for (std::size_t i = 1; i < turn_candidates.size(); ++i)
        {
            const auto type =
                findBasicTurnType(from, via_edge, turn_candidates[i], node_based_graph);
            turn_candidates[i].instruction = {type, getTurnDirection(turn_candidates[i].angle)};
        }
    }
    //  *     *
    //  *   *
    //  * *
    //  * *
    //  *   *
    //  *     *
    // Two roads at the right side of a street
    else if ( false && angularDeviation(turn_candidates[3].angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
    {
        //currently unhandled
    }
    //  *     *
    //    *   *
    //      * *
    //      * *
    //    *   *
    //  *     *
    // Two roads at the left side of a street
    else if ( false && angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
    {
        //currently unhandled
    }
    else
    {
        if (fallback_count++ < 10)
        {
            const auto coord = localizer(node_based_graph->GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Resolved to keep fallback on four way turn assignment at "
                << std::setprecision(12) << toFloating(coord.lat) << " " << toFloating(coord.lon);
            for (const auto &candidate : turn_candidates)
            {
                const auto &out_data = node_based_graph->GetEdgeData(candidate.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "Candidate: " << candidate.toString() << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class
                    << " At: " << localizer(node_based_graph->GetTarget(candidate.eid));
            }
        }
    }
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic Turn Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
handleComplexTurn(const NodeID from,
                  const EdgeID via_edge,
                  std::vector<TurnCandidate> turn_candidates,
                  const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic Turn Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph->GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
setTurnTypes(const NodeID from,
             const EdgeID via_edge,
             std::vector<TurnCandidate> turn_candidates,
             const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    NodeID turn_node = node_based_graph->GetTarget(via_edge);

    for (auto &candidate : turn_candidates)
    {
        if (!candidate.valid)
            continue;
        const EdgeID onto_edge = candidate.eid;
        const NodeID to_node = node_based_graph->GetTarget(onto_edge);

        auto turn = AnalyzeTurn(from, via_edge, turn_node, onto_edge, to_node, candidate.angle,
                                node_based_graph);

        auto confidence = getTurnConfidence(candidate.angle, turn);
        candidate.instruction = turn;
        candidate.confidence = confidence;
    }
    return turn_candidates;
}

std::vector<TurnCandidate>
optimizeRamps(const EdgeID via_edge,
              std::vector<TurnCandidate> turn_candidates,
              const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    EdgeID continue_eid = SPECIAL_EDGEID;
    double continue_angle = 0;
    const auto &in_edge_data = node_based_graph->GetEdgeData(via_edge);
    for (auto &candidate : turn_candidates)
    {
        if (candidate.instruction.direction_modifier == DirectionModifier::UTurn)
            continue;

        const auto &out_edge_data = node_based_graph->GetEdgeData(candidate.eid);
        if (out_edge_data.name_id == in_edge_data.name_id)
        {
            continue_eid = candidate.eid;
            continue_angle = candidate.angle;
            if (angularDeviation(candidate.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
                isRampClass(in_edge_data.road_classification.road_class))
                candidate.instruction = TurnType::Suppressed;
            break;
        }
    }

    if (continue_eid != SPECIAL_EDGEID)
    {
        bool to_the_right = true;
        for (auto &candidate : turn_candidates)
        {
            if (candidate.eid == continue_eid)
            {
                to_the_right = false;
                continue;
            }

            if (candidate.instruction.type != TurnType::Ramp)
                continue;

            if (isSlightModifier(candidate.instruction.direction_modifier))
                candidate.instruction.direction_modifier =
                    (to_the_right) ? DirectionModifier::SlightRight : DirectionModifier::SlightLeft;
        }
    }
    return turn_candidates;
}

// requires sorted candidates
std::vector<TurnCandidate>
optimizeCandidates(const EdgeID via_eid,
                   std::vector<TurnCandidate> turn_candidates,
                   const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph,
                   const std::vector<QueryNode> &node_info_list)
{
    BOOST_ASSERT_MSG(std::is_sorted(turn_candidates.begin(), turn_candidates.end(),
                                    [](const TurnCandidate &left, const TurnCandidate &right)
                                    {
                                        return left.angle < right.angle;
                                    }),
                     "Turn Candidates not sorted by angle.");
    if (turn_candidates.size() <= 1)
        return turn_candidates;

    turn_candidates = optimizeRamps(via_eid, std::move(turn_candidates), node_based_graph);

    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    // handle availability of multiple u-turns (e.g. street with separated small parking roads)
    if (isUturn(turn_candidates[0].instruction) && turn_candidates[0].angle == 0)
    {
        if (isUturn(turn_candidates[getLeft(0)].instruction))
            turn_candidates[getLeft(0)].instruction.direction_modifier =
                DirectionModifier::SharpLeft;
        if (isUturn(turn_candidates[getRight(0)].instruction))
            turn_candidates[getRight(0)].instruction.direction_modifier =
                DirectionModifier::SharpRight;
    }

    const auto keepStraight = [](double angle)
    {
        return std::abs(angle - 180) < 5;
    };

    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        auto &turn = turn_candidates[turn_index];
        if (!isBasic(turn.instruction.type) || isUturn(turn.instruction))
            continue;
        auto &left = turn_candidates[getLeft(turn_index)];
        if (turn.angle == left.angle)
        {
            util::SimpleLogger().Write(logWARNING)
                << "[warning] conflicting turn angles, identical road duplicated? "
                << std::setprecision(12) << node_info_list[node_based_graph->GetTarget(via_eid)].lat
                << " " << node_info_list[node_based_graph->GetTarget(via_eid)].lon << std::endl;
        }
        if (isConflict(turn.instruction, left.instruction))
        {
            // begin of a conflicting region
            std::size_t conflict_begin = turn_index;
            std::size_t conflict_end = getLeft(turn_index);
            std::size_t conflict_size = 2;
            while (
                isConflict(turn_candidates[getLeft(conflict_end)].instruction, turn.instruction) &&
                conflict_size < turn_candidates.size())
            {
                conflict_end = getLeft(conflict_end);
                ++conflict_size;
            }

            turn_index = (conflict_end < conflict_begin) ? turn_candidates.size() : conflict_end;

            if (conflict_size > 3)
            {
                // check if some turns are invalid to find out about good handling
            }

            auto &instruction_left_of_end = turn_candidates[getLeft(conflict_end)].instruction;
            auto &instruction_right_of_begin =
                turn_candidates[getRight(conflict_begin)].instruction;
            auto &candidate_at_end = turn_candidates[conflict_end];
            auto &candidate_at_begin = turn_candidates[conflict_begin];
            if (conflict_size == 2)
            {
                if (turn.instruction.direction_modifier == DirectionModifier::Straight)
                {
                    if (instruction_left_of_end.direction_modifier !=
                            DirectionModifier::SlightLeft &&
                        instruction_right_of_begin.direction_modifier !=
                            DirectionModifier::SlightRight)
                    {
                        std::int32_t resolved_count = 0;
                        // uses side-effects in resolve
                        if (!keepStraight(candidate_at_end.angle) &&
                            !resolve(candidate_at_end.instruction, instruction_left_of_end,
                                     RESOLVE_TO_LEFT))
                            util::SimpleLogger().Write(logDEBUG)
                                << "[warning] failed to resolve conflict";
                        else
                            ++resolved_count;
                        // uses side-effects in resolve
                        if (!keepStraight(candidate_at_begin.angle) &&
                            !resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                     RESOLVE_TO_RIGHT))
                            util::SimpleLogger().Write(logDEBUG)
                                << "[warning] failed to resolve conflict";
                        else
                            ++resolved_count;
                        if (resolved_count >= 1 &&
                            (!keepStraight(candidate_at_begin.angle) ||
                             !keepStraight(candidate_at_end.angle))) // should always be the
                                                                     // case, theoretically
                            continue;
                    }
                }
                if (candidate_at_begin.confidence < candidate_at_end.confidence)
                { // if right shift is cheaper, or only option
                    if (resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                RESOLVE_TO_RIGHT))
                        continue;
                    else if (resolve(candidate_at_end.instruction, instruction_left_of_end,
                                     RESOLVE_TO_LEFT))
                        continue;
                }
                else
                {
                    if (resolve(candidate_at_end.instruction, instruction_left_of_end,
                                RESOLVE_TO_LEFT))
                        continue;
                    else if (resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                     RESOLVE_TO_RIGHT))
                        continue;
                }
                if (isSlightTurn(turn.instruction) || isSharpTurn(turn.instruction))
                {
                    auto resolve_direction =
                        (turn.instruction.direction_modifier == DirectionModifier::SlightRight ||
                         turn.instruction.direction_modifier == DirectionModifier::SharpLeft)
                            ? RESOLVE_TO_RIGHT
                            : RESOLVE_TO_LEFT;
                    if (resolve_direction == RESOLVE_TO_RIGHT &&
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT))
                        continue;
                    else if (resolve_direction == RESOLVE_TO_LEFT &&
                             resolveTransitive(
                                 candidate_at_end.instruction, instruction_left_of_end,
                                 turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                                 RESOLVE_TO_LEFT))
                        continue;
                }
            }
            else if (conflict_size >= 3)
            {
                // a conflict of size larger than three cannot be handled with the current
                // model.
                // Handle it as best as possible and keep the rest of the conflicting turns
                if (conflict_size > 3)
                {
                    NodeID conflict_location = node_based_graph->GetTarget(via_eid);
                    util::SimpleLogger().Write(logDEBUG)
                        << "[warning] found conflict larget than size three at "
                        << node_info_list[conflict_location].lat << ", "
                        << node_info_list[conflict_location].lon;
                }

                if (!resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                             RESOLVE_TO_RIGHT))
                {
                    if (isSlightTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT);
                    else if (isSharpTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_end.instruction, instruction_left_of_end,
                            turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                            RESOLVE_TO_LEFT);
                }
                if (!resolve(candidate_at_end.instruction, instruction_left_of_end,
                             RESOLVE_TO_LEFT))
                {
                    if (isSlightTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_end.instruction, instruction_left_of_end,
                            turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                            RESOLVE_TO_LEFT);
                    else if (isSharpTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT);
                }
            }
        }
    }
    return turn_candidates;
}

bool isObviousChoice(const EdgeID via_eid,
                     const std::size_t turn_index,
                     const std::vector<TurnCandidate> &turn_candidates,
                     const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };
    const auto &candidate = turn_candidates[turn_index];
    const EdgeData &in_data = node_based_graph->GetEdgeData(via_eid);
    const EdgeData &out_data = node_based_graph->GetEdgeData(candidate.eid);
    const auto &candidate_to_the_left = turn_candidates[getLeft(turn_index)];

    const auto &candidate_to_the_right = turn_candidates[getRight(turn_index)];

    const auto hasValidRatio =
        [&](const TurnCandidate &left, const TurnCandidate &center, const TurnCandidate &right)
    {
        auto angle_left = (left.angle > 180) ? angularDeviation(left.angle, STRAIGHT_ANGLE) : 180;
        auto angle_right =
            (right.angle < 180) ? angularDeviation(right.angle, STRAIGHT_ANGLE) : 180;
        auto self_angle = angularDeviation(center.angle, STRAIGHT_ANGLE);
        return angularDeviation(center.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
               ((center.angle < STRAIGHT_ANGLE)
                    ? (angle_right > self_angle && angle_left / self_angle > DISTINCTION_RATIO)
                    : (angle_left > self_angle && angle_right / self_angle > DISTINCTION_RATIO));
    };
    // only valid turn
    if (!isLowPriorityRoadClass(
            node_based_graph->GetEdgeData(candidate.eid).road_classification.road_class))
    {
        bool is_only_normal_road = true;
        // TODO find out why this can also be reached for non-u-turns
        for (size_t i = 0; i < turn_candidates.size(); ++i)
        {
            if (i == turn_index || turn_candidates[i].angle == 0) // skip self and u-turn
                continue;
            if (!isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[i].eid)
                                            .road_classification.road_class))
            {
                is_only_normal_road = false;
                break;
            }
        }
        if (is_only_normal_road == true)
            return true;
    }

    return turn_candidates.size() == 1 ||
           // only non u-turn
           (turn_candidates.size() == 2 &&
            isUturn(candidate_to_the_left.instruction)) || // nearly straight turn
           angularDeviation(candidate.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION ||
           hasValidRatio(candidate_to_the_left, candidate, candidate_to_the_right) ||
           (in_data.name_id != 0 && in_data.name_id == out_data.name_id &&
            angularDeviation(candidate.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE / 2);
}

std::vector<TurnCandidate>
suppressTurns(const EdgeID via_eid,
              std::vector<TurnCandidate> turn_candidates,
              const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    if (turn_candidates.size() == 3)
    {
        BOOST_ASSERT(turn_candidates[0].instruction.direction_modifier == DirectionModifier::UTurn);
        if (isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[1].eid)
                                       .road_classification.road_class) &&
            !isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[2].eid)
                                        .road_classification.road_class))
        {
            if (angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
            {
                if (node_based_graph->GetEdgeData(turn_candidates[2].eid).name_id ==
                    node_based_graph->GetEdgeData(via_eid).name_id)
                {
                    turn_candidates[2].instruction = TurnInstruction::NO_TURN();
                }
                else
                {
                    turn_candidates[2].instruction.type = TurnType::NewName;
                }
                return turn_candidates;
            }
        }
        else if (isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[2].eid)
                                            .road_classification.road_class) &&
                 !isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[1].eid)
                                             .road_classification.road_class))
        {
            if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
            {
                if (node_based_graph->GetEdgeData(turn_candidates[1].eid).name_id ==
                    node_based_graph->GetEdgeData(via_eid).name_id)
                {
                    turn_candidates[1].instruction = TurnInstruction::NO_TURN();
                }
                else
                {
                    turn_candidates[1].instruction.type = TurnType::NewName;
                }
                return turn_candidates;
            }
        }
    }

    BOOST_ASSERT_MSG(std::is_sorted(turn_candidates.begin(), turn_candidates.end(),
                                    [](const TurnCandidate &left, const TurnCandidate &right)
                                    {
                                        return left.angle < right.angle;
                                    }),
                     "Turn Candidates not sorted by angle.");

    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    const EdgeData &in_data = node_based_graph->GetEdgeData(via_eid);

    bool has_obvious_with_same_name = false;
    double obvious_with_same_name_angle = 0;
    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        if (node_based_graph->GetEdgeData(turn_candidates[turn_index].eid).name_id ==
                in_data.name_id &&
            isObviousChoice(via_eid, turn_index, turn_candidates, node_based_graph))
        {
            has_obvious_with_same_name = true;
            obvious_with_same_name_angle = turn_candidates[turn_index].angle;
            break;
        }
    }

    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        auto &candidate = turn_candidates[turn_index];
        if (!isBasic(candidate.instruction.type))
            continue;

        const EdgeData &out_data = node_based_graph->GetEdgeData(candidate.eid);
        if (out_data.name_id == in_data.name_id && in_data.name_id != 0 &&
            candidate.instruction.direction_modifier != DirectionModifier::UTurn &&
            !has_obvious_with_same_name)
        {
            candidate.instruction.type = TurnType::Continue;
        }
        if (candidate.valid && !isUturn(candidate.instruction))
        {
            // TODO road category would be useful to indicate obviousness of turn
            // check if turn can be omitted or at least changed
            const auto &left = turn_candidates[getLeft(turn_index)];
            const auto &right = turn_candidates[getRight(turn_index)];

            // make very slight instructions straight, if they are the only valid choice going
            // with
            // at most a slight turn
            if ((!isSlightModifier(getTurnDirection(left.angle)) || !left.valid) &&
                (!isSlightModifier(getTurnDirection(right.angle)) || !right.valid) &&
                angularDeviation(candidate.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
                candidate.instruction.direction_modifier = DirectionModifier::Straight;

            // TODO this smaller comparison for turns is DANGEROUS, has to be revised if turn
            // instructions change
            if (in_data.travel_mode ==
                out_data.travel_mode) // make sure to always announce mode changes
            {
                if (isObviousChoice(via_eid, turn_index, turn_candidates, node_based_graph))
                {

                    if (in_data.name_id == out_data.name_id) // same road
                    {
                        candidate.instruction.type = TurnType::Suppressed;
                    }

                    else if (!has_obvious_with_same_name)
                    {
                        // TODO discuss, we might want to keep the current name of the turn. But
                        // this would mean emitting a turn when you just keep on a road
                        if (isRampClass(in_data.road_classification.road_class) &&
                            !isRampClass(out_data.road_classification.road_class))
                        {
                            candidate.instruction.type = TurnType::Merge;
                            candidate.instruction.direction_modifier =
                                mirrorDirectionModifier(candidate.instruction.direction_modifier);
                        }
                        else
                        {
                            if (canBeSuppressed(candidate.instruction.type))
                                candidate.instruction.type = TurnType::NewName;
                        }
                    }
                    else if (candidate.angle < obvious_with_same_name_angle)
                        candidate.instruction.direction_modifier = DirectionModifier::SlightRight;
                    else
                        candidate.instruction.direction_modifier = DirectionModifier::SlightLeft;
                }
                else if (candidate.instruction.direction_modifier == DirectionModifier::Straight &&
                         has_obvious_with_same_name)
                {
                    if (candidate.angle < obvious_with_same_name_angle)
                        candidate.instruction.direction_modifier = DirectionModifier::SlightRight;
                    else
                        candidate.instruction.direction_modifier = DirectionModifier::SlightLeft;
                }
            }
        }
    }
    return turn_candidates;
}

std::vector<TurnCandidate>
getTurnCandidates(const NodeID from_node,
                  const EdgeID via_eid,
                  const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph,
                  const std::vector<QueryNode> &node_info_list,
                  const std::shared_ptr<RestrictionMap const> restriction_map,
                  const std::unordered_set<NodeID> &barrier_nodes,
                  const CompressedEdgeContainer &compressed_edge_container)
{
    std::vector<TurnCandidate> turn_candidates;
    const NodeID turn_node = node_based_graph->GetTarget(via_eid);
    const NodeID only_restriction_to_node =
        restriction_map->CheckForEmanatingIsOnlyTurn(from_node, turn_node);
    const bool is_barrier_node = barrier_nodes.find(turn_node) != barrier_nodes.end();

    for (const EdgeID onto_edge : node_based_graph->GetAdjacentEdgeRange(turn_node))
    {
        bool turn_is_valid = true;
        if (node_based_graph->GetEdgeData(onto_edge).reversed)
        {
            turn_is_valid = false;
        }
        const NodeID to_node = node_based_graph->GetTarget(onto_edge);

        if (turn_is_valid && (only_restriction_to_node != SPECIAL_NODEID) &&
            (to_node != only_restriction_to_node))
        {
            // We are at an only_-restriction but not at the right turn.
            //            ++restricted_turns_counter;
            turn_is_valid = false;
        }

        if (turn_is_valid)
        {
            if (is_barrier_node)
            {
                if (from_node != to_node)
                {
                    //                    ++skipped_barrier_turns_counter;
                    turn_is_valid = false;
                }
            }
            else
            {
                if (from_node == to_node && node_based_graph->GetOutDegree(turn_node) > 1)
                {
                    auto number_of_emmiting_bidirectional_edges = 0;
                    for (auto edge : node_based_graph->GetAdjacentEdgeRange(turn_node))
                    {
                        auto target = node_based_graph->GetTarget(edge);
                        auto reverse_edge = node_based_graph->FindEdge(target, turn_node);
                        if (!node_based_graph->GetEdgeData(reverse_edge).reversed)
                        {
                            ++number_of_emmiting_bidirectional_edges;
                        }
                    }
                    if (number_of_emmiting_bidirectional_edges > 1)
                    {
                        //                        ++skipped_uturns_counter;
                        turn_is_valid = false;
                    }
                }
            }
        }

        // only add an edge if turn is not a U-turn except when it is
        // at the end of a dead-end street
        if (restriction_map->CheckIfTurnIsRestricted(from_node, turn_node, to_node) &&
            (only_restriction_to_node == SPECIAL_NODEID) && (to_node != only_restriction_to_node))
        {
            // We are at an only_-restriction but not at the right turn.
            //            ++restricted_turns_counter;
            turn_is_valid = false;
        }

        // unpack first node of second segment if packed

        const auto first_coordinate = getRepresentativeCoordinate(
            from_node, turn_node, via_eid, INVERT, compressed_edge_container, node_info_list);
        const auto third_coordinate = getRepresentativeCoordinate(
            turn_node, to_node, onto_edge, !INVERT, compressed_edge_container, node_info_list);

        const auto angle = util::coordinate_calculation::computeAngle(
            first_coordinate, node_info_list[turn_node], third_coordinate);

        turn_candidates.push_back(
            {onto_edge, turn_is_valid, angle, {TurnType::Invalid, DirectionModifier::UTurn}, 0});
    }

    const auto ByAngle = [](const TurnCandidate &first, const TurnCandidate second)
    {
        return first.angle < second.angle;
    };
    std::sort(std::begin(turn_candidates), std::end(turn_candidates), ByAngle);

    return mergeSegregatedRoads(from_node, via_eid, std::move(turn_candidates), node_based_graph);
}

std::vector<TurnCandidate>
mergeSegregatedRoads(const NodeID from_node,
                     const EdgeID via_eid,
                     std::vector<TurnCandidate> turn_candidates,
                     const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
#define PRINT_SEGREGATION_INFO 0

#if PRINT_SEGREGATION_INFO
    std::cout << "Input:\n";
    for (const auto &candidate : turn_candidates)
        std::cout << "\t" << candidate.toString() << std::endl;
#endif
    const auto getLeft = [&](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };

    const auto getRight = [&](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    const auto mergable = [&](std::size_t first, std::size_t second) -> bool
    {
        const auto &first_data = node_based_graph->GetEdgeData(turn_candidates[first].eid);
        const auto &second_data = node_based_graph->GetEdgeData(turn_candidates[second].eid);
#if PRINT_SEGREGATION_INFO
        std::cout << "First:  " << first_data.name_id << " " << first_data.travel_mode << " "
                  << first_data.road_classification.road_class << " "
                  << turn_candidates[first].angle << " " << first_data.reversed << "\n";
        std::cout << "Second: " << second_data.name_id << " " << second_data.travel_mode << " "
                  << second_data.road_classification.road_class << " "
                  << turn_candidates[second].angle << " " << second_data.reversed << std::endl;
        std::cout << "Deviation: " << angularDeviation(turn_candidates[first].angle,
                                                       turn_candidates[second].angle) << std::endl;
#endif

        return first_data.name_id != INVALID_NAME_ID && first_data.name_id == second_data.name_id &&
               !first_data.roundabout && !second_data.roundabout &&
               first_data.travel_mode == second_data.travel_mode &&
               first_data.road_classification == second_data.road_classification &&
               // compatible threshold
               angularDeviation(turn_candidates[first].angle, turn_candidates[second].angle) < 60 &&
               first_data.reversed != second_data.reversed;
    };

    const auto merge = [](const TurnCandidate &first, const TurnCandidate &second) -> TurnCandidate
    {
        if (!first.valid)
        {
            TurnCandidate result = second;
            result.angle = (first.angle + second.angle) / 2;
            if (first.angle - second.angle > 180)
                result.angle += 180;
            if (result.angle > 360)
                result.angle -= 360;

#if PRINT_SEGREGATION_INFO
            std::cout << "Merged: " << first.angle << " and " << second.angle << " to "
                      << result.angle << std::endl;
#endif
            return result;
        }
        else
        {
            BOOST_ASSERT(!second.valid);
            TurnCandidate result = first;
            result.angle = (first.angle + second.angle) / 2;

            if (first.angle - second.angle > 180)
                result.angle += 180;
            if (result.angle > 360)
                result.angle -= 360;

#if PRINT_SEGREGATION_INFO
            std::cout << "Merged: " << first.angle << " and " << second.angle << " to "
                      << result.angle << std::endl;
#endif
            return result;
        }
    };
    if (turn_candidates.size() == 1)
        return turn_candidates;

    if (mergable(0, turn_candidates.size() - 1))
    {
        // std::cout << "First merge" << std::endl;
        const double correction_factor =
            (360 - turn_candidates[turn_candidates.size() - 1].angle) / 2;
        for (std::size_t i = 1; i + 1 < turn_candidates.size(); ++i)
            turn_candidates[i].angle += correction_factor;
        turn_candidates[turn_candidates.size() - 1].angle = 0;
    }
    else if (mergable(0, 1))
    {
        // std::cout << "First merge" << std::endl;
        const double correction_factor = (turn_candidates[1].angle) / 2;
        for (std::size_t i = 2; i < turn_candidates.size(); ++i)
            turn_candidates[i].angle += correction_factor;
        turn_candidates[1].angle = 0;
    }

    for (std::size_t index = 0; index < turn_candidates.size(); ++index)
    {
        if (mergable(index, getRight(index)))
        {
            turn_candidates[getRight(index)] =
                merge(turn_candidates[getRight(index)], turn_candidates[index]);
            turn_candidates.erase(turn_candidates.begin() + index);
            --index;
        }
    }

    const auto ByAngle = [](const TurnCandidate &first, const TurnCandidate second)
    {
        return first.angle < second.angle;
    };
    std::sort(std::begin(turn_candidates), std::end(turn_candidates), ByAngle);
#if PRINT_SEGREGATION_INFO
    std::cout << "Result:\n";
    for (const auto &candidate : turn_candidates)
        std::cout << "\t" << candidate.toString() << std::endl;
#endif
    return turn_candidates;
}

// node_u -- (edge_1) --> node_v -- (edge_2) --> node_w
TurnInstruction
AnalyzeTurn(const NodeID node_u,
            const EdgeID edge1,
            const NodeID node_v,
            const EdgeID edge2,
            const NodeID node_w,
            const double angle,
            const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    (void)node_v;
    const EdgeData &data1 = node_based_graph->GetEdgeData(edge1);
    const EdgeData &data2 = node_based_graph->GetEdgeData(edge2);
    bool from_ramp = isRampClass(data1.road_classification.road_class);
    bool to_ramp = isRampClass(data2.road_classification.road_class);
    if (node_u == node_w)
    {
        return {TurnType::Turn, DirectionModifier::UTurn};
    }

    if (!from_ramp && to_ramp)
    {
        return {TurnType::Ramp, getTurnDirection(angle)};
    }

    // assign a designated turn angle instruction purely based on the angle
    return {TurnType::Turn, getTurnDirection(angle)};
}

std::vector<TurnCandidate>
handleConflicts(const NodeID from,
                const EdgeID via_edge,
                std::vector<TurnCandidate> turn_candidates,
                const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    (void)from;
    (void)via_edge;
    (void)node_based_graph;
    const auto isConflict = [](const TurnCandidate &left, const TurnCandidate &right)
    {
        // most obvious, same instructions conflict
        if (left.instruction == right.instruction)
            return true;

        return left.instruction.direction_modifier != DirectionModifier::UTurn &&
               left.instruction.direction_modifier == right.instruction.direction_modifier;
    };

    return turn_candidates;
}

void assignFork(const EdgeID via_edge,
                TurnCandidate &left,
                TurnCandidate &right,
                const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    const auto &in_data = node_based_graph->GetEdgeData(via_edge);
    { // left fork
        const auto &out_data = node_based_graph->GetEdgeData(left.eid);
        if (angularDeviation(left.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        {
            if (requiresAnnouncedment(in_data, out_data))
            {
                left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
            }
            else
            {
                left.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            }
        }
        else
        {
            left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        }
    }
    { // right fork
        const auto &out_data = node_based_graph->GetEdgeData(right.eid);
        if (angularDeviation(right.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        {
            if (requiresAnnouncedment(in_data, out_data))
            {
                right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
            }
            else
            {
                right.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            }
        }
        else
        {
            right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
        }
    }
}

void assignFork(const EdgeID via_edge,
                TurnCandidate &left,
                TurnCandidate &center,
                TurnCandidate &right,
                const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
    if (angularDeviation(center.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        const auto &in_data = node_based_graph->GetEdgeData(via_edge);
        const auto &out_data = node_based_graph->GetEdgeData(center.eid);
        if (requiresAnnouncedment(in_data, out_data))
        {
            center.instruction = {TurnType::Fork, DirectionModifier::Straight};
        }
        else
        {
            center.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
        }
    }
    else
    {
        center.instruction = {TurnType::Fork, DirectionModifier::Straight};
    }
    right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
}

} // anemspace detail
} // namespace guidance
} // namespace extractor
} // namespace osrm
