#ifndef OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_HANDLER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_HANDLER_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/toolkit.hpp"
#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/turn_lane_data.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/query_node.hpp"

#include "util/guidance/turn_lanes.hpp"
#include "util/name_table.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace osrm
{
namespace extractor
{
namespace guidance
{

// Given an Intersection, the graph to access the data and  the turn lanes, the turn lane matcher
// assigns appropriate turn tupels to the different turns.
namespace lanes
{
class TurnLaneHandler
{
    typedef enum TurnLaneScenario {
        SIMPLE,             // a straightforward assignment
        PARTITION_LOCAL,    // an assignment that requires partitioning, using local turns
        SIMPLE_PREVIOUS,    // an assignemtnn using the turns specified at the previous road (e.g.
                            // traffic light, lanes not drawn up to the intersection)
        PARTITION_PREVIOUS, // a set of lanes on a turn with a traffic island. The lanes for the
                            // turn end at the previous turn (parts of it remain valid without being
                            // shown again)
        SLIPROAD, // Sliproads are simple assignments that, for better visual representation should
                  // include turns from other roads in their listings
        MERGE,    // Merging Lanes
        NONE,     // not a turn lane scenario at all
        INVALID,  // some error might have occurred
        UNKNOWN,  // UNKNOWN describes all cases that we are currently not able to handle
        NUM_SCENARIOS
    } TurnLaneScenario;

    const constexpr static char *scenario_names[TurnLaneScenario::NUM_SCENARIOS] = {
        "Simple",
        "Partition Local",
        "Simple Previous",
        "Partition Previous",
        "Sliproad",
        "None",
        "Invalid",
        "Unknown"};

  public:
    typedef std::vector<TurnLaneData> LaneDataVector;

    TurnLaneHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                    std::vector<std::uint32_t> &turn_lane_offsets,
                    std::vector<TurnLaneType::Mask> &turn_lane_masks,
                    LaneDescriptionMap &lane_description_map,
                    const std::vector<QueryNode> &node_info_list,
                    const TurnAnalysis &turn_analysis,
                    LaneDataIdMap &id_map);

    ~TurnLaneHandler();

    Intersection assignTurnLanes(const NodeID at, const EdgeID via_edge, Intersection intersection);

  private:
    unsigned *count_handled;
    unsigned *count_called;
    // we need to be able to look at previous intersections to, in some cases, find the correct turn
    // lanes for a turn
    const util::NodeBasedDynamicGraph &node_based_graph;
    std::vector<std::uint32_t> &turn_lane_offsets;
    std::vector<TurnLaneType::Mask> &turn_lane_masks;
    LaneDescriptionMap &lane_description_map;
    const std::vector<QueryNode> &node_info_list;
    const TurnAnalysis &turn_analysis;
    LaneDataIdMap &id_map;

    // Find out which scenario we have to handle
    TurnLaneScenario deduceScenario(const NodeID at,
                                    const EdgeID via_edge,
                                    const Intersection &intersection,
                                    // Output Parameters to reduce repeated creation
                                    LaneDescriptionID &lane_description_id,
                                    LaneDataVector &lane_data,
                                    NodeID &previous_node,
                                    EdgeID &previous_id,
                                    Intersection &previous_intersection,
                                    LaneDataVector &previous_lane_data,
                                    LaneDescriptionID &previous_description_id);

    // check whether we can handle an intersection
    bool isSimpleIntersection(const LaneDataVector &turn_lane_data,
                              const Intersection &intersection) const;

    // in case of a simple intersection, assign the lane entries
    Intersection simpleMatchTuplesToTurns(Intersection intersection,
                                          const LaneDataVector &lane_data,
                                          const LaneDescriptionID lane_string_id);

    // partition lane data into lane data relevant at current turn and at next turn
    std::pair<TurnLaneHandler::LaneDataVector, TurnLaneHandler::LaneDataVector> partitionLaneData(
        const NodeID at, LaneDataVector turn_lane_data, const Intersection &intersection) const;

    // Sliproad turns have a separated lane to the right/left of other depicted lanes. These lanes
    // are not necessarily separated clearly from the rest of the way. As a result, we combine both
    // lane entries for our output, while performing the matching with the separated lanes only.
    Intersection handleSliproadTurn(Intersection intersection,
                                    const LaneDescriptionID lane_description_id,
                                    LaneDataVector lane_data,
                                    const Intersection &previous_intersection,
                                    const LaneDescriptionID &previous_lane_description_id,
                                    const LaneDataVector &previous_lane_data);

    // get the lane data for an intersection
    void extractLaneData(const EdgeID via_edge,
                         LaneDescriptionID &lane_description_id,
                         LaneDataVector &lane_data) const;
};

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_HANDLER_HPP_
