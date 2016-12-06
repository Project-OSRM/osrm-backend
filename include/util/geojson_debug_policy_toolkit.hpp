#ifndef OSRM_GEOJSON_DEBUG_POLICY_TOOLKIT_HPP
#define OSRM_GEOJSON_DEBUG_POLICY_TOOLKIT_HPP

#include "extractor/external_memory_node.hpp"
#include "util/coordinate.hpp"
#include "util/json_container.hpp"

#include <algorithm>
#include <iterator>

#include <boost/optional.hpp>

namespace osrm
{
namespace util
{

enum GeojsonStyleSize
{
    tiny,
    small,
    medium,
    large,
    extra_large,
    num_styles
};

enum GeojsonStyleColors
{
    red,
    purple,
    blue,
    green,
    yellow,
    cyan,
    brown,
    pink,
    num_colors
};

const constexpr char *geojson_debug_predifined_colors[GeojsonStyleColors::num_colors] = {
    "#FF4848", "#800080", "#5757FF", "#1FCB4A", "#FFE920", "#29AFD6", "#B05F3C", "#FE67EB"};

const constexpr double geojson_predefined_sizes[GeojsonStyleSize::num_styles] = {
    2.0, 3.5, 5.0, 6.5, 8};

inline util::json::Object makeStyle(const GeojsonStyleSize size_type,
                                    const GeojsonStyleColors predefined_color)
{
    util::json::Object style;
    // style everything, since we don't know the feature type
    style.values["stroke"] = geojson_debug_predifined_colors[predefined_color];
    style.values["circle-color"] = geojson_debug_predifined_colors[predefined_color];
    style.values["line-width"] = geojson_predefined_sizes[size_type];
    style.values["circle-radius"] = geojson_predefined_sizes[size_type];
    return style;
}

struct CoordinateToJsonArray
{
    util::json::Array operator()(const util::Coordinate coordinate)
    {
        util::json::Array json_coordinate;
        json_coordinate.values.push_back(static_cast<double>(toFloating(coordinate.lon)));
        json_coordinate.values.push_back(static_cast<double>(toFloating(coordinate.lat)));
        return json_coordinate;
    }
};

struct NodeIdToCoordinate
{
    NodeIdToCoordinate(const std::vector<extractor::QueryNode> &node_coordinates)
        : node_coordinates(node_coordinates)
    {
    }

    const std::vector<extractor::QueryNode> &node_coordinates;

    util::json::Array operator()(const NodeID nid)
    {
        auto coordinate = node_coordinates[nid];
        CoordinateToJsonArray converter;
        return converter(coordinate);
    }
};

inline util::json::Object makeFeature(std::string type,
                                      util::json::Array coordinates,
                                      const boost::optional<util::json::Object> &properties = {})
{
    util::json::Object result;
    result.values["type"] = "Feature";
    result.values["properties"] = properties ? *properties : util::json::Object();
    util::json::Object geometry;
    geometry.values["type"] = std::move(type);
    geometry.values["properties"] = util::json::Object();
    geometry.values["coordinates"] = std::move(coordinates);
    result.values["geometry"] = geometry;

    return result;
}

inline util::json::Array makeJsonArray(const std::vector<util::Coordinate> &input_coordinates)
{
    util::json::Array coordinates;
    std::transform(input_coordinates.begin(),
                   input_coordinates.end(),
                   std::back_inserter(coordinates.values),
                   CoordinateToJsonArray());
    return coordinates;
}
} // namespace util
} // namespace osrm

#endif /* OSRM_GEOJSON_DEBUG_POLICY_TOOLKIT_HPP */
