#ifndef OSRM_EXTRACTOR_GUIDANCE_NODE_BASED_GRAPH_WALKER
#define OSRM_EXTRACTOR_GUIDANCE_NODE_BASED_GRAPH_WALKER

#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <utility>

namespace osrm
{
namespace extractor
{
namespace guidance
{

/*
 * The graph hopper is a utility that lets you find certain intersections with a node based graph,
 * accumulating information along the way
 */
class NodeBasedGraphWalker
{
  public:
    NodeBasedGraphWalker(const util::NodeBasedDynamicGraph &node_based_graph,
                         const IntersectionGenerator &intersection_generator);

    /*
     * the returned node-id, edge-id are either the last ones used, just prior accumulator
     * terminating or empty if the traversal ran into a dead end. For examples of the
     * selector/accumulator look below. You can find an example for both (and the required interface
     * description). The function returns the last used `NodeID` and `EdgeID` (node just prior to
     * the last intersection and the edge it was reached by), if it wasn't stopped early (e.g. the
     * selector not provinding any further edge to traverse)
     */
    template <class accumulator_type, class selector_type>
    boost::optional<std::pair<NodeID, EdgeID>> TraverseRoad(NodeID starting_at_node_id,
                                                            EdgeID following_edge_id,
                                                            accumulator_type &accumulator,
                                                            const selector_type &selector) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const IntersectionGenerator &intersection_generator;
};

/*
 * Accumulate all coordinates following a road until we
 * Example of a possible accumulator for walking a node-based graph
 */
struct LengthLimitedCoordinateAccumulator
{
    LengthLimitedCoordinateAccumulator(
        const extractor::guidance::CoordinateExtractor &coordinate_extractor,
        const double max_length);

    /*
     * !! REQUIRED - Function for the use of TraverseRoad in the graph walker.
     * Terminate should return true if the last intersection given to accumulator is supposed to
     * stop the search. A typical example would be to find the next intersection with degree larger
     * than 2 (an actual intersection). Here you should return true if the last intersection you
     * looked at was of degree larger than 2.
     */
    bool terminate(); // true if the path has traversed enough distance

    /*
     * !! REQUIRED - Function for the use of TraverseRoad in the graph walker.
     * starting with the very first provided node and edge, the graph walker will call `update` on
     * your accumulator. Here you can choose to accumulate any data that you might want to collect /
     * update your termination criteria. The accumulator described here will extract the coordinates
     * that we see traversing `via_edge` and store them for later usage.
     */
    void update(const NodeID from_node, const EdgeID via_edge, const NodeID to_node);

    double accumulated_length = 0;
    std::vector<util::Coordinate> coordinates;

  private:
    const extractor::guidance::CoordinateExtractor &coordinate_extractor;
    const double max_length;
};

/*
 * The SelectRoadByNameOnlyChoiceAndStraightness tries to follow a given name along a route. We
 * offer methods to skip
 * over bridges/similar situations if desired, following narrow turns
 * This struct offers an example implementation of a possible road selector for traversing the
 * node-based graph using the NodeBasedGraphWalker
 */
struct SelectRoadByNameOnlyChoiceAndStraightness
{
    SelectRoadByNameOnlyChoiceAndStraightness(const NameID desired_name_id,
                                              const bool requires_entry);

    /*
     * !! REQUIRED - Function for the use of TraverseRoad in the graph walker.
     * The operator() needs to return (if any is found) the next road to continue in the graph
     * traversal. If no such edge is found, return {} is allowed. Usually you want to choose some
     * form of obious turn to follow.
     */
    boost::optional<EdgeID> operator()(const NodeID nid,
                                       const EdgeID via_edge_id,
                                       const IntersectionView &intersection,
                                       const util::NodeBasedDynamicGraph &node_based_graph) const;

  private:
    const NameID desired_name_id;
    const bool requires_entry;
};

/* Following only a straight road
 * Follow only the straightmost turn, as long as its the only choice or has the desired name
 */
struct SelectStraightmostRoadByNameAndOnlyChoice
{
    SelectStraightmostRoadByNameAndOnlyChoice(const NameID desired_name_id,
                                              const double initial_bearing,
                                              const bool requires_entry);

    /*
     * !! REQUIRED - Function for the use of TraverseRoad in the graph walker.
     * The operator() needs to return (if any is found) the next road to continue in the graph
     * traversal. If no such edge is found, return {} is allowed. Usually you want to choose some
     * form of obious turn to follow.
     */
    boost::optional<EdgeID> operator()(const NodeID nid,
                                       const EdgeID via_edge_id,
                                       const IntersectionView &intersection,
                                       const util::NodeBasedDynamicGraph &node_based_graph) const;

  private:
    const NameID desired_name_id;
    const double initial_bearing;
    const bool requires_entry;
};

// find the next intersection given a hop limit
struct IntersectionFinderAccumulator
{
    IntersectionFinderAccumulator(const std::uint8_t hop_limit,
                                  const IntersectionGenerator &intersection_generator);
    // true if the path has traversed enough distance
    bool terminate();

    // update the accumulator
    void update(const NodeID from_node, const EdgeID via_edge, const NodeID to_node);

    std::uint8_t hops;
    const std::uint8_t hop_limit;

    // we need to be able to look-up the intersection
    const IntersectionGenerator &intersection_generator;

    // the result we are looking for
    NodeID nid;
    EdgeID via_edge_id;
    IntersectionView intersection;
};

template <class accumulator_type, class selector_type>
boost::optional<std::pair<NodeID, EdgeID>>
NodeBasedGraphWalker::TraverseRoad(NodeID current_node_id,
                                   EdgeID current_edge_id,
                                   accumulator_type &accumulator,
                                   const selector_type &selector) const
{
    /*
     * since graph hopping is used in many ways, we don't generate an adjusted intersection
     * (otherwise we could end up in infinite recursion if we call the graph hopper during the
     * adjustment itself). Relying only on `GetConnectedRoads` (which itself does no graph hopping),
     * we prevent this from happening.
     */
    const auto stop_node_id = current_node_id;
    /* we wan't to put out the last valid entries. To do so, we need to update within the following
     * loop. We use a for loop since traversal of the node-based-graph is expensive and we don't
     * want to look at many coordinates. If you require more than 2/3 intersections down the road,
     * you are doing something wrong/unsupported by OSRM. To not fail hard in cases that offer
     * strange loop contractions, we restrict ourselves to an extremely large number of possible
     * steps and simply warn in cases were extraction runs into these limits.
     */
    for (std::size_t safety_hop_limit = 0; safety_hop_limit < 1000; ++safety_hop_limit)
    {
        accumulator.update(
            current_node_id, current_edge_id, node_based_graph.GetTarget(current_edge_id));

        // we have looped back to our initial intersection
        if (node_based_graph.GetTarget(current_edge_id) == stop_node_id)
            return {};

        // look at the next intersection
        const constexpr auto LOW_PRECISION = true;
        const auto next_intersection = intersection_generator.GetConnectedRoads(
            current_node_id, current_edge_id, LOW_PRECISION);

        // don't follow u-turns or go past our initial intersection
        if (next_intersection.size() <= 1)
            return {};

        auto next_edge_id =
            selector(current_node_id, current_edge_id, next_intersection, node_based_graph);

        if (!next_edge_id)
            return {};

        if (accumulator.terminate())
            return {std::make_pair(current_node_id, current_edge_id)};

        current_node_id = node_based_graph.GetTarget(current_edge_id);
        current_edge_id = *next_edge_id;
    }

    BOOST_ASSERT(
        "Reached safety hop limit. Graph hopper seems to have been caught in an endless loop");
    return {};
}

struct SkipTrafficSignalBarrierRoadSelector
{
    boost::optional<EdgeID> operator()(const NodeID,
                                       const EdgeID,
                                       const IntersectionView &intersection,
                                       const util::NodeBasedDynamicGraph &) const
    {
        if (intersection.isTrafficSignalOrBarrier())
        {
            return boost::make_optional(intersection[1].eid);
        }
        else
        {
            return boost::none;
        }
    }
};

struct DistanceToNextIntersectionAccumulator
{
    DistanceToNextIntersectionAccumulator(
        const extractor::guidance::CoordinateExtractor &extractor_,
        const util::NodeBasedDynamicGraph &graph_,
        const double threshold)
        : extractor{extractor_}, graph{graph_}, threshold{threshold}
    {
    }

    bool terminate()
    {
        if (distance > threshold)
        {
            too_far_away = true;
            return true;
        }

        return false;
    }

    void update(const NodeID start, const EdgeID onto, const NodeID)
    {
        using namespace util::coordinate_calculation;

        const auto coords = extractor.GetForwardCoordinatesAlongRoad(start, onto);
        distance += getLength(coords.begin(), coords.end(), &haversineDistance);
    }

    const extractor::guidance::CoordinateExtractor &extractor;
    const util::NodeBasedDynamicGraph &graph;
    const double threshold;
    bool too_far_away = false;
    double distance = 0.;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_NODE_BASED_GRAPH_WALKER */
