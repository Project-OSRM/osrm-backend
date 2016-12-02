#ifndef OSRM_EXTRACTOR_GEOJSON_DEBUG_POLICIES
#define OSRM_EXTRACTOR_GEOJSON_DEBUG_POLICIES

#include <vector>

#include "extractor/query_node.hpp"
#include "util/coordinate.hpp"
#include "util/json_container.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include "extractor/guidance/coordinate_extractor.hpp"
#include "extractor/guidance/intersection.hpp"

#include <boost/optional.hpp>

namespace osrm
{
namespace extractor
{
// generate a visualisation of an intersection, printing the coordinates used for angle calculation
struct IntersectionPrinter
{
    IntersectionPrinter(const util::NodeBasedDynamicGraph &node_based_graph,
                        const std::vector<extractor::QueryNode> &node_coordinates,
                        const extractor::guidance::CoordinateExtractor &coordinate_extractor);

    // renders the used coordinate locations for all entries/as well as the resulting
    // intersection-classification
    util::json::Array operator()(const NodeID intersection_node,
                                 const extractor::guidance::Intersection &intersection,
                                 const boost::optional<util::json::Object> &node_style = {},
                                 const boost::optional<util::json::Object> &way_style = {}) const;

    const util::NodeBasedDynamicGraph &node_based_graph;
    const std::vector<extractor::QueryNode> &node_coordinates;
    const extractor::guidance::CoordinateExtractor &coordinate_extractor;
};

} /* namespace extractor */
} /* namespace osrm */

#endif /* OSRM_EXTRACTOR_GEOJSON_DEBUG_POLICIES */
