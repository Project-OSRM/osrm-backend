#ifndef OSRM_GUIDANCE_SLIPROAD_HANDLER_HPP_
#define OSRM_GUIDANCE_SLIPROAD_HANDLER_HPP_

#include "extractor/name_table.hpp"

#include "guidance/intersection.hpp"
#include "guidance/intersection_handler.hpp"
#include "guidance/is_through_street.hpp"

#include "util/node_based_graph.hpp"

#include <optional>
#include <vector>

namespace osrm::guidance
{

// Intersection handlers deal with all issues related to intersections.
class SliproadHandler final : public IntersectionHandler
{
  public:
    SliproadHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                    const extractor::EdgeBasedNodeDataContainer &node_data_container,
                    const std::vector<util::Coordinate> &coordinates,
                    const extractor::CompressedEdgeContainer &compressed_geometries,
                    const extractor::RestrictionMap &node_restriction_map,
                    const std::unordered_set<NodeID> &barrier_nodes,
                    const extractor::TurnLanesIndexedArray &turn_lanes_data,
                    const extractor::NameTable &name_table,
                    const extractor::SuffixTable &street_name_suffix_table);

    ~SliproadHandler() override final = default;

    // check whether the handler can actually handle the intersection
    bool canProcess(const NodeID /*nid*/,
                    const EdgeID /*via_eid*/,
                    const Intersection & /*intersection*/) const override final;

    // process the intersection
    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;

  private:
    std::optional<std::size_t> getObviousIndexWithSliproads(const EdgeID from,
                                                            const Intersection &intersection,
                                                            const NodeID at) const;

    // Next intersection from `start` onto `onto` is too far away for a Siproad scenario
    bool nextIntersectionIsTooFarAway(const NodeID start, const EdgeID onto) const;

    // Does the road from `current` to `next` continue
    bool roadContinues(const EdgeID current, const EdgeID next) const;

    // Is the area under the triangle a valid Sliproad triangle
    bool isValidSliproadArea(const double max_area, const NodeID, const NodeID, const NodeID) const;

    // Is the Sliproad a link the both roads it shortcuts must not be links
    bool isValidSliproadLink(const extractor::intersection::IntersectionViewData &sliproad,
                             const extractor::intersection::IntersectionViewData &first,
                             const extractor::intersection::IntersectionViewData &second) const;

    // check if no mode changes are involved
    bool allSameMode(const EdgeID in_road,
                     const EdgeID sliproad_candidate,
                     const EdgeID target_road) const;

    // Could a Sliproad reach this intersection?
    static bool
    canBeTargetOfSliproad(const extractor::intersection::IntersectionView &intersection);

    // Scales a threshold based on the underlying road classification.
    // Example: a 100 m threshold for a highway if different on living streets.
    // The return value is guaranteed to not be larger than `threshold`.
    static double scaledThresholdByRoadClass(const double max_threshold,
                                             const extractor::RoadClassification &classification);

    const extractor::intersection::CoordinateExtractor coordinate_extractor;
};

} // namespace osrm::guidance

#endif /*OSRM_GUIDANCE_SLIPROAD_HANDLER_HPP_*/
