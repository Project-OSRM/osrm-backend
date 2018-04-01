#ifndef OSRM_EXTRACTOR_GEOJSON_DEBUG_POLICIES
#define OSRM_EXTRACTOR_GEOJSON_DEBUG_POLICIES

#include <algorithm>
#include <vector>

#include "extractor/query_node.hpp"
#include "util/coordinate.hpp"
#include "util/json_container.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include "guidance/coordinate_extractor.hpp"
#include "guidance/intersection.hpp"
#include "util/coordinate.hpp"
#include "util/geojson_debug_policy_toolkit.hpp"

#include <boost/optional.hpp>

namespace osrm
{
namespace extractor
{

// generate a visualisation of an intersection, printing the coordinates used for angle calculation
template <typename IntersectionType> struct IntersectionPrinter
{
    IntersectionPrinter(const util::NodeBasedDynamicGraph &node_based_graph,
                        const std::vector<extractor::QueryNode> &node_coordinates,
                        const extractor::guidance::CoordinateExtractor &coordinate_extractor);

    // renders the used coordinate locations for all entries/as well as the resulting
    // intersection-classification
    util::json::Array operator()(const NodeID intersection_node,
                                 const IntersectionType &intersection,
                                 const boost::optional<util::json::Object> &node_style = {},
                                 const boost::optional<util::json::Object> &way_style = {}) const;

    const util::NodeBasedDynamicGraph &node_based_graph;
    const std::vector<extractor::QueryNode> &node_coordinates;
    const extractor::guidance::CoordinateExtractor &coordinate_extractor;
};

// IMPLEMENTATION
template <typename IntersectionType>
IntersectionPrinter<IntersectionType>::IntersectionPrinter(
    const util::NodeBasedDynamicGraph &node_based_graph,
    const std::vector<extractor::QueryNode> &node_coordinates,
    const extractor::guidance::CoordinateExtractor &coordinate_extractor)
    : node_based_graph(node_based_graph), node_coordinates(node_coordinates),
      coordinate_extractor(coordinate_extractor)
{
}

template <typename IntersectionType>
util::json::Array IntersectionPrinter<IntersectionType>::
operator()(const NodeID intersection_node,
           const IntersectionType &intersection,
           const boost::optional<util::json::Object> &node_style,
           const boost::optional<util::json::Object> &way_style) const
{
    // request the number of lanes. This process needs to be in sync with what happens over at
    // intersection analysis
    const auto intersection_lanes =
        intersection.FindMaximum(guidance::makeExtractLanesForRoad(node_based_graph));

    std::vector<util::Coordinate> coordinates;
    coordinates.reserve(intersection.size());
    coordinates.push_back(node_coordinates[intersection_node]);

    const auto road_to_coordinate = [&](const auto &road) {
        const constexpr auto FORWARD = false;
        const auto to_node = node_based_graph.GetTarget(road.eid);
        return coordinate_extractor.GetCoordinateAlongRoad(
            intersection_node, road.eid, FORWARD, to_node, intersection_lanes);
    };

    std::transform(intersection.begin(),
                   intersection.end(),
                   std::back_inserter(coordinates),
                   road_to_coordinate);

    util::json::Array features;
    features.values.push_back(
        util::makeFeature("MultiPoint", makeJsonArray(coordinates), node_style));

    if (coordinates.size() > 1)
    {
        std::vector<util::Coordinate> line_coordinates(2);
        line_coordinates[0] = coordinates.front();
        const auto coordinate_to_line = [&](const util::Coordinate coordinate) {
            line_coordinates[1] = coordinate;
            return util::makeFeature("LineString", makeJsonArray(line_coordinates), way_style);
        };

        std::transform(std::next(coordinates.begin()),
                       coordinates.end(),
                       std::back_inserter(features.values),
                       coordinate_to_line);
    }
    return features;
}

} /* namespace extractor */
} /* namespace osrm */

#endif /* OSRM_EXTRACTOR_GEOJSON_DEBUG_POLICIES */
