#ifndef OSRM_GUIDANCE_DRIVEWAY_HANDLER_HPP
#define OSRM_GUIDANCE_DRIVEWAY_HANDLER_HPP

#include "guidance/intersection_handler.hpp"

namespace osrm
{
namespace guidance
{

// Intersection handlers deal with all issues related to intersections.
class DrivewayHandler final : public IntersectionHandler
{
  public:
    DrivewayHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                    const extractor::EdgeBasedNodeDataContainer &node_data_container,
                    const std::vector<util::Coordinate> &coordinates,
                    const extractor::CompressedEdgeContainer &compressed_geometries,
                    const extractor::RestrictionMap &node_restriction_map,
                    const std::unordered_set<NodeID> &barrier_nodes,
                    const extractor::TurnLanesIndexedArray &turn_lanes_data,
                    const extractor::NameTable &name_table,
                    const extractor::SuffixTable &street_name_suffix_table);

    ~DrivewayHandler() override final = default;

    // check whether the handler can actually handle the intersection
    bool canProcess(const NodeID nid,
                    const EdgeID via_eid,
                    const Intersection &intersection) const override final;

    // process the intersection
    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;
};

} // namespace guidance
} // namespace osrm

#endif /* OSRM_GUIDANCE_DRIVEWAY_HANDLER_HPP */
