#ifndef OSRM_EXTRACTOR_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_

#include "extractor/travel_mode.hpp"
#include "guidance/constants.hpp"
#include "guidance/intersection.hpp"
#include "guidance/intersection_handler.hpp"
#include "util/node_based_graph.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{

// Suppresses instructions for certain modes.
// Think: ferry route. This handler suppresses all instructions while on the ferry route.
// We don't want to announce "Turn Right", "Turn Left" while on ferries, as one example.
class SuppressModeHandler final : public IntersectionHandler
{
  public:
    SuppressModeHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                        const EdgeBasedNodeDataContainer &node_data_container,
                        const std::vector<util::Coordinate> &coordinates,
                        const extractor::CompressedEdgeContainer &compressed_geometries,
                        const RestrictionMap &node_restriction_map,
                        const std::unordered_set<NodeID> &barrier_nodes,
                        const guidance::TurnLanesIndexedArray &turn_lanes_data,
                        const util::NameTable &name_table,
                        const SuffixTable &street_name_suffix_table);

    ~SuppressModeHandler() override final = default;

    bool canProcess(const NodeID nid,
                    const EdgeID via_eid,
                    const Intersection &intersection) const override final;

    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;
};

} // namespace osrm
} // namespace extractor
} // namespace guidance

#endif /* OSRM_EXTRACTOR_GUIDANCE_SUPPRESS_MODE_HANDLER_HPP_ */
