#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_NORMALIZER_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_NORMALIZER_HPP_

#include "util/typedefs.hpp"

#include "util/attributes.hpp"

#include "extractor/guidance/coordinate_extractor.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/query_node.hpp"

#include "extractor/suffix_table.hpp"
#include "util/name_table.hpp"

#include <vector>

namespace osrm
{
namespace extractor
{
namespace guidance
{

/*
 * An intersection is a central part in computing guidance decisions. However the model in OSM and
 * the view we want to use in guidance are not necessarily the same thing. We have to account for
 * some models that are chosen explicitly in OSM and that don't actually describe how a human would
 * experience an intersection.
 *
 * For example, if a small pedestrian island is located at a traffic light right in the middle of a
 * road, OSM tends to model the road as two separate ways. A human would consider these two ways a
 * single road, though. In this normalizer, we try to account for these subtle differences between
 * OSM data and human perception to improve our decision base for guidance later on.
 */
class IntersectionNormalizer
{
  public:
    IntersectionNormalizer(const util::NodeBasedDynamicGraph &node_based_graph,
                           const std::vector<extractor::QueryNode> &node_coordinates,
                           const util::NameTable &name_table,
                           const SuffixTable &street_name_suffix_table,
                           const IntersectionGenerator &intersection_generator);

    // The function takes an intersection an converts it to a `perceived` intersection which closer
    // represents how a human might experience the intersection
    OSRM_ATTR_WARN_UNUSED
    Intersection operator()(const NodeID node_at_intersection, Intersection intersection) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const std::vector<extractor::QueryNode> &node_coordinates;
    const util::NameTable &name_table;
    const SuffixTable &street_name_suffix_table;

    const IntersectionGenerator &intersection_generator;

    // check if two indices in an intersection can be seen as a single road in the perceived
    // intersection representation. See below for an example. Utility function for
    // MergeSegregatedRoads
    bool CanMerge(const NodeID intersection_node,
                  const Intersection &intersection,
                  std::size_t first_index,
                  std::size_t second_index) const;

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
    OSRM_ATTR_WARN_UNUSED
    Intersection MergeSegregatedRoads(const NodeID intersection_node,
                                      Intersection intersection) const;

    // The counterpiece to mergeSegregatedRoads. While we can adjust roads that split up at the
    // intersection itself, it can also happen that intersections are connected to joining roads.
    //
    //     *                           *
    //     *        is converted to    *
    //   v   a ---                     a ---
    //   v   ^                         +
    //   v   ^                         +
    //       b
    //
    // for the local view of b at a.
    OSRM_ATTR_WARN_UNUSED
    Intersection AdjustForJoiningRoads(const NodeID node_at_intersection,
                                       Intersection intersection) const;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_NORMALIZER_HPP_ */
