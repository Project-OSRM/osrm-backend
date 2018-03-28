#ifndef OSRM_GUIDANCE_ROUNDABOUT_HANDLER_HPP_
#define OSRM_GUIDANCE_ROUNDABOUT_HANDLER_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/intersection/coordinate_extractor.hpp"
#include "extractor/name_table.hpp"
#include "extractor/query_node.hpp"

#include "guidance/intersection.hpp"
#include "guidance/intersection_handler.hpp"
#include "guidance/is_through_street.hpp"
#include "guidance/roundabout_type.hpp"

#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <unordered_set>
#include <vector>

namespace osrm
{
namespace guidance
{

namespace detail
{
struct RoundaboutFlags
{
    bool on_roundabout;
    bool can_enter;
    bool can_exit_separately;
};
} // namespace detail

// The roundabout handler processes all roundabout related instructions.
// It performs both the distinction between rotaries and roundabouts and
// assigns appropriate entry/exit instructions.
class RoundaboutHandler : public IntersectionHandler
{
  public:
    RoundaboutHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                      const extractor::EdgeBasedNodeDataContainer &node_data_container,
                      const std::vector<util::Coordinate> &coordinates,
                      const extractor::CompressedEdgeContainer &compressed_geometries,
                      const extractor::RestrictionMap &node_restriction_map,
                      const std::unordered_set<NodeID> &barrier_nodes,
                      const extractor::TurnLanesIndexedArray &turn_lanes_data,
                      const extractor::NameTable &name_table,
                      const extractor::SuffixTable &street_name_suffix_table);

    ~RoundaboutHandler() override final = default;

    // check whether the handler can actually handle the intersection
    bool canProcess(const NodeID from_nid,
                    const EdgeID via_eid,
                    const Intersection &intersection) const override final;

    // process the intersection
    Intersection operator()(const NodeID from_nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;

  private:
    detail::RoundaboutFlags getRoundaboutFlags(const NodeID from_nid,
                                               const EdgeID via_eid,
                                               const Intersection &intersection) const;

    // decide whether we lookk at a roundabout or a rotary
    RoundaboutType getRoundaboutType(const NodeID nid) const;

    // TODO handle bike/walk cases that allow crossing a roundabout!
    // Processing of roundabouts
    // Produces instructions to enter/exit a roundabout or to stay on it.
    // Performs the distinction between roundabout and rotaries.
    Intersection handleRoundabouts(const RoundaboutType roundabout_type,
                                   const EdgeID via_edge,
                                   const bool on_roundabout,
                                   const bool can_exit_roundabout,
                                   Intersection intersection) const;

    bool
    qualifiesAsRoundaboutIntersection(const std::unordered_set<NodeID> &roundabout_nodes) const;

    const extractor::intersection::CoordinateExtractor coordinate_extractor;
};

} // namespace guidance
} // namespace osrm

#endif /*OSRM_GUIDANCE_ROUNDABOUT_HANDLER_HPP_*/
