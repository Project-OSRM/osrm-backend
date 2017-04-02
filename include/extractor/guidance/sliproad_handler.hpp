#ifndef OSRM_EXTRACTOR_GUIDANCE_SLIPROAD_HANDLER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_SLIPROAD_HANDLER_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/guidance/intersection_handler.hpp"
#include "extractor/query_node.hpp"

#include "util/name_table.hpp"
#include "util/node_based_graph.hpp"

#include <vector>

#include <boost/optional.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

// Intersection handlers deal with all issues related to intersections.
// They assign appropriate turn operations to the TurnOperations.
class SliproadHandler final : public IntersectionHandler
{
  public:
    SliproadHandler(const IntersectionGenerator &intersection_generator,
                    const util::NodeBasedDynamicGraph &node_based_graph,
                    const std::vector<util::Coordinate> &coordinates,
                    const util::NameTable &name_table,
                    const SuffixTable &street_name_suffix_table);

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
    boost::optional<std::size_t> getObviousIndexWithSliproads(const EdgeID from,
                                                              const Intersection &intersection,
                                                              const NodeID at) const;

    // Next intersection from `start` onto `onto` is too far away for a Siproad scenario
    bool nextIntersectionIsTooFarAway(const NodeID start, const EdgeID onto) const;

    // Through street: does a road continue with from's name at the intersection
    bool isThroughStreet(const EdgeID from, const IntersectionView &intersection) const;

    // Does the road from `current` to `next` continue
    bool roadContinues(const EdgeID current, const EdgeID next) const;

    // Is the area under the triangle a valid Sliproad triangle
    bool isValidSliproadArea(const double max_area, const NodeID, const NodeID, const NodeID) const;

    // Is the Sliproad a link the both roads it shortcuts must not be links
    bool isValidSliproadLink(const IntersectionViewData &sliproad,
                             const IntersectionViewData &first,
                             const IntersectionViewData &second) const;

    // check if no mode changes are involved
    bool allSameMode(const EdgeID in_road,
                     const EdgeID sliproad_candidate,
                     const EdgeID target_road) const;

    // Could a Sliproad reach this intersection?
    static bool canBeTargetOfSliproad(const IntersectionView &intersection);

    // Scales a threshold based on the underlying road classification.
    // Example: a 100 m threshold for a highway if different on living streets.
    // The return value is guaranteed to not be larger than `threshold`.
    static double scaledThresholdByRoadClass(const double max_threshold,
                                             const RoadClassification &classification);
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_SLIPROAD_HANDLER_HPP_*/
