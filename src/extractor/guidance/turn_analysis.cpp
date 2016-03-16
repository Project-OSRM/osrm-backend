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

// what angle is interpreted as going straight
const double constexpr STRAIGHT_ANGLE = 180.;
// if a turn deviates this much from going straight, it will be kept straight
const double constexpr MAXIMAL_ALLOWED_NO_TURN_DEVIATION = 3.;
// angle that lies between two nearly indistinguishable roads
const double constexpr NARROW_TURN_ANGLE = 30.;
const double constexpr GROUP_ANGLE = 90;
// angle difference that can be classified as straight, if its the only narrow turn
const double constexpr FUZZY_ANGLE_DIFFERENCE = 15.;
const double constexpr DISTINCTION_RATIO = 2;
const unsigned constexpr INVALID_NAME_ID = 0;

using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

bool requiresAnnouncement(const EdgeData &from, const EdgeData &to)
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

TurnAnalysis::TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                           const std::vector<QueryNode> &node_info_list,
                           const RestrictionMap &restriction_map,
                           const std::unordered_set<NodeID> &barrier_nodes,
                           const CompressedEdgeContainer &compressed_edge_container,
                           const util::NameTable &name_table)
    : node_based_graph(node_based_graph), node_info_list(node_info_list),
      restriction_map(restriction_map), barrier_nodes(barrier_nodes),
      compressed_edge_container(compressed_edge_container), name_table(name_table)
{
}

namespace detail
{

inline FunctionalRoadClass roadClass(const TurnCandidate &candidate,
                                     const util::NodeBasedDynamicGraph &graph)
{
    return graph.GetEdgeData(candidate.eid).road_classification.road_class;
}

inline bool isMotorwayClass(FunctionalRoadClass road_class)
{
    return road_class == FunctionalRoadClass::MOTORWAY || road_class == FunctionalRoadClass::TRUNK;
}

inline bool isMotorwayClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    return isMotorwayClass(node_based_graph.GetEdgeData(eid).road_classification.road_class);
}

inline bool isRampClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    return isRampClass(node_based_graph.GetEdgeData(eid).road_classification.road_class);
}

} // namespace detail

#define PRINT_DEBUG_CANDIDATES 0
std::vector<TurnCandidate> TurnAnalysis::getTurns(const NodeID from, const EdgeID via_edge) const
{
    localizer.node_info_list = &node_info_list;
    auto turn_candidates = getTurnCandidates(from, via_edge);

    const auto &in_edge_data = node_based_graph.GetEdgeData(via_edge);

    // main priority: roundabouts
    bool on_roundabout = in_edge_data.roundabout;
    bool can_enter_roundabout = false;
    bool can_exit_roundabout = false;
    for (const auto &candidate : turn_candidates)
    {
        const auto &edge_data = node_based_graph.GetEdgeData(candidate.eid);
        // only check actual outgoing edges
        if (edge_data.reversed)
            continue;

        if (edge_data.roundabout)
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
        return handleRoundabouts(from, via_edge, on_roundabout, can_enter_roundabout,
                                 can_exit_roundabout, std::move(turn_candidates));
    }

    // set initial defaults for normal turns and modifier based on angle
    turn_candidates = setTurnTypes(from, via_edge, std::move(turn_candidates));

    if (isMotorwayJunction(from, via_edge, turn_candidates))
    {
        return handleMotorwayJunction(from, via_edge, std::move(turn_candidates));
    }

    if (turn_candidates.size() == 1)
    {
        turn_candidates = handleOneWayTurn(from, via_edge, std::move(turn_candidates));
    }
    else if (turn_candidates.size() == 2)
    {
        turn_candidates = handleTwoWayTurn(from, via_edge, std::move(turn_candidates));
    }
    else if (turn_candidates.size() == 3)
    {
        turn_candidates = handleThreeWayTurn(from, via_edge, std::move(turn_candidates));
    }
    else
    {
        turn_candidates = handleComplexTurn(from, via_edge, std::move(turn_candidates));
    }
    // complex intersection, potentially requires conflict resolution
    return handleConflicts(from, via_edge, std::move(turn_candidates));

    return turn_candidates;
}

inline std::size_t countValid(const std::vector<TurnCandidate> &turn_candidates)
{
    return std::count_if(turn_candidates.begin(), turn_candidates.end(),
                         [](const TurnCandidate &candidate)
                         {
                             return candidate.valid;
                         });
}

std::vector<TurnCandidate>
TurnAnalysis::handleRoundabouts(const NodeID from,
                                const EdgeID via_edge,
                                const bool on_roundabout,
                                const bool can_enter_roundabout,
                                const bool can_exit_roundabout,
                                std::vector<TurnCandidate> turn_candidates) const
{
    (void)from;
    // TODO requires differentiation between roundabouts and rotaries
    // detect via radius (get via circle through three vertices)
    NodeID node_v = node_based_graph.GetTarget(via_edge);
    if (on_roundabout)
    {
        // Shoule hopefully have only a single exit and continue
        // at least for cars. How about bikes?
        for (auto &candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
            if (out_data.roundabout)
            {
                // TODO can forks happen in roundabouts? E.g. required lane changes
                if (1 == node_based_graph.GetDirectedOutDegree(node_v))
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
                      << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                      << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
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
            const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
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
                      << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                      << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
#endif
        return turn_candidates;
    }
}

std::vector<TurnCandidate>
TurnAnalysis::fallbackTurnAssignmentMotorway(std::vector<TurnCandidate> turn_candidates) const
{
    for (auto &candidate : turn_candidates)
    {
        const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);

        util::SimpleLogger().Write(logWARNING)
            << "Candidate: " << candidate.toString() << " Name: " << out_data.name_id
            << " Road Class: " << (int)out_data.road_classification.road_class
            << " At: " << localizer(node_based_graph.GetTarget(candidate.eid));

        if (!candidate.valid)
            continue;

        const auto type = detail::isMotorwayClass(out_data.road_classification.road_class)
                              ? TurnType::Merge
                              : TurnType::Turn;
        if (angularDeviation(candidate.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE)
            candidate.instruction = {type, DirectionModifier::Straight};
        else
        {
            candidate.instruction = {type, candidate.angle > STRAIGHT_ANGLE
                                               ? DirectionModifier::SlightLeft
                                               : DirectionModifier::SlightRight};
        }
    }
    return turn_candidates;
}

std::vector<TurnCandidate> TurnAnalysis::handleFromMotorway(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    (void)from;
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    BOOST_ASSERT(detail::isMotorwayClass(in_data.road_classification.road_class));

    const auto countExitingMotorways = [this](const std::vector<TurnCandidate> &turn_candidates)
    {
        unsigned count = 0;
        for (const auto &candidate : turn_candidates)
        {
            if (candidate.valid && detail::isMotorwayClass(candidate.eid, node_based_graph))
                ++count;
        }
        return count;
    };

    // find the angle that continues on our current highway
    const auto getContinueAngle = [this, in_data](const std::vector<TurnCandidate> &turn_candidates)
    {
        for (const auto &candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
            if (candidate.angle != 0 && in_data.name_id == out_data.name_id &&
                in_data.name_id != 0 &&
                detail::isMotorwayClass(out_data.road_classification.road_class))
                return candidate.angle;
        }
        return turn_candidates[0].angle;
    };

    const auto getMostLikelyContinue = [this,
                                        in_data](const std::vector<TurnCandidate> &turn_candidates)
    {
        double angle = turn_candidates[0].angle;
        double best = 180;
        for (const auto &candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
            if (detail::isMotorwayClass(out_data.road_classification.road_class) &&
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
                assignFork(via_edge, turn_candidates[2], turn_candidates[1]);
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
        else if (turn_candidates.size() == 4 &&
                 detail::roadClass(turn_candidates[1], node_based_graph) ==
                     detail::roadClass(turn_candidates[2], node_based_graph) &&
                 detail::roadClass(turn_candidates[2], node_based_graph) ==
                     detail::roadClass(turn_candidates[3], node_based_graph))
        {
            // tripple fork at the end
            assignFork(via_edge, turn_candidates[3], turn_candidates[2], turn_candidates[1]);
        }
        else if (countValid(turn_candidates) > 0) // check whether turns exist at all
        {
            // FALLBACK, this should hopefully never be reached
            auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Fallback reached from motorway at " << std::setprecision(12)
                << toFloating(coord.lat) << " " << toFloating(coord.lon) << ", no continue angle, "
                << turn_candidates.size() << " candidates, " << countValid(turn_candidates)
                << " valid ones.";
            fallbackTurnAssignmentMotorway(turn_candidates);
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
                    BOOST_ASSERT(detail::isRampClass(candidate.eid, node_based_graph));
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
                BOOST_ASSERT(!detail::isRampClass(turn_candidates[1].eid, node_based_graph));

                turn_candidates[1].instruction =
                    getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[1]);
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
                            detail::isRampClass(candidate.eid, node_based_graph) ? TurnType::Ramp
                                                                                 : TurnType::Turn,
                            (candidate.angle < 145) ? DirectionModifier::Right
                                                    : DirectionModifier::SlightRight};
                    }
                    else if (candidate.angle > continue_angle)
                    {
                        candidate.instruction = {
                            detail::isRampClass(candidate.eid, node_based_graph) ? TurnType::Ramp
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
                    getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[1]);
                util::SimpleLogger().Write(logWARNING)
                    << "Disabled U-Turn on a freeway at "
                    << localizer(node_based_graph.GetTarget(via_edge));
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
                        detail::isMotorwayClass(turn_candidates[i].eid, node_based_graph))
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
                assignFork(via_edge, turn_candidates[second_valid], turn_candidates[first_valid]);
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
                        detail::isMotorwayClass(turn_candidates[i].eid, node_based_graph))
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
                           turn_candidates[first_valid]);
            }
            else
            {
                auto coord = localizer(node_based_graph.GetTarget(via_edge));
                util::SimpleLogger().Write(logWARNING)
                    << "Found motorway junction with more than "
                       "2 exiting motorways or additional ramps at "
                    << std::setprecision(12) << toFloating(coord.lat) << " "
                    << toFloating(coord.lon);
                fallbackTurnAssignmentMotorway(turn_candidates);
            }
        } // done for more than one highway exit
    }

#if PRINT_DEBUG_CANDIDATES
    std::cout << "From Motorway Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate> TurnAnalysis::handleMotorwayRamp(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    (void)from;
    auto num_valid_turns = countValid(turn_candidates);
    // ramp straight into a motorway/ramp
    if (turn_candidates.size() == 2 && num_valid_turns == 1)
    {
        BOOST_ASSERT(!turn_candidates[0].valid);
        BOOST_ASSERT(detail::isMotorwayClass(turn_candidates[1].eid, node_based_graph));

        turn_candidates[1].instruction =
            getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[1]);
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
                if (detail::isMotorwayClass(turn_candidates[1].eid, node_based_graph))
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
                    turn_candidates[1].instruction = getInstructionForObvious(
                        turn_candidates.size(), via_edge, turn_candidates[1]);
            }
            else
            {
                BOOST_ASSERT(turn_candidates[2].valid);
                if (detail::isMotorwayClass(turn_candidates[2].eid, node_based_graph))
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
                    turn_candidates[1].instruction = getInstructionForObvious(
                        turn_candidates.size(), via_edge, turn_candidates[1]);
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
            if (detail::isMotorwayClass(turn_candidates[1].eid, node_based_graph) &&
                detail::isMotorwayClass(turn_candidates[2].eid, node_based_graph))
            {
                assignFork(via_edge, turn_candidates[2], turn_candidates[1]);
            }
            else
            {
                // continued ramp passing motorway entry
                //      M  R
                //      M  R
                //      | /
                //      R
                if (detail::isMotorwayClass(node_based_graph.GetEdgeData(turn_candidates[1].eid)
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
            const auto &edge_data = node_based_graph.GetEdgeData(candidate.eid);
            if (!candidate.valid &&
                detail::isMotorwayClass(edge_data.road_classification.road_class))
            {
                passed_highway_entry = true;
            }
            else if (detail::isMotorwayClass(edge_data.road_classification.road_class))
            {
                candidate.instruction = {TurnType::Merge, passed_highway_entry
                                                              ? DirectionModifier::SlightRight
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
        fallbackTurnAssignmentMotorway(turn_candidates);
    }

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Onto Motorway Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate> TurnAnalysis::handleMotorwayJunction(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    (void)from;
    // BOOST_ASSERT(!turn_candidates[0].valid); //This fails due to @themarex handling of dead end
    // streets
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);

    // coming from motorway
    if (detail::isMotorwayClass(in_data.road_classification.road_class))
    {
        return handleFromMotorway(from, via_edge, std::move(turn_candidates));
    }
    else // coming from a ramp
    {
        return handleMotorwayRamp(from, via_edge, std::move(turn_candidates));
        // ramp merging straight onto motorway
    }
}

bool TurnAnalysis::isMotorwayJunction(const NodeID from,
                                      const EdgeID via_edge,
                                      const std::vector<TurnCandidate> &turn_candidates) const
{
    (void)from;

    bool has_motorway = false;
    bool has_normal_roads = false;

    for (const auto &candidate : turn_candidates)
    {
        const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
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

    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    return has_motorway ||
           in_data.road_classification.road_class == FunctionalRoadClass::MOTORWAY ||
           in_data.road_classification.road_class == FunctionalRoadClass::TRUNK;
}

TurnType TurnAnalysis::findBasicTurnType(const EdgeID via_edge,
                                         const TurnCandidate &candidate) const
{

    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);

    bool on_ramp = isRampClass(in_data.road_classification.road_class);

    bool onto_ramp = isRampClass(out_data.road_classification.road_class);

    if (!on_ramp && onto_ramp)
        return TurnType::Ramp;

    if (in_data.name_id == out_data.name_id && in_data.name_id != INVALID_NAME_ID)
    {
        return TurnType::Continue;
    }

    return TurnType::Turn;
}

TurnInstruction TurnAnalysis::getInstructionForObvious(const std::size_t num_candidates,
                                                       const EdgeID via_edge,
                                                       const TurnCandidate &candidate) const
{
    const auto type = findBasicTurnType(via_edge, candidate);
    if (type == TurnType::Ramp)
    {
        return {TurnType::Ramp, getTurnDirection(candidate.angle)};
    }

    if (angularDeviation(candidate.angle, 0) < 0.01)
    {
        return {TurnType::Turn, DirectionModifier::UTurn};
    }
    if (type == TurnType::Turn)
    {
        const auto &in_data = node_based_graph.GetEdgeData(via_edge);
        const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
        if (in_data.name_id != out_data.name_id &&
            requiresNameAnnounced(name_table.get_name_for_id(in_data.name_id),
                                  name_table.get_name_for_id(out_data.name_id)))
            return {TurnType::NewName, getTurnDirection(candidate.angle)};
        else
            return {TurnType::Suppressed, getTurnDirection(candidate.angle)};
    }
    BOOST_ASSERT(type == TurnType::Continue);
    if (num_candidates > 2)
    {
        return {TurnType::Suppressed, getTurnDirection(candidate.angle)};
    }
    else
    {
        return {TurnType::NoTurn, getTurnDirection(candidate.angle)};
    }
}

std::vector<TurnCandidate> TurnAnalysis::handleOneWayTurn(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    BOOST_ASSERT(turn_candidates[0].angle < 0.001);
    (void)from, (void)via_edge;

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic (one) Turn Candidates:\n";
    for (auto tc : turn_candidates)
    {
        std::cout << "\t" << tc.toString() << " ";
        if (tc.eid != SPECIAL_EDGEID)
        {
            std::cout << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class <<
                "name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
        }
        else
        {
            std::cout << " dead end" << std::endl;
        }
    }
#endif
    return turn_candidates;
}

std::vector<TurnCandidate> TurnAnalysis::handleTwoWayTurn(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    BOOST_ASSERT(turn_candidates[0].angle < 0.001);
    (void)from;
    turn_candidates[1].instruction =
        getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[1]);

    if (turn_candidates[1].instruction.type == TurnType::Suppressed)
        turn_candidates[1].instruction.type = TurnType::NoTurn;

#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic Two Turns Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate> TurnAnalysis::handleThreeWayTurn(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    BOOST_ASSERT(turn_candidates[0].angle < 0.001);
    (void)from;
    const auto isObviousOfTwo = [](const TurnCandidate turn, const TurnCandidate other)
    {
        return (angularDeviation(turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
                angularDeviation(other.angle, STRAIGHT_ANGLE) > 85) ||
               (angularDeviation(other.angle, STRAIGHT_ANGLE) /
                    angularDeviation(turn.angle, STRAIGHT_ANGLE) >
                1.4);
    };

    /* Two nearly straight turns -> FORK
                OOOOOOO
              /
       IIIIII
              \
                OOOOOOO
    */
    if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
        angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
    {
        if (turn_candidates[1].valid && turn_candidates[2].valid)
        {
            const auto left_class =
                node_based_graph.GetEdgeData(turn_candidates[2].eid).road_classification.road_class;
            const auto right_class =
                node_based_graph.GetEdgeData(turn_candidates[1].eid).road_classification.road_class;
            if (canBeSeenAsFork(left_class, right_class))
                assignFork(via_edge, turn_candidates[2], turn_candidates[1]);
            else if (getPriority(left_class) > getPriority(right_class))
            {
                turn_candidates[1].instruction =
                    getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[1]);
                turn_candidates[2].instruction = {findBasicTurnType(via_edge, turn_candidates[2]),
                                                  DirectionModifier::SlightLeft};
            }
            else
            {
                turn_candidates[2].instruction =
                    getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[2]);
                turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                                  DirectionModifier::SlightRight};
            }
        }
        else
        {
            if (turn_candidates[1].valid)
                turn_candidates[1].instruction =
                    getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[1]);
            if (turn_candidates[2].valid)
                turn_candidates[2].instruction =
                    getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[2]);
        }
    }
    /*  T Intersection

      OOOOOOO T OOOOOOOO
              I
              I
              I
    */
    else if (angularDeviation(turn_candidates[1].angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[2].angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >
                 NARROW_TURN_ANGLE)
    {
        if (turn_candidates[1].valid)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, turn_candidates[1]))
                turn_candidates[1].instruction = {TurnType::EndOfRoad, DirectionModifier::Right};
            else
                turn_candidates[1].instruction = {TurnType::Ramp, DirectionModifier::Right};
        }
        if (turn_candidates[2].valid)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, turn_candidates[2]))

                turn_candidates[2].instruction = {TurnType::EndOfRoad, DirectionModifier::Left};
            else
                turn_candidates[2].instruction = {TurnType::Ramp, DirectionModifier::Left};
        }
    }
    /* T Intersection, Cross left
              O
              O
              O
     IIIIIIII - OOOOOOOOOO
    */
    else if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[2].angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >
                 NARROW_TURN_ANGLE)
    {
        if (turn_candidates[1].valid)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, turn_candidates[1]))
                turn_candidates[1].instruction =
                    getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[1]);
            else
                turn_candidates[1].instruction = {TurnType::Ramp, DirectionModifier::Straight};
        }
        if (turn_candidates[2].valid)
        {
            turn_candidates[2].instruction = {findBasicTurnType(via_edge, turn_candidates[2]),
                                              DirectionModifier::Left};
        }
    }
    /* T Intersection, Cross right

     IIIIIIII T OOOOOOOOOO
              O
              O
              O
    */
    else if (angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >
                 NARROW_TURN_ANGLE)
    {
        if (turn_candidates[2].valid)
            turn_candidates[2].instruction =
                getInstructionForObvious(turn_candidates.size(), via_edge, turn_candidates[2]);
        if (turn_candidates[1].valid)
            turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                              DirectionModifier::Right};
    }
    // merge onto a through street
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(turn_candidates[1].eid).name_id &&
             node_based_graph.GetEdgeData(turn_candidates[1].eid).name_id ==
                 node_based_graph.GetEdgeData(turn_candidates[2].eid).name_id)
    {
        const auto findTurn = [isObviousOfTwo](const TurnCandidate turn,
                                               const TurnCandidate other) -> TurnInstruction
        {
            return {isObviousOfTwo(turn, other) ? TurnType::Merge : TurnType::Turn,
                    getTurnDirection(turn.angle)};
        };
        turn_candidates[1].instruction = findTurn(turn_candidates[1], turn_candidates[2]);
        turn_candidates[2].instruction = findTurn(turn_candidates[2], turn_candidates[1]);
    }

    // other street merges from the left
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(via_edge).name_id &&
             node_based_graph.GetEdgeData(via_edge).name_id ==
                 node_based_graph.GetEdgeData(turn_candidates[1].eid).name_id)
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
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(via_edge).name_id &&
             node_based_graph.GetEdgeData(via_edge).name_id ==
                 node_based_graph.GetEdgeData(turn_candidates[2].eid).name_id)
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
        const unsigned in_name_id = node_based_graph.GetEdgeData(via_edge).name_id;
        const unsigned out_names[2] = {
            node_based_graph.GetEdgeData(turn_candidates[1].eid).name_id,
            node_based_graph.GetEdgeData(turn_candidates[2].eid).name_id};
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
    std::cout << "Basic Three Turn Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

void TurnAnalysis::handleDistinctConflict(const EdgeID via_edge,
                                          TurnCandidate &left,
                                          TurnCandidate &right) const
{
    // single turn of both is valid (don't change the valid one)
    // or multiple identical angles -> bad OSM intersection
    if ((!left.valid || !right.valid) || (left.angle == right.angle))
    {
        if (left.valid)
            left.instruction = {findBasicTurnType(via_edge, left), getTurnDirection(left.angle)};
        if (right.valid)
            right.instruction = {findBasicTurnType(via_edge, right), getTurnDirection(right.angle)};
        return;
    }

    if (getTurnDirection(left.angle) == DirectionModifier::Straight ||
        getTurnDirection(left.angle) == DirectionModifier::SlightLeft ||
        getTurnDirection(right.angle) == DirectionModifier::SlightRight)
    {
        const auto left_class =
            node_based_graph.GetEdgeData(left.eid).road_classification.road_class;
        const auto right_class =
            node_based_graph.GetEdgeData(right.eid).road_classification.road_class;
        if (canBeSeenAsFork(left_class, right_class))
            assignFork(via_edge, left, right);
        else if (getPriority(left_class) > getPriority(right_class))
        {
            // FIXME this should possibly know about the actual candidates?
            right.instruction = getInstructionForObvious(4, via_edge, right);
            left.instruction = {findBasicTurnType(via_edge, left), DirectionModifier::SlightLeft};
        }
        else
        {
            // FIXME this should possibly know about the actual candidates?
            left.instruction = getInstructionForObvious(4, via_edge, left);
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
    // Both turns?
    if (TurnType::Ramp != left_type && TurnType::Ramp != right_type)
    {
        if (left.angle < STRAIGHT_ANGLE)
        {
            left.instruction = {TurnType::FirstTurn, getTurnDirection(left.angle)};
            right.instruction = {TurnType::SecondTurn, getTurnDirection(right.angle)};
        }
        else
        {
            left.instruction = {TurnType::SecondTurn, getTurnDirection(left.angle)};
            right.instruction = {TurnType::FirstTurn, getTurnDirection(right.angle)};
        }
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
        if (angularDeviation(left.angle, 90) > angularDeviation(right.angle, 90))
        {
            left.instruction = {left_type, DirectionModifier::SlightRight};
            right.instruction = {right_type, DirectionModifier::Right};
        }
        else
        {
            left.instruction = {left_type, DirectionModifier::Right};
            right.instruction = {right_type, DirectionModifier::SharpRight};
        }
    }
    else
    {
        if (angularDeviation(left.angle, 270) > angularDeviation(right.angle, 270))
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

std::vector<TurnCandidate> TurnAnalysis::handleComplexTurn(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    (void)from; // FIXME unused
    static int fallback_count = 0;
    const std::size_t obvious_index = findObviousTurn(via_edge, turn_candidates);
    const auto fork_range = findFork(via_edge, turn_candidates);
    std::size_t straightmost_turn = 0;
    double straightmost_deviation = 180;
    for (std::size_t i = 0; i < turn_candidates.size(); ++i)
    {
        const double deviation = angularDeviation(turn_candidates[i].angle, STRAIGHT_ANGLE);
        if (deviation < straightmost_deviation)
        {
            straightmost_deviation = deviation;
            straightmost_turn = i;
        }
    }

    if (obvious_index != 0)
    {
        turn_candidates[obvious_index].instruction = getInstructionForObvious(
            turn_candidates.size(), via_edge, turn_candidates[obvious_index]);

        // assign left/right turns
        turn_candidates = assignLeftTurns(via_edge, std::move(turn_candidates), obvious_index + 1);
        turn_candidates = assignRightTurns(via_edge, std::move(turn_candidates), obvious_index);
    }
    else if (fork_range.first != 0 && fork_range.second - fork_range.first <= 2) // found fork
    {
        if (fork_range.second - fork_range.first == 1)
        {
            auto &left = turn_candidates[fork_range.second];
            auto &right = turn_candidates[fork_range.first];
            const auto left_class =
                node_based_graph.GetEdgeData(left.eid).road_classification.road_class;
            const auto right_class =
                node_based_graph.GetEdgeData(right.eid).road_classification.road_class;
            if (canBeSeenAsFork(left_class, right_class))
                assignFork(via_edge, left, right);
            else if (getPriority(left_class) > getPriority(right_class))
            {
                right.instruction =
                    getInstructionForObvious(turn_candidates.size(), via_edge, right);
                left.instruction = {findBasicTurnType(via_edge, left),
                                    DirectionModifier::SlightLeft};
            }
            else
            {
                left.instruction = getInstructionForObvious(turn_candidates.size(), via_edge, left);
                right.instruction = {findBasicTurnType(via_edge, right),
                                     DirectionModifier::SlightRight};
            }
        }
        else if (fork_range.second - fork_range.second == 2)
        {
            assignFork(via_edge, turn_candidates[fork_range.second],
                       turn_candidates[fork_range.first + 1], turn_candidates[fork_range.first]);
        }
        // assign left/right turns
        turn_candidates =
            assignLeftTurns(via_edge, std::move(turn_candidates), fork_range.second + 1);
        turn_candidates = assignRightTurns(via_edge, std::move(turn_candidates), fork_range.first);
    }
    else if (straightmost_deviation < FUZZY_ANGLE_DIFFERENCE &&
             !turn_candidates[straightmost_turn].valid)
    {
        // invalid straight turn
        turn_candidates =
            assignLeftTurns(via_edge, std::move(turn_candidates), straightmost_turn + 1);
        turn_candidates = assignRightTurns(via_edge, std::move(turn_candidates), straightmost_turn);
    }
    // no straight turn
    else if (turn_candidates[straightmost_turn].angle > 180)
    {
        // at most three turns on either side
        turn_candidates = assignLeftTurns(via_edge, std::move(turn_candidates), straightmost_turn);
        turn_candidates = assignRightTurns(via_edge, std::move(turn_candidates), straightmost_turn);
    }
    else if (turn_candidates[straightmost_turn].angle < 180)
    {
        turn_candidates =
            assignLeftTurns(via_edge, std::move(turn_candidates), straightmost_turn + 1);
        turn_candidates =
            assignRightTurns(via_edge, std::move(turn_candidates), straightmost_turn + 1);
    }
    else
    {
        if (fallback_count++ < 10)
        {
            const auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Resolved to keep fallback on complex turn assignment at "
                << std::setprecision(12) << toFloating(coord.lat) << " " << toFloating(coord.lon)
                << "Straightmost: " << straightmost_turn;
            ;
            for (const auto &candidate : turn_candidates)
            {
                const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "Candidate: " << candidate.toString() << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class
                    << " At: " << localizer(node_based_graph.GetTarget(candidate.eid));
            }
        }
    }
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Basic Complex Turn Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate> TurnAnalysis::setTurnTypes(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    NodeID turn_node = node_based_graph.GetTarget(via_edge);

    for (auto &candidate : turn_candidates)
    {
        if (!candidate.valid)
            continue;
        const EdgeID onto_edge = candidate.eid;
        const NodeID to_node = node_based_graph.GetTarget(onto_edge);

        auto turn = AnalyzeTurn(from, via_edge, turn_node, onto_edge, to_node, candidate.angle);

        auto confidence = getTurnConfidence(candidate.angle, turn);
        candidate.instruction = turn;
        candidate.confidence = confidence;
    }
    return turn_candidates;
}

//                                               a
//                                               |
//                                               |
//                                               v
// For an intersection from_node --via_edi--> turn_node ----> c
//                                               ^
//                                               |
//                                               |
//                                               b
// This functions returns _all_ turns as if the graph was undirected.
// That means we not only get (from_node, turn_node, c) in the above example
// but also (from_node, turn_node, a), (from_node, turn_node, b). These turns are
// marked as invalid and only needed for intersection classification.
std::vector<TurnCandidate> TurnAnalysis::getTurnCandidates(const NodeID from_node,
                                                           const EdgeID via_eid) const
{
    std::vector<TurnCandidate> turn_candidates;
    const NodeID turn_node = node_based_graph.GetTarget(via_eid);
    const NodeID only_restriction_to_node =
        restriction_map.CheckForEmanatingIsOnlyTurn(from_node, turn_node);
    const bool is_barrier_node = barrier_nodes.find(turn_node) != barrier_nodes.end();

    for (const EdgeID onto_edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
    {
        BOOST_ASSERT( onto_edge != SPECIAL_EDGEID );
        const NodeID to_node = node_based_graph.GetTarget(onto_edge);

        bool turn_is_valid =
            // reverse edges are never valid turns because the resulting turn would look like this:
            // from_node --via_edge--> turn_node <--onto_edge-- to_node
            // however we need this for capture intersection shape for incoming one-ways
            !node_based_graph.GetEdgeData(onto_edge).reversed &&
            // we are not turning over a barrier
            (!is_barrier_node || from_node == to_node) &&
            // We are at an only_-restriction but not at the right turn.
            (only_restriction_to_node == SPECIAL_NODEID || to_node == only_restriction_to_node) &&
            // the turn is not restricted
            !restriction_map.CheckIfTurnIsRestricted(from_node, turn_node, to_node);

        auto angle = 0.;
        if (from_node == to_node)
        {
            if (turn_is_valid && !is_barrier_node)
            {
                // we only add u-turns for dead-end streets.
                if (node_based_graph.GetOutDegree(turn_node) > 1)
                {
                    auto number_of_emmiting_bidirectional_edges = 0;
                    for (auto edge : node_based_graph.GetAdjacentEdgeRange(turn_node))
                    {
                        auto target = node_based_graph.GetTarget(edge);
                        auto reverse_edge = node_based_graph.FindEdge(target, turn_node);
                        BOOST_ASSERT(reverse_edge != SPECIAL_EDGEID);
                        if (!node_based_graph.GetEdgeData(reverse_edge).reversed)
                        {
                            ++number_of_emmiting_bidirectional_edges;
                        }
                    }
                    // is a dead-end
                    turn_is_valid = number_of_emmiting_bidirectional_edges <= 1;
                }
            }
            BOOST_ASSERT(angle >= 0. && angle < std::numeric_limits<double>::epsilon());
        }
        else
        {
            // unpack first node of second segment if packed
            const auto first_coordinate = getRepresentativeCoordinate(
                from_node, turn_node, via_eid, INVERT, compressed_edge_container, node_info_list);
            const auto third_coordinate = getRepresentativeCoordinate(
                turn_node, to_node, onto_edge, !INVERT, compressed_edge_container, node_info_list);
            angle = util::coordinate_calculation::computeAngle(
                first_coordinate, node_info_list[turn_node], third_coordinate);
        }

        turn_candidates.push_back(
            {onto_edge, turn_is_valid, angle, {TurnType::Invalid, DirectionModifier::UTurn}, 0});
    }

    const auto ByAngle = [](const TurnCandidate &first, const TurnCandidate second)
    {
        return first.angle < second.angle;
    };
    std::sort(std::begin(turn_candidates), std::end(turn_candidates), ByAngle);

    BOOST_ASSERT(turn_candidates[0].angle >= 0. && turn_candidates[0].angle < std::numeric_limits<double>::epsilon());

    return mergeSegregatedRoads(from_node, via_eid, std::move(turn_candidates));
}

std::vector<TurnCandidate> TurnAnalysis::mergeSegregatedRoads(
    const NodeID from_node, const EdgeID via_eid, std::vector<TurnCandidate> turn_candidates) const
{
    (void)from_node; // FIXME
    (void)via_eid;   // FIXME
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
    (void)getLeft; // FIXME

    const auto getRight = [&](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    const auto mergable = [&](std::size_t first, std::size_t second) -> bool
    {
        const auto &first_data = node_based_graph.GetEdgeData(turn_candidates[first].eid);
        const auto &second_data = node_based_graph.GetEdgeData(turn_candidates[second].eid);
#if PRINT_SEGREGATION_INFO
        std::cout << "First:  " << first_data.name_id << " " << first_data.travel_mode << " "
                  << first_data.road_classification.road_class << " "
                  << turn_candidates[first].angle << " " << first_data.reversed << "\n";
        std::cout << "Second: " << second_data.name_id << " " << second_data.travel_mode << " "
                  << second_data.road_classification.road_class << " "
                  << turn_candidates[second].angle << " " << second_data.reversed << std::endl;
        std::cout << "Deviation: "
                  << angularDeviation(turn_candidates[first].angle, turn_candidates[second].angle)
                  << std::endl;
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
        turn_candidates[0] = merge(turn_candidates.front(),turn_candidates.back());
        turn_candidates[0].angle = 0;
        turn_candidates.pop_back();
    }
    else if (mergable(0, 1))
    {
        // std::cout << "First merge" << std::endl;
        const double correction_factor = (turn_candidates[1].angle) / 2;
        for (std::size_t i = 2; i < turn_candidates.size(); ++i)
            turn_candidates[i].angle += correction_factor;
        turn_candidates[0] = merge(turn_candidates[0],turn_candidates[1]);
        turn_candidates[0].angle = 0;
        turn_candidates.erase(turn_candidates.begin() + 1);
    }

    for (std::size_t index = 2; index < turn_candidates.size(); ++index)
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
TurnInstruction TurnAnalysis::AnalyzeTurn(const NodeID node_u,
                                          const EdgeID edge1,
                                          const NodeID node_v,
                                          const EdgeID edge2,
                                          const NodeID node_w,
                                          const double angle) const
{
    (void)node_v;
    const EdgeData &data1 = node_based_graph.GetEdgeData(edge1);
    const EdgeData &data2 = node_based_graph.GetEdgeData(edge2);
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

std::vector<TurnCandidate> TurnAnalysis::handleConflicts(
    const NodeID from, const EdgeID via_edge, std::vector<TurnCandidate> turn_candidates) const
{
    (void)from;     // FIXME
    (void)via_edge; // FIXME
    const auto isConflict = [](const TurnCandidate &left, const TurnCandidate &right)
    {
        // most obvious, same instructions conflict
        if (left.instruction == right.instruction)
            return true;

        return left.instruction.direction_modifier != DirectionModifier::UTurn &&
               left.instruction.direction_modifier == right.instruction.direction_modifier;
    };
    (void)isConflict; // FIXME
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Post Conflict Resolution Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph.GetEdgeData(tc.eid).road_classification.road_class
                  << " name: " << node_based_graph.GetEdgeData(tc.eid).name_id << std::endl;
#endif
    return turn_candidates;
}

void TurnAnalysis::assignFork(const EdgeID via_edge,
                              TurnCandidate &left,
                              TurnCandidate &right) const
{
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);
    const bool low_priority_left = isLowPriorityRoadClass(
        node_based_graph.GetEdgeData(left.eid).road_classification.road_class);
    const bool low_priority_right = isLowPriorityRoadClass(
        node_based_graph.GetEdgeData(right.eid).road_classification.road_class);
    { // left fork
        const auto &out_data = node_based_graph.GetEdgeData(left.eid);
        if ((angularDeviation(left.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
             angularDeviation(right.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE))
        {
            if (requiresAnnouncement(in_data, out_data))
            {
                if (low_priority_right && !low_priority_left)
                    left.instruction = getInstructionForObvious(3, via_edge, left);
                else
                {
                    if (low_priority_left && !low_priority_right)
                        left.instruction = {TurnType::Turn, DirectionModifier::SlightLeft};
                    else
                        left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
                }
            }
            else
            {
                left.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            }
        }
        else
        {
            if (low_priority_right && !low_priority_left)
                left.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
            else
            {
                if (low_priority_left && !low_priority_right)
                    left.instruction = {TurnType::Turn, DirectionModifier::SlightLeft};
                else
                    left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
            }
        }
    }
    { // right fork
        const auto &out_data = node_based_graph.GetEdgeData(right.eid);
        if (angularDeviation(right.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            angularDeviation(left.angle, STRAIGHT_ANGLE) > FUZZY_ANGLE_DIFFERENCE)
        {
            if (requiresAnnouncement(in_data, out_data))
            {
                if (low_priority_left && !low_priority_right)
                    right.instruction = getInstructionForObvious(3, via_edge, right);
                else
                {
                    if (low_priority_right && !low_priority_left)
                        right.instruction = {TurnType::Turn, DirectionModifier::SlightRight};
                    else
                        right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
                }
            }
            else
            {
                right.instruction = {TurnType::Suppressed, DirectionModifier::Straight};
            }
        }
        else
        {
            if (low_priority_left && !low_priority_right)
                right.instruction = {TurnType::Suppressed, DirectionModifier::SlightLeft};
            else
            {
                if (low_priority_right && !low_priority_left)
                    right.instruction = {TurnType::Turn, DirectionModifier::SlightRight};
                else
                    right.instruction = {TurnType::Fork, DirectionModifier::SlightRight};
            }
        }
    }
}

void TurnAnalysis::assignFork(const EdgeID via_edge,
                              TurnCandidate &left,
                              TurnCandidate &center,
                              TurnCandidate &right) const
{
    // TODO handle low priority road classes in a reasonable way
    if (left.valid && center.valid && right.valid)
    {
        left.instruction = {TurnType::Fork, DirectionModifier::SlightLeft};
        if (angularDeviation(center.angle, 180) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
        {
            const auto &in_data = node_based_graph.GetEdgeData(via_edge);
            const auto &out_data = node_based_graph.GetEdgeData(center.eid);
            if (requiresAnnouncement(in_data, out_data))
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
    else if (left.valid)
    {
        if (right.valid)
            assignFork(via_edge, left, right);
        else if (center.valid)
            assignFork(via_edge, left, center);
        else
            left.instruction = {findBasicTurnType(via_edge, left), getTurnDirection(left.angle)};
    }
    else if (right.valid)
    {
        if (center.valid)
            assignFork(via_edge, center, right);
        else
            right.instruction = {findBasicTurnType(via_edge, right), getTurnDirection(right.angle)};
    }
    else
    {
        if (center.valid)
            center.instruction = {findBasicTurnType(via_edge, center),
                                  getTurnDirection(center.angle)};
    }
}

std::size_t TurnAnalysis::findObviousTurn(const EdgeID via_edge,
                                          const std::vector<TurnCandidate> &turn_candidates) const
{
    // no obvious candidate
    if (turn_candidates.size() == 1)
        return 0;

    // a single non u-turn is obvious
    if (turn_candidates.size() == 2)
        return 1;

    // at least three candidates
    std::size_t best = 0;
    double best_deviation = 180;

    std::size_t best_continue = 0;
    double best_continue_deviation = 180;

    const EdgeData &in_data = node_based_graph.GetEdgeData(via_edge);
    for (std::size_t i = 1; i < turn_candidates.size(); ++i)
    {
        const double deviation = angularDeviation(turn_candidates[i].angle, STRAIGHT_ANGLE);
        if (turn_candidates[i].valid && deviation < best_deviation)
        {
            best_deviation = deviation;
            best = i;
        }

        const auto out_data = node_based_graph.GetEdgeData(turn_candidates[i].eid);
        if (turn_candidates[i].valid && out_data.name_id == in_data.name_id &&
            deviation < best_continue_deviation)
        {
            best_continue_deviation = deviation;
            best_continue = i;
        }
    }

    if (best == 0)
        return 0;

    if (best_deviation >= 2 * NARROW_TURN_ANGLE)
        return 0;

    // TODO incorporate road class in decision
    if (best != 0 && best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        return best;
    }

    // has no obvious continued road
    if (best_continue == 0 || true)
    {
        // Find left/right deviation
        const double left_deviation = angularDeviation(
            turn_candidates[(best + 1) % turn_candidates.size()].angle, STRAIGHT_ANGLE);
        const double right_deviation =
            angularDeviation(turn_candidates[best - 1].angle, STRAIGHT_ANGLE);

        if (best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            std::min(left_deviation, right_deviation) > FUZZY_ANGLE_DIFFERENCE)
            return best;

        // other narrow turns?
        if (angularDeviation(turn_candidates[best - 1].angle, STRAIGHT_ANGLE) <=
            FUZZY_ANGLE_DIFFERENCE)
            return 0;
        if (angularDeviation(turn_candidates[(best + 1) % turn_candidates.size()].angle,
                             STRAIGHT_ANGLE) <= FUZZY_ANGLE_DIFFERENCE)
            return 0;

        // Well distinct turn that is nearly straight
        if (left_deviation / best_deviation >= DISTINCTION_RATIO &&
            right_deviation / best_deviation >= DISTINCTION_RATIO)
        {
            return best;
        }
    }

    return 0; // no obvious turn
}

std::pair<std::size_t, std::size_t>
TurnAnalysis::findFork(const EdgeID via_edge,
                       const std::vector<TurnCandidate> &turn_candidates) const
{

    std::size_t best = 0;
    double best_deviation = 180;

    // TODO handle road classes
    (void)via_edge;

    for (std::size_t i = 1; i < turn_candidates.size(); ++i)
    {
        const double deviation = angularDeviation(turn_candidates[i].angle, STRAIGHT_ANGLE);
        if (turn_candidates[i].valid && deviation < best_deviation)
        {
            best_deviation = deviation;
            best = i;
        }
    }

    if (best_deviation <= NARROW_TURN_ANGLE)
    {
        std::size_t left = best, right = best;
        while (left + 1 < turn_candidates.size() &&
               angularDeviation(turn_candidates[left].angle, turn_candidates[left + 1].angle) <
                   NARROW_TURN_ANGLE)
            ++left;
        while (right > 1 &&
               angularDeviation(turn_candidates[right].angle, turn_candidates[right - 1].angle) <
                   NARROW_TURN_ANGLE)
            --right;

        // TODO check whether 2*NARROW_TURN is too large
        if (right < left &&
            angularDeviation(turn_candidates[left].angle,
                             turn_candidates[(left + 1) % turn_candidates.size()].angle) >=
                2 * NARROW_TURN_ANGLE &&
            angularDeviation(turn_candidates[right].angle, turn_candidates[right - 1].angle) >=
                2 * NARROW_TURN_ANGLE)
            return std::make_pair(right, left);
    }
    return std::make_pair(0llu, 0llu);
}

// Can only assign three turns
std::vector<TurnCandidate> TurnAnalysis::assignLeftTurns(const EdgeID via_edge,
                                                         std::vector<TurnCandidate> turn_candidates,
                                                         const std::size_t starting_at) const
{
    const auto count_valid = [&turn_candidates, starting_at]()
    {
        std::size_t count = 0;
        for (std::size_t i = starting_at; i < turn_candidates.size(); ++i)
            if (turn_candidates[i].valid)
                ++count;
        return count;
    };
    if (starting_at == turn_candidates.size() || count_valid() == 0)
        return turn_candidates;
    // handle single turn
    if (turn_candidates.size() - starting_at == 1)
    {
        if (!turn_candidates[starting_at].valid)
            return turn_candidates;

        if (angularDeviation(turn_candidates[starting_at].angle, STRAIGHT_ANGLE) >
                NARROW_TURN_ANGLE &&
            angularDeviation(turn_candidates[starting_at].angle, 0) > NARROW_TURN_ANGLE)
        {
            // assign left turn
            turn_candidates[starting_at].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at]), DirectionModifier::Left};
        }
        else if (angularDeviation(turn_candidates[starting_at].angle, STRAIGHT_ANGLE) <=
                 NARROW_TURN_ANGLE)
        {
            turn_candidates[starting_at].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at]),
                DirectionModifier::SlightLeft};
        }
        else
        {
            turn_candidates[starting_at].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at]),
                DirectionModifier::SharpLeft};
        }
    }
    // two turns on at the side
    else if (turn_candidates.size() - starting_at == 2)
    {
        const auto first_direction = getTurnDirection(turn_candidates[starting_at].angle);
        const auto second_direction = getTurnDirection(turn_candidates[starting_at + 1].angle);
        if (first_direction == second_direction)
        {
            // conflict
            handleDistinctConflict(via_edge, turn_candidates[starting_at + 1],
                                   turn_candidates[starting_at]);
        }
        else
        {
            turn_candidates[starting_at].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at]), first_direction};
            turn_candidates[starting_at + 1].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at + 1]), second_direction};
        }
    }
    else if (turn_candidates.size() - starting_at == 3)
    {
        const auto first_direction = getTurnDirection(turn_candidates[starting_at].angle);
        const auto second_direction = getTurnDirection(turn_candidates[starting_at + 1].angle);
        const auto third_direction = getTurnDirection(turn_candidates[starting_at + 2].angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            // implies first != third, based on the angles and clockwise order
            if (turn_candidates[starting_at].valid)
                turn_candidates[starting_at].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at]), first_direction};
            if (turn_candidates[starting_at + 1].valid)
                turn_candidates[starting_at + 1].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at + 1]),
                    second_direction};
            if (turn_candidates[starting_at + 2].valid)
                turn_candidates[starting_at + 2].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at + 2]),
                    second_direction};
        }
        else if (2 >= (turn_candidates[starting_at].valid + turn_candidates[starting_at + 1].valid +
                       turn_candidates[starting_at + 2].valid))
        {
            // at least one invalid turn
            if (!turn_candidates[starting_at].valid)
            {
                handleDistinctConflict(via_edge, turn_candidates[starting_at + 2],
                                       turn_candidates[starting_at + 1]);
            }
            else if (!turn_candidates[starting_at + 1].valid)
            {
                handleDistinctConflict(via_edge, turn_candidates[starting_at + 2],
                                       turn_candidates[starting_at]);
            }
            else
            {
                handleDistinctConflict(via_edge, turn_candidates[starting_at + 1],
                                       turn_candidates[starting_at]);
            }
        }
        else if (turn_candidates[starting_at].valid && turn_candidates[starting_at + 1].valid &&
                 turn_candidates[starting_at + 2].valid &&
                 angularDeviation(turn_candidates[starting_at].angle,
                                  turn_candidates[starting_at + 1].angle) >= NARROW_TURN_ANGLE &&
                 angularDeviation(turn_candidates[starting_at + 1].angle,
                                  turn_candidates[starting_at + 2].angle) >= NARROW_TURN_ANGLE)
        {
            turn_candidates[starting_at].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at]),
                DirectionModifier::SlightLeft};
            turn_candidates[starting_at + 1].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at + 1]),
                DirectionModifier::Left};
            turn_candidates[starting_at + 2].instruction = {
                findBasicTurnType(via_edge, turn_candidates[starting_at + 2]),
                DirectionModifier::SharpLeft};
        }
        else if (turn_candidates[starting_at].valid && turn_candidates[starting_at + 1].valid &&
                 turn_candidates[starting_at + 2].valid &&
                 ((first_direction == second_direction && second_direction == third_direction) ||
                  (third_direction == second_direction &&
                   angularDeviation(turn_candidates[starting_at].angle,
                                    turn_candidates[starting_at + 1].angle) < GROUP_ANGLE) ||
                  (second_direction == first_direction &&
                   angularDeviation(turn_candidates[starting_at + 1].angle,
                                    turn_candidates[starting_at + 2].angle) < GROUP_ANGLE)))
        {
            turn_candidates[starting_at].instruction = {
                detail::isRampClass(turn_candidates[starting_at].eid, node_based_graph) ? FirstRamp
                                                                                        : FirstTurn,
                second_direction};
            turn_candidates[starting_at + 1].instruction = {
                detail::isRampClass(turn_candidates[starting_at + 1].eid, node_based_graph)
                    ? SecondRamp
                    : SecondTurn,
                second_direction};
            turn_candidates[starting_at + 2].instruction = {
                detail::isRampClass(turn_candidates[starting_at + 2].eid, node_based_graph)
                    ? ThirdRamp
                    : ThirdTurn,
                second_direction};
        }
        else if (turn_candidates[starting_at].valid && turn_candidates[starting_at + 1].valid &&
                 turn_candidates[starting_at + 2].valid &&
                 ((third_direction == second_direction &&
                   angularDeviation(turn_candidates[starting_at].angle,
                                    turn_candidates[starting_at + 1].angle) >= GROUP_ANGLE) ||
                  (second_direction == first_direction &&
                   angularDeviation(turn_candidates[starting_at + 1].angle,
                                    turn_candidates[starting_at + 2].angle) >= GROUP_ANGLE)))
        {
            // conflict one side with an additional very sharp turn
            if (angularDeviation(turn_candidates[starting_at + 1].angle,
                                 turn_candidates[starting_at + 2].angle) >= GROUP_ANGLE)
            {
                handleDistinctConflict(via_edge, turn_candidates[starting_at + 1],
                                       turn_candidates[starting_at]);
                turn_candidates[starting_at + 2].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at + 2]), third_direction};
            }
            else
            {
                turn_candidates[starting_at].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at]), first_direction};
                handleDistinctConflict(via_edge, turn_candidates[starting_at + 2],
                                       turn_candidates[starting_at + 1]);
            }
        }

        else if ((first_direction == second_direction &&
                  turn_candidates[starting_at].valid != turn_candidates[starting_at + 1].valid) ||
                 (second_direction == third_direction &&
                  turn_candidates[starting_at + 1].valid != turn_candidates[starting_at + 2].valid))
        {
            // no conflict, due to conflict being restricted to valid/invalid
            if (turn_candidates[starting_at].valid)
                turn_candidates[starting_at].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at]), first_direction};
            if (turn_candidates[starting_at + 1].valid)
                turn_candidates[starting_at + 1].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at + 1]),
                    second_direction};
            if (turn_candidates[starting_at + 2].valid)
                turn_candidates[starting_at + 2].instruction = {
                    findBasicTurnType(via_edge, turn_candidates[starting_at + 2]), third_direction};
        }
        else
        {
            auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Reached fallback for left turns, size 3: " << std::setprecision(12)
                << toFloating(coord.lat) << " " << toFloating(coord.lon);
            for (const auto candidate : turn_candidates)
            {
                const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "\tCandidate: " << candidate.toString() << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class
                    << " At: " << localizer(node_based_graph.GetTarget(candidate.eid));
            }

            for (std::size_t i = starting_at; i < turn_candidates.size(); ++i)
                if (turn_candidates[i].valid)
                    turn_candidates[i].instruction = {
                        findBasicTurnType(via_edge, turn_candidates[i]),
                        getTurnDirection(turn_candidates[i].angle)};
        }
    }
    else if (turn_candidates.size() - starting_at == 4)
    {
        if (turn_candidates[starting_at].valid)
            turn_candidates[starting_at].instruction = {
                detail::isRampClass(turn_candidates[starting_at].eid, node_based_graph) ? FirstRamp
                                                                                        : FirstTurn,
                DirectionModifier::Left};
        if (turn_candidates[starting_at + 1].valid)
            turn_candidates[starting_at + 1].instruction = {
                detail::isRampClass(turn_candidates[starting_at + 1].eid, node_based_graph)
                    ? SecondRamp
                    : SecondTurn,
                DirectionModifier::Left};
        if (turn_candidates[starting_at + 2].valid)
            turn_candidates[starting_at + 2].instruction = {
                detail::isRampClass(turn_candidates[starting_at + 2].eid, node_based_graph)
                    ? ThirdRamp
                    : ThirdTurn,
                DirectionModifier::Left};
        if (turn_candidates[starting_at + 3].valid)
            turn_candidates[starting_at + 3].instruction = {
                detail::isRampClass(turn_candidates[starting_at + 3].eid, node_based_graph)
                    ? FourthRamp
                    : FourthTurn,
                DirectionModifier::Left};
    }
    else
    {
        for (auto &candidate : turn_candidates)
        {
            if (!candidate.valid)
                continue;
            candidate.instruction = {detail::isRampClass(candidate.eid, node_based_graph) ? Ramp
                                                                                          : Turn,
                                     getTurnDirection(candidate.angle)};
        }
        /*
        auto coord = localizer(node_based_graph.GetTarget(via_edge));
        util::SimpleLogger().Write(logWARNING)
            << "Reached fallback for left turns (" << starting_at << ") " << std::setprecision(12)
            << toFloating(coord.lat) << " " << toFloating(coord.lon);
        for (const auto candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
            util::SimpleLogger().Write(logWARNING)
                << "\tCandidate: " << candidate.toString() << " Name: " << out_data.name_id
                << " Road Class: " << (int)out_data.road_classification.road_class
                << " At: " << localizer(node_based_graph.GetTarget(candidate.eid));
        }
        */
    }
    return turn_candidates;
}

// can only assign three turns
std::vector<TurnCandidate>
TurnAnalysis::assignRightTurns(const EdgeID via_edge,
                               std::vector<TurnCandidate> turn_candidates,
                               const std::size_t up_to) const
{
    BOOST_ASSERT(up_to <= turn_candidates.size());
    const auto count_valid = [&turn_candidates, up_to]()
    {
        std::size_t count = 0;
        for (std::size_t i = 1; i < up_to; ++i)
            if (turn_candidates[i].valid)
                ++count;
        return count;
    };
    if (up_to <= 1 || count_valid() == 0)
        return turn_candidates;
    // handle single turn
    if (up_to == 2)
    {
        if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) > NARROW_TURN_ANGLE &&
            angularDeviation(turn_candidates[1].angle, 0) > NARROW_TURN_ANGLE)
        {
            // assign left turn
            turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                              DirectionModifier::Right};
        }
        else if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) <= NARROW_TURN_ANGLE)
        {
            turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                              DirectionModifier::SlightRight};
        }
        else
        {
            turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                              DirectionModifier::SharpRight};
        }
    }
    else if (up_to == 3)
    {
        const auto first_direction = getTurnDirection(turn_candidates[1].angle);
        const auto second_direction = getTurnDirection(turn_candidates[2].angle);
        if (first_direction == second_direction)
        {
            // conflict
            handleDistinctConflict(via_edge, turn_candidates[2], turn_candidates[1]);
        }
        else
        {
            turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                              first_direction};
            turn_candidates[2].instruction = {findBasicTurnType(via_edge, turn_candidates[2]),
                                              second_direction};
        }
    }
    else if (up_to == 4)
    {
        const auto first_direction = getTurnDirection(turn_candidates[1].angle);
        const auto second_direction = getTurnDirection(turn_candidates[2].angle);
        const auto third_direction = getTurnDirection(turn_candidates[3].angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            if (turn_candidates[1].valid)
                turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                                  first_direction};
            if (turn_candidates[2].valid)
                turn_candidates[2].instruction = {findBasicTurnType(via_edge, turn_candidates[2]),
                                                  second_direction};
            if (turn_candidates[3].valid)
                turn_candidates[3].instruction = {findBasicTurnType(via_edge, turn_candidates[3]),
                                                  third_direction};
        }
        else if (2 >=
                 (turn_candidates[1].valid + turn_candidates[2].valid + turn_candidates[3].valid))
        {
            // at least a single invalid
            if (!turn_candidates[3].valid)
            {
                handleDistinctConflict(via_edge, turn_candidates[2], turn_candidates[1]);
            }
            else if (!turn_candidates[1].valid)
            {
                handleDistinctConflict(via_edge, turn_candidates[3], turn_candidates[2]);
            }
            else // handles one-valid as well as two valid (1,3)
            {
                handleDistinctConflict(via_edge, turn_candidates[3], turn_candidates[1]);
            }
        }

        else if (turn_candidates[1].valid && turn_candidates[2].valid && turn_candidates[3].valid &&
                 angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >=
                     NARROW_TURN_ANGLE &&
                 angularDeviation(turn_candidates[2].angle, turn_candidates[3].angle) >=
                     NARROW_TURN_ANGLE)
        {
            turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                              DirectionModifier::SharpRight};
            turn_candidates[2].instruction = {findBasicTurnType(via_edge, turn_candidates[2]),
                                              DirectionModifier::Right};
            turn_candidates[3].instruction = {findBasicTurnType(via_edge, turn_candidates[3]),
                                              DirectionModifier::SlightRight};
        }
        else if (turn_candidates[1].valid && turn_candidates[2].valid && turn_candidates[3].valid &&
                 ((first_direction == second_direction && second_direction == third_direction) ||
                  (first_direction == second_direction &&
                   angularDeviation(turn_candidates[2].angle, turn_candidates[3].angle) <
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) <
                       GROUP_ANGLE)))
        {
            turn_candidates[1].instruction = {
                detail::isRampClass(turn_candidates[1].eid, node_based_graph) ? ThirdRamp
                                                                              : ThirdTurn,
                second_direction};
            turn_candidates[2].instruction = {
                detail::isRampClass(turn_candidates[2].eid, node_based_graph) ? SecondRamp
                                                                              : SecondTurn,
                second_direction};
            turn_candidates[3].instruction = {
                detail::isRampClass(turn_candidates[3].eid, node_based_graph) ? FirstRamp
                                                                              : FirstTurn,
                second_direction};
        }
        else if (turn_candidates[1].valid && turn_candidates[2].valid && turn_candidates[3].valid &&
                 ((first_direction == second_direction &&
                   angularDeviation(turn_candidates[2].angle, turn_candidates[3].angle) >=
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(turn_candidates[1].angle, turn_candidates[2].angle) >=
                       GROUP_ANGLE)))
        {
            if (angularDeviation(turn_candidates[2].angle, turn_candidates[3].angle) >= GROUP_ANGLE)
            {
                handleDistinctConflict(via_edge, turn_candidates[2], turn_candidates[1]);
                turn_candidates[3].instruction = {findBasicTurnType(via_edge, turn_candidates[3]),
                                                  third_direction};
            }
            else
            {
                turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                                  first_direction};
                handleDistinctConflict(via_edge, turn_candidates[3], turn_candidates[2]);
            }
        }
        else if ((first_direction == second_direction &&
                  turn_candidates[1].valid != turn_candidates[2].valid) ||
                 (second_direction == third_direction &&
                  turn_candidates[2].valid != turn_candidates[3].valid))
        {
            if (turn_candidates[1].valid)
                turn_candidates[1].instruction = {findBasicTurnType(via_edge, turn_candidates[1]),
                                                  first_direction};
            if (turn_candidates[2].valid)
                turn_candidates[2].instruction = {findBasicTurnType(via_edge, turn_candidates[2]),
                                                  second_direction};
            if (turn_candidates[3].valid)
                turn_candidates[3].instruction = {findBasicTurnType(via_edge, turn_candidates[3]),
                                                  third_direction};
        }
        else
        {
            auto coord = localizer(node_based_graph.GetTarget(via_edge));
            util::SimpleLogger().Write(logWARNING)
                << "Reached fallback for right turns, size 3: " << std::setprecision(12)
                << toFloating(coord.lat) << " " << toFloating(coord.lon) << " Valids: "
                << (turn_candidates[1].valid + turn_candidates[2].valid + turn_candidates[3].valid);
            for (const auto candidate : turn_candidates)
            {
                const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "\tCandidate: " << candidate.toString() << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class
                    << " At: " << localizer(node_based_graph.GetTarget(candidate.eid));
            }

            for (std::size_t i = 1; i < up_to; ++i)
                if (turn_candidates[i].valid)
                    turn_candidates[i].instruction = {
                        findBasicTurnType(via_edge, turn_candidates[i]),
                        getTurnDirection(turn_candidates[i].angle)};
        }
    }
    else if (up_to == 5)
    {
        if (turn_candidates[4].valid)
            turn_candidates[4].instruction = {
                detail::isRampClass(turn_candidates[4].eid, node_based_graph) ? FirstRamp
                                                                              : FirstTurn,
                DirectionModifier::Right};
        if (turn_candidates[3].valid)
            turn_candidates[3].instruction = {
                detail::isRampClass(turn_candidates[3].eid, node_based_graph) ? SecondRamp
                                                                              : SecondTurn,
                DirectionModifier::Right};
        if (turn_candidates[2].valid)
            turn_candidates[2].instruction = {
                detail::isRampClass(turn_candidates[2].eid, node_based_graph) ? ThirdRamp
                                                                              : ThirdTurn,
                DirectionModifier::Right};
        if (turn_candidates[1].valid)
            turn_candidates[1].instruction = {
                detail::isRampClass(turn_candidates[1].eid, node_based_graph) ? FourthRamp
                                                                              : FourthTurn,
                DirectionModifier::Right};
    }
    else
    {
        for (std::size_t i = 1; i < up_to; ++i)
        {
            auto &candidate = turn_candidates[i];
            if (!candidate.valid)
                continue;
            candidate.instruction = {detail::isRampClass(candidate.eid, node_based_graph) ? Ramp
                                                                                          : Turn,
                                     getTurnDirection(candidate.angle)};
        }

        /*
        auto coord = localizer(node_based_graph.GetTarget(via_edge));
        util::SimpleLogger().Write(logWARNING)
            << "Reached fallback for right turns (" << up_to << ") " << std::setprecision(12)
            << toFloating(coord.lat) << " " << toFloating(coord.lon);
        for (const auto candidate : turn_candidates)
        {
            const auto &out_data = node_based_graph.GetEdgeData(candidate.eid);
            util::SimpleLogger().Write(logWARNING)
                << "\tCandidate: " << candidate.toString() << " Name: " << out_data.name_id
                << " Road Class: " << (int)out_data.road_classification.road_class
                << " At: " << localizer(node_based_graph.GetTarget(candidate.eid));
        }
        */
    }
    return turn_candidates;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
