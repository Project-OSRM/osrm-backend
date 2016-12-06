#ifndef OSRM_EXTRACTOR_GUIDANCE_MERGEABLE_ROADS
#define OSRM_EXTRACTOR_GUIDANCE_MERGEABLE_ROADS

#include "extractor/guidance/intersection.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

namespace osrm
{

//FWD declarations
namespace util
{
    class NameTable;
} // namespace util

namespace extractor
{

struct QueryNode;
class SuffixTable;

namespace guidance
{
class IntersectionGenerator;
class CoordinateExtractor;


class MergableRoadDetector
{
  public:
    // in case we have to change the mode we are operating on
    using MergableRoadData = IntersectionShapeData;

    MergableRoadDetector(const util::NodeBasedDynamicGraph &node_based_graph,
                         const std::vector<QueryNode> &node_coordinates,
                         const IntersectionGenerator &intersection_generator,
                         const CoordinateExtractor &coordinate_extractor,
                         const util::NameTable &name_table,
                         const SuffixTable &street_name_suffix_table);

    // OSM ways tend to be modelled as separate ways for different directions. This is often due to
    // small gras strips in the middle between the two directions or due to pedestrian islands at
    // intersections.
    //
    // To reduce unnecessary information due to these artificial intersections (which are not
    // actually perceived as such) we try and merge these for our internal representation to both
    // get better perceived turn angles and get a better reprsentation of our intersections in
    // general.
    //
    //         i   h                                             i,h
    //         |   |                                              |
    //         |   |                                              |
    // b - - -   v   - - - g                                      |
    //         > a <               is transformed into: b,c - - - a - - -  g,f
    // c - - -   ^   - - - f                                      |
    //         |   |                                              |
    //         |   |                                              |
    //         d   e                                             d,e
    bool CanMergeRoad(const NodeID intersection_node,
                      const MergableRoadData &lhs,
                      const MergableRoadData &rhs) const;

    // check if a road cannot influence the merging of the other. This is necessary to prevent
    // situations with more than two roads that could participate in a merge
    bool IsDistinctFrom(const MergableRoadData &lhs, const MergableRoadData &rhs) const;

  private:
    // check if two name ids can be seen as identical (in presence of refs/others)
    // in our case this translates into no name announcement in either direction (lhs->rhs and
    // rhs->lhs)
    bool HaveIdenticalNames(const NameID lhs, const NameID rhs) const;

    // When it comes to merging roads, we need to find out if two ways actually represent the
    // same road. This check tries to identify roads which are the same road in opposite directions
    bool EdgeDataSupportsMerge(const util::NodeBasedEdgeData &lhs_edge_data,
                               const util::NodeBasedEdgeData &rhs_edge_data) const;

    // Detect traffic loops.
    // Since OSRM cannot handle loop edges, we cannot directly see a connection between a node and
    // itself. We need to skip at least a single node in between.
    bool IsTrafficLoop(const NodeID intersection_node, const MergableRoadData &road) const;

    // Detector to check if we are looking at roads splitting up just prior to entering an
    // intersection:
    //
    //        c
    //      / |
    // a -<   |
    //      \ |
    //        b
    //
    // A common scheme in OSRM is that roads spit up in separate ways when approaching an
    // intersection. This detector tries to detect these narrow triangles which usually just offer a
    // small island for pedestrians in the middle.
    bool IsNarrowTriangle(const NodeID intersection_node,
                          const MergableRoadData &lhs,
                          const MergableRoadData &rhs) const;

    // Detector to check for whether two roads are following the same direction.
    // If roads don't end up right at a connected intersection, we could look at a situation like
    //
    //      __________________________
    //     /
    // ---
    //     \__________________________
    //
    // This detector tries to find out about whether two roads are parallel after the separation
    bool HaveSameDirection(const NodeID intersection_node,
                           const MergableRoadData &lhs,
                           const MergableRoadData &rhs) const;

    // Detector for small traffic islands. If a road is splitting up, just to connect again later,
    // we don't wan't to have this information within our list of intersections/possible turn
    // locations.
    //
    //     ___________
    // ---<___________>-----
    //
    //
    // Would feel just like a single straight road to a driver and should be represented as such in
    // our engine
    bool IsTrafficIsland(const NodeID intersection_node,
                         const MergableRoadData &lhs,
                         const MergableRoadData &rhs) const;

    // A negative detector, preventing a merge, trying to detect link roads between two main roads.
    //
    //  d - - - - - - - - e - f
    //             . / '
    //  a - - - b - - - - - - c
    //
    // The detector wants to prevent merges that are connected to `b-e`
    bool IsLinkRoad(const NodeID intersection_node, const MergableRoadData &road) const;

    const util::NodeBasedDynamicGraph &node_based_graph;
    const std::vector<QueryNode> &node_coordinates;
    const IntersectionGenerator &intersection_generator;
    const CoordinateExtractor &coordinate_extractor;

    // name detection
    const util::NameTable &name_table;
    const SuffixTable &street_name_suffix_table;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif
