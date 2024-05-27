#ifndef OSRM_GEOJSON_DEBUG_POLICIES
#define OSRM_GEOJSON_DEBUG_POLICIES

#include <optional>
#include <vector>

#include "extractor/query_node.hpp"
#include "util/coordinate.hpp"
#include "util/json_container.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

namespace osrm::util
{

struct NodeIdVectorToLineString
{
    NodeIdVectorToLineString(const std::vector<util::Coordinate> &node_coordinates);

    // converts a vector of node ids into a linestring geojson feature
    util::json::Object operator()(const std::vector<NodeID> &node_ids,
                                  const std::optional<json::Object> &properties = {}) const;

    const std::vector<util::Coordinate> &node_coordinates;
};

struct CoordinateVectorToLineString
{
    // converts a vector of node ids into a linestring geojson feature
    util::json::Object operator()(const std::vector<util::Coordinate> &coordinates,
                                  const std::optional<json::Object> &properties = {}) const;
};

struct NodeIdVectorToMultiPoint
{
    NodeIdVectorToMultiPoint(const std::vector<util::Coordinate> &node_coordinates);

    // converts a vector of node ids into a linestring geojson feature
    util::json::Object operator()(const std::vector<NodeID> &node_ids,
                                  const std::optional<json::Object> &properties = {}) const;

    const std::vector<util::Coordinate> &node_coordinates;
};

struct CoordinateVectorToMultiPoint
{
    // converts a vector of node ids into a linestring geojson feature
    util::json::Object operator()(const std::vector<util::Coordinate> &coordinates,
                                  const std::optional<json::Object> &properties = {}) const;
};

} // namespace osrm::util

#endif /* OSRM_GEOJSON_DEBUG_POLICIES */