#ifndef OSRM_GUIDANCE_TURN_LANE_HANDLER_HPP_
#define OSRM_GUIDANCE_TURN_LANE_HANDLER_HPP_

#include "extractor/name_table.hpp"
#include "extractor/query_node.hpp"
#include "extractor/turn_lane_types.hpp"

#include "guidance/intersection.hpp"
#include "guidance/turn_analysis.hpp"
#include "guidance/turn_lane_data.hpp"

#include "util/guidance/turn_lanes.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace osrm::guidance::lanes
{

namespace
{
using TurnLaneScenario = enum TurnLaneScenario {
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
};

} // namespace

class TurnLaneHandler
{
    using UpgradableMutex = boost::interprocess::interprocess_upgradable_mutex;
    using ScopedReaderLock = boost::interprocess::sharable_lock<UpgradableMutex>;
    using ScopedWriterLock = boost::interprocess::scoped_lock<UpgradableMutex>;

  public:
    using LaneDataVector = std::vector<TurnLaneData>;

    TurnLaneHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                    const extractor::EdgeBasedNodeDataContainer &node_data_container,
                    const std::vector<util::Coordinate> &node_coordinates,
                    const extractor::CompressedEdgeContainer &compressed_geometries,
                    const extractor::RestrictionMap &node_restriction_map,
                    const std::unordered_set<NodeID> &barrier_nodes,
                    const extractor::TurnLanesIndexedArray &turn_lanes_data,
                    extractor::LaneDescriptionMap &lane_description_map,
                    const TurnAnalysis &turn_analysis,
                    util::guidance::LaneDataIdMap &id_map);

    ~TurnLaneHandler();

    [[nodiscard]] Intersection
    assignTurnLanes(const NodeID at, const EdgeID via_edge, Intersection intersection);

  private:
    mutable std::atomic<std::size_t> count_handled;
    mutable std::atomic<std::size_t> count_called;
    // we need to be able to look at previous intersections to, in some cases, find the correct turn
    // lanes for a turn
    const util::NodeBasedDynamicGraph &node_based_graph;
    const extractor::EdgeBasedNodeDataContainer &node_data_container;
    const std::vector<util::Coordinate> &node_coordinates;
    const extractor::CompressedEdgeContainer &compressed_geometries;
    const extractor::RestrictionMap &node_restriction_map;
    const std::unordered_set<NodeID> &barrier_nodes;
    const extractor::TurnLanesIndexedArray &turn_lanes_data;

    std::vector<std::uint32_t> turn_lane_offsets;
    std::vector<extractor::TurnLaneType::Mask> turn_lane_masks;
    extractor::LaneDescriptionMap &lane_description_map;
    const TurnAnalysis &turn_analysis;
    util::guidance::LaneDataIdMap &id_map;

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
    [[nodiscard]] Intersection simpleMatchTuplesToTurns(Intersection intersection,
                                                        const LaneDataVector &lane_data,
                                                        const LaneDescriptionID lane_string_id);

    // partition lane data into lane data relevant at current turn and at next turn
    [[nodiscard]] std::pair<TurnLaneHandler::LaneDataVector, TurnLaneHandler::LaneDataVector>
    partitionLaneData(const NodeID at,
                      LaneDataVector turn_lane_data,
                      const Intersection &intersection) const;

    // Sliproad turns have a separated lane to the right/left of other depicted lanes. These lanes
    // are not necessarily separated clearly from the rest of the way. As a result, we combine both
    // lane entries for our output, while performing the matching with the separated lanes only.
    [[nodiscard]] Intersection handleSliproadTurn(Intersection intersection,
                                                  const LaneDescriptionID lane_description_id,
                                                  LaneDataVector lane_data,
                                                  const Intersection &previous_intersection);

    // get the lane data for an intersection
    void extractLaneData(const EdgeID via_edge,
                         LaneDescriptionID &lane_description_id,
                         LaneDataVector &lane_data) const;
};

} // namespace osrm::guidance::lanes

#endif // OSRM_EXTRACTOR_GUIDANCE_TURN_LANE_HANDLER_HPP_
