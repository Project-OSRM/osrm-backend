#ifndef OSRM_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_
#define OSRM_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_

#include "extractor/name_table.hpp"

#include "guidance/intersection.hpp"
#include "guidance/intersection_handler.hpp"

#include "util/node_based_graph.hpp"

namespace osrm::guidance
{

// Suppresses instructions for certain modes.
// Think: ferry route. This handler suppresses all instructions while on the ferry route.
// We don't want to announce "Turn Right", "Turn Left" while on ferries, as one example.
class SuppressModeHandler final : public IntersectionHandler
{
  public:
    SuppressModeHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                        const extractor::EdgeBasedNodeDataContainer &node_data_container,
                        const std::vector<util::Coordinate> &coordinates,
                        const extractor::CompressedEdgeContainer &compressed_geometries,
                        const extractor::RestrictionMap &node_restriction_map,
                        const std::unordered_set<NodeID> &barrier_nodes,
                        const extractor::TurnLanesIndexedArray &turn_lanes_data,
                        const extractor::NameTable &name_table,
                        const extractor::SuffixTable &street_name_suffix_table);

    ~SuppressModeHandler() override final = default;

    bool canProcess(const NodeID nid,
                    const EdgeID via_eid,
                    const Intersection &intersection) const override final;

    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;
};

} // namespace osrm::guidance

#endif /* OSRM_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_ */
