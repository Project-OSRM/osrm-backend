#ifndef OSRM_EXTRACTOR_TURN_ANALYSIS
#define OSRM_EXTRACTOR_TURN_ANALYSIS

#include "extractor/guidance/turn_classification.hpp"
#include "extractor/guidance/toolkit.hpp"
#include "extractor/restriction_map.hpp"
#include "extractor/compressed_edge_container.hpp"

#include "util/name_table.hpp"

#include <cstdint>

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_set>

namespace osrm
{
namespace extractor
{
namespace guidance
{

// What is exposed to the outside
struct TurnOperation final
{
    EdgeID eid;
    double angle;
    TurnInstruction instruction;
};

// For the turn analysis, we require a full list of all connected roads to determine the outcome.
// Invalid turns can influence the perceived angles
//
// aaa(2)aa
//          a - bbbbb
// aaa(1)aa
//
// will not be perceived as a turn from (1) -> b, and as a U-turn from (1) -> (2).
// In addition, they can influence whether a turn is obvious or not.
struct ConnectedRoad final
{
    ConnectedRoad(const TurnOperation turn, const bool entry_allowed = false);

    TurnOperation turn;
    bool entry_allowed; // a turn may be relevant to good instructions, even if we cannot take
                        // the road

    std::string toString() const
    {
        std::string result = "[connection] ";
        result += std::to_string(turn.eid);
        result += " allows entry: ";
        result += std::to_string(entry_allowed);
        result += " angle: ";
        result += std::to_string(turn.angle);
        result += " instruction: ";
        result += std::to_string(static_cast<std::int32_t>(turn.instruction.type)) + " " +
                  std::to_string(static_cast<std::int32_t>(turn.instruction.direction_modifier));
        return result;
    }
};

class TurnAnalysis
{

  public:
    TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                 const std::vector<QueryNode> &node_info_list,
                 const RestrictionMap &restriction_map,
                 const std::unordered_set<NodeID> &barrier_nodes,
                 const CompressedEdgeContainer &compressed_edge_container,
                 const util::NameTable &name_table);

    // the entry into the turn analysis
    std::vector<TurnOperation> getTurns(const NodeID from_node, const EdgeID via_eid) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const std::vector<QueryNode> &node_info_list;
    const RestrictionMap &restriction_map;
    const std::unordered_set<NodeID> &barrier_nodes;
    const CompressedEdgeContainer &compressed_edge_container;
    const util::NameTable &name_table;

    // Check for restrictions/barriers and generate a list of valid and invalid turns present at
    // the
    // node reached
    // from `from_node` via `via_eid`
    // The resulting candidates have to be analysed for their actual instructions later on.
    std::vector<ConnectedRoad> getConnectedRoads(const NodeID from_node,
                                                 const EdgeID via_eid) const;

    // Merge segregated roads to omit invalid turns in favor of treating segregated roads as
    // one.
    // This function combines roads the following way:
    //
    //     *                           *
    //     *        is converted to    *
    //   v   ^                         +
    //   v   ^                         +
    //
    // The treatment results in a straight turn angle of 180º rather than a turn angle of approx
    // 160
    std::vector<ConnectedRoad> mergeSegregatedRoads(std::vector<ConnectedRoad> intersection) const;

    // TODO distinguish roundabouts and rotaries
    // TODO handle bike/walk cases that allow crossing a roundabout!

    // Processing of roundabouts
    // Produces instructions to enter/exit a roundabout or to stay on it.
    // Performs the distinction between roundabout and rotaries.
    std::vector<ConnectedRoad> handleRoundabouts(const EdgeID via_edge,
                                                 const bool on_roundabout,
                                                 const bool can_exit_roundabout,
                                                 std::vector<ConnectedRoad> intersection) const;

    // Indicates a Junction containing a motoryway
    bool isMotorwayJunction(const EdgeID via_edge,
                            const std::vector<ConnectedRoad> &intersection) const;

    // Decide whether a turn is a turn or a ramp access
    TurnType findBasicTurnType(const EdgeID via_edge, const ConnectedRoad &candidate) const;

    // Get the Instruction for an obvious turn
    // Instruction will be a silent instruction
    TurnInstruction getInstructionForObvious(const std::size_t number_of_candidates,
                                             const EdgeID via_edge,
                                             const ConnectedRoad &candidate) const;

    // Helper Function that decides between NoTurn or NewName
    TurnInstruction
    noTurnOrNewName(const NodeID from, const EdgeID via_edge, const ConnectedRoad &candidate) const;

    // Basic Turn Handling

    // Dead end.
    std::vector<ConnectedRoad> handleOneWayTurn(std::vector<ConnectedRoad> intersection) const;

    // Mode Changes, new names...
    std::vector<ConnectedRoad> handleTwoWayTurn(const EdgeID via_edge,
                                                std::vector<ConnectedRoad> intersection) const;

    // Forks, T intersections and similar
    std::vector<ConnectedRoad> handleThreeWayTurn(const EdgeID via_edge,
                                                  std::vector<ConnectedRoad> intersection) const;

    // Handling of turns larger then degree three
    std::vector<ConnectedRoad> handleComplexTurn(const EdgeID via_edge,
                                                 std::vector<ConnectedRoad> intersection) const;

    // Any Junction containing motorways
    std::vector<ConnectedRoad> handleMotorwayJunction(
        const EdgeID via_edge, std::vector<ConnectedRoad> intersection) const;

    std::vector<ConnectedRoad> handleFromMotorway(const EdgeID via_edge,
                                                  std::vector<ConnectedRoad> intersection) const;

    std::vector<ConnectedRoad> handleMotorwayRamp(const EdgeID via_edge,
                                                  std::vector<ConnectedRoad> intersection) const;

    // Utility function, setting basic turn types. Prepares for normal turn handling.
    std::vector<ConnectedRoad> setTurnTypes(const NodeID from,
                                            const EdgeID via_edge,
                                            std::vector<ConnectedRoad> intersection) const;

    // Assignment of specific turn types
    void assignFork(const EdgeID via_edge, ConnectedRoad &left, ConnectedRoad &right) const;
    void assignFork(const EdgeID via_edge,
                    ConnectedRoad &left,
                    ConnectedRoad &center,
                    ConnectedRoad &right) const;

    void
    handleDistinctConflict(const EdgeID via_edge, ConnectedRoad &left, ConnectedRoad &right) const;

    // Type specific fallbacks
    std::vector<ConnectedRoad>
    fallbackTurnAssignmentMotorway(std::vector<ConnectedRoad> intersection) const;

    // Classification
    std::size_t findObviousTurn(const EdgeID via_edge,
                                const std::vector<ConnectedRoad> &intersection) const;
    std::pair<std::size_t, std::size_t>
    findFork(const std::vector<ConnectedRoad> &intersection) const;

    std::vector<ConnectedRoad> assignLeftTurns(const EdgeID via_edge,
                                               std::vector<ConnectedRoad> intersection,
                                               const std::size_t starting_at) const;
    std::vector<ConnectedRoad> assignRightTurns(const EdgeID via_edge,
                                                std::vector<ConnectedRoad> intersection,
                                                const std::size_t up_to) const;

}; // class TurnAnalysis

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_TURN_ANALYSIS
