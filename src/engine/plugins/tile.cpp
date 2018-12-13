#include "guidance/turn_instruction.hpp"

#include "engine/plugins/plugin_base.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "engine/plugins/tile.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/string_view.hpp"
#include "util/vector_tile.hpp"
#include "util/web_mercator.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>

#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>

#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cmath>
#include <cstdint>

namespace osrm
{
namespace engine
{
namespace plugins
{

constexpr const static int MIN_ZOOM_FOR_TURNS = 15;

namespace
{

using RTreeLeaf = datafacade::BaseDataFacade::RTreeLeaf;
// Simple container class for WGS84 coordinates
template <typename T> struct Point final
{
    Point(T _x, T _y) : x(_x), y(_y) {}

    const T x;
    const T y;
};

// Simple container to hold a bounding box
struct BBox final
{
    BBox(const double _minx, const double _miny, const double _maxx, const double _maxy)
        : minx(_minx), miny(_miny), maxx(_maxx), maxy(_maxy)
    {
    }

    double width() const { return maxx - minx; }
    double height() const { return maxy - miny; }

    const double minx;
    const double miny;
    const double maxx;
    const double maxy;
};

// Simple container for integer coordinates (i.e. pixel coords)
struct point_type_i final
{
    point_type_i(std::int64_t _x, std::int64_t _y) : x(_x), y(_y) {}

    const std::int64_t x;
    const std::int64_t y;
};

using FixedPoint = Point<std::int32_t>;
using FloatPoint = Point<double>;

using FixedLine = std::vector<FixedPoint>;
using FloatLine = std::vector<FloatPoint>;

// We use boost::geometry to clip lines/points that are outside or cross the boundary
// of the tile we're rendering.  We need these types defined to use boosts clipping
// logic
typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> point_t;
typedef boost::geometry::model::linestring<point_t> linestring_t;
typedef boost::geometry::model::box<point_t> box_t;
typedef boost::geometry::model::multi_linestring<linestring_t> multi_linestring_t;
const static box_t clip_box(point_t(-util::vector_tile::BUFFER, -util::vector_tile::BUFFER),
                            point_t(util::vector_tile::EXTENT + util::vector_tile::BUFFER,
                                    util::vector_tile::EXTENT + util::vector_tile::BUFFER));

/**
 * Return the x1,y1,x2,y2 pixel coordinates of a line in a given
 * tile.
 *
 * @param start the first coordinate of the line
 * @param target the last coordinate of the line
 * @param tile_bbox the boundaries of the tile, in mercator coordinates
 * @return a FixedLine with coordinates relative to the tile_bbox.
 */
linestring_t floatLineToTileLine(const FloatLine &geo_line, const BBox &tile_bbox)
{
    linestring_t unclipped_line;

    for (auto const &pt : geo_line)
    {
        double px_merc = pt.x * util::web_mercator::DEGREE_TO_PX;
        double py_merc = util::web_mercator::latToY(util::FloatLatitude{pt.y}) *
                         util::web_mercator::DEGREE_TO_PX;
        // convert lon/lat to tile coordinates
        const auto px = std::round(
            ((px_merc - tile_bbox.minx) * util::web_mercator::TILE_SIZE / tile_bbox.width()) *
            util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE);
        const auto py = std::round(
            ((tile_bbox.maxy - py_merc) * util::web_mercator::TILE_SIZE / tile_bbox.height()) *
            util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE);

        boost::geometry::append(unclipped_line, point_t(px, py));
    }

    return unclipped_line;
}

std::vector<FixedLine> coordinatesToTileLine(const std::vector<util::Coordinate> &points,
                                             const BBox &tile_bbox)
{
    FloatLine geo_line;
    for (auto const &c : points)
    {
        geo_line.emplace_back(static_cast<double>(util::toFloating(c.lon)),
                              static_cast<double>(util::toFloating(c.lat)));
    }

    linestring_t unclipped_line = floatLineToTileLine(geo_line, tile_bbox);

    multi_linestring_t clipped_line;
    boost::geometry::intersection(clip_box, unclipped_line, clipped_line);

    std::vector<FixedLine> result;

    // b::g::intersection might return a line with one point if the
    // original line was very short and coords were dupes
    for (auto const &cl : clipped_line)
    {
        if (cl.size() < 2)
            continue;

        FixedLine tile_line;
        for (const auto &p : cl)
            tile_line.emplace_back(p.get<0>(), p.get<1>());

        result.emplace_back(std::move(tile_line));
    }

    return result;
}

/**
 * Return the x1,y1,x2,y2 pixel coordinates of a line in a given
 * tile.
 *
 * @param start the first coordinate of the line
 * @param target the last coordinate of the line
 * @param tile_bbox the boundaries of the tile, in mercator coordinates
 * @return a FixedLine with coordinates relative to the tile_bbox.
 */
FixedLine coordinatesToTileLine(const util::Coordinate start,
                                const util::Coordinate target,
                                const BBox &tile_bbox)
{
    FloatLine geo_line;
    geo_line.emplace_back(static_cast<double>(util::toFloating(start.lon)),
                          static_cast<double>(util::toFloating(start.lat)));
    geo_line.emplace_back(static_cast<double>(util::toFloating(target.lon)),
                          static_cast<double>(util::toFloating(target.lat)));

    linestring_t unclipped_line = floatLineToTileLine(geo_line, tile_bbox);

    multi_linestring_t clipped_line;
    boost::geometry::intersection(clip_box, unclipped_line, clipped_line);

    FixedLine tile_line;

    // b::g::intersection might return a line with one point if the
    // original line was very short and coords were dupes
    if (!clipped_line.empty() && clipped_line[0].size() == 2)
    {
        for (const auto &p : clipped_line[0])
        {
            tile_line.emplace_back(p.get<0>(), p.get<1>());
        }
    }

    return tile_line;
}

/**
 * Converts lon/lat into coordinates inside a Mercator projection tile (x/y pixel values)
 *
 * @param point the lon/lat you want the tile coords for
 * @param tile_bbox the mercator boundaries of the tile
 * @return a point (x,y) on the tile defined by tile_bbox
 */
FixedPoint coordinatesToTilePoint(const util::Coordinate point, const BBox &tile_bbox)
{
    const FloatPoint geo_point{static_cast<double>(util::toFloating(point.lon)),
                               static_cast<double>(util::toFloating(point.lat))};

    const double px_merc = geo_point.x * util::web_mercator::DEGREE_TO_PX;
    const double py_merc = util::web_mercator::latToY(util::FloatLatitude{geo_point.y}) *
                           util::web_mercator::DEGREE_TO_PX;

    const auto px = static_cast<std::int32_t>(std::round(
        ((px_merc - tile_bbox.minx) * util::web_mercator::TILE_SIZE / tile_bbox.width()) *
        util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE));
    const auto py = static_cast<std::int32_t>(std::round(
        ((tile_bbox.maxy - py_merc) * util::web_mercator::TILE_SIZE / tile_bbox.height()) *
        util::vector_tile::EXTENT / util::web_mercator::TILE_SIZE));

    return FixedPoint{px, py};
}

std::vector<RTreeLeaf> getEdges(const DataFacadeBase &facade, unsigned x, unsigned y, unsigned z)
{
    double min_lon, min_lat, max_lon, max_lat;

    // Convert the z,x,y mercator tile coordinates into WGS84 lon/lat values
    //
    util::web_mercator::xyzToWGS84(
        x, y, z, min_lon, min_lat, max_lon, max_lat, util::web_mercator::TILE_SIZE * 0.10);

    util::Coordinate southwest{util::FloatLongitude{min_lon}, util::FloatLatitude{min_lat}};
    util::Coordinate northeast{util::FloatLongitude{max_lon}, util::FloatLatitude{max_lat}};

    // Fetch all the segments that are in our bounding box.
    // This hits the OSRM StaticRTree
    return facade.GetEdgesInBox(southwest, northeast);
}

std::vector<std::size_t> getEdgeIndex(const std::vector<RTreeLeaf> &edges)
{
    // In order to ensure consistent tile encoding, we need to process
    // all edges in the same order.  Differences in OSX/Linux/Windows
    // sorting methods mean that GetEdgesInBox doesn't return the same
    // ordered array on all platforms.
    // GetEdgesInBox is marked `const`, so we can't sort the array itself,
    // instead we create an array of indexes and sort that instead.
    std::vector<std::size_t> sorted_edge_indexes(edges.size(), 0);
    std::iota(
        sorted_edge_indexes.begin(), sorted_edge_indexes.end(), 0); // fill with 0,1,2,3,...N-1

    // Now, sort that array based on the edges list, using the u/v node IDs
    // as the sort condition
    std::sort(sorted_edge_indexes.begin(),
              sorted_edge_indexes.end(),
              [&edges](const std::size_t &left, const std::size_t &right) -> bool {
                  return (edges[left].u != edges[right].u) ? edges[left].u < edges[right].u
                                                           : edges[left].v < edges[right].v;
              });

    return sorted_edge_indexes;
}

std::vector<NodeID> getSegregatedNodes(const DataFacadeBase &facade,
                                       const std::vector<RTreeLeaf> &edges)
{
    std::vector<NodeID> result;

    for (RTreeLeaf const &e : edges)
    {
        if (e.forward_segment_id.enabled && facade.IsSegregated(e.forward_segment_id.id))
            result.push_back(e.forward_segment_id.id);
    }

    return result;
}

struct SpeedLayer : public vtzero::layer_builder
{

    vtzero::value_index_small_uint uint_index;
    vtzero::value_index<vtzero::double_value_type, float, std::unordered_map> double_index;
    vtzero::value_index_internal<std::unordered_map> string_index;
    vtzero::value_index_bool bool_index;

    vtzero::index_value key_speed;
    vtzero::index_value key_is_small;
    vtzero::index_value key_datasource;
    vtzero::index_value key_weight;
    vtzero::index_value key_duration;
    vtzero::index_value key_name;
    vtzero::index_value key_rate;
    vtzero::index_value key_is_startpoint;

    SpeedLayer(vtzero::tile_builder &tile)
        : layer_builder(tile, "speeds"), uint_index(*this), double_index(*this),
          string_index(*this), bool_index(*this), key_speed(add_key_without_dup_check("speed")),
          key_is_small(add_key_without_dup_check("is_small")),
          key_datasource(add_key_without_dup_check("datasource")),
          key_weight(add_key_without_dup_check("weight")),
          key_duration(add_key_without_dup_check("duration")),
          key_name(add_key_without_dup_check("name")), key_rate(add_key_without_dup_check("rate")),
          key_is_startpoint(add_key_without_dup_check("is_startpoint"))
    {
    }

}; // struct SpeedLayer

class SpeedLayerFeatureBuilder : public vtzero::linestring_feature_builder
{

    SpeedLayer &m_layer;

  public:
    SpeedLayerFeatureBuilder(SpeedLayer &layer, uint64_t id)
        : vtzero::linestring_feature_builder(layer), m_layer(layer)
    {
        set_id(id);
    }

    void set_speed(unsigned int value)
    {
        add_property(m_layer.key_speed, m_layer.uint_index(std::min(value, 127u)));
    }

    void set_is_small(bool value) { add_property(m_layer.key_is_small, m_layer.bool_index(value)); }

    void set_datasource(const std::string &value)
    {
        add_property(m_layer.key_datasource,
                     m_layer.string_index(vtzero::encoded_property_value{value}));
    }

    void set_weight(double value) { add_property(m_layer.key_weight, m_layer.double_index(value)); }

    void set_duration(double value)
    {
        add_property(m_layer.key_duration, m_layer.double_index(value));
    }

    void set_name(const boost::string_ref &value)
    {
        add_property(
            m_layer.key_name,
            m_layer.string_index(vtzero::encoded_property_value{value.data(), value.size()}));
    }

    void set_rate(double value) { add_property(m_layer.key_rate, m_layer.double_index(value)); }

    void set_is_startpoint(bool value)
    {
        add_property(m_layer.key_is_startpoint, m_layer.bool_index(value));
    }

}; // class SpeedLayerFeatureBuilder

struct TurnsLayer : public vtzero::layer_builder
{

    vtzero::value_index<vtzero::sint_value_type, int, std::unordered_map> int_index;
    vtzero::value_index<vtzero::float_value_type, float, std::unordered_map> float_index;
    vtzero::value_index_internal<std::unordered_map> string_index;

    vtzero::index_value key_bearing_in;
    vtzero::index_value key_turn_angle;
    vtzero::index_value key_cost;
    vtzero::index_value key_weight;
    vtzero::index_value key_turn_type;
    vtzero::index_value key_turn_modifier;

    TurnsLayer(vtzero::tile_builder &tile)
        : layer_builder(tile, "turns"), int_index(*this), float_index(*this), string_index(*this),
          key_bearing_in(add_key_without_dup_check("bearing_in")),
          key_turn_angle(add_key_without_dup_check("turn_angle")),
          key_cost(add_key_without_dup_check("cost")),
          key_weight(add_key_without_dup_check("weight")),
          key_turn_type(add_key_without_dup_check("type")),
          key_turn_modifier(add_key_without_dup_check("modifier"))
    {
    }

}; // struct TurnsLayer

class TurnsLayerFeatureBuilder : public vtzero::point_feature_builder
{

    TurnsLayer &m_layer;

  public:
    TurnsLayerFeatureBuilder(TurnsLayer &layer, uint64_t id)
        : vtzero::point_feature_builder(layer), m_layer(layer)
    {
        set_id(id);
    }

    void set_bearing_in(int value)
    {
        add_property(m_layer.key_bearing_in, m_layer.int_index(value));
    }

    void set_turn_angle(int value)
    {
        add_property(m_layer.key_turn_angle, m_layer.int_index(value));
    }

    void set_cost(float value) { add_property(m_layer.key_cost, m_layer.float_index(value)); }

    void set_weight(float value) { add_property(m_layer.key_weight, m_layer.float_index(value)); }

    void set_turn(osrm::guidance::TurnInstruction value)
    {
        const auto type = osrm::guidance::internalInstructionTypeToString(value.type);
        const auto modifier = osrm::guidance::instructionModifierToString(value.direction_modifier);
        add_property(
            m_layer.key_turn_type,
            m_layer.string_index(vtzero::encoded_property_value{type.data(), type.size()}));
        add_property(
            m_layer.key_turn_modifier,
            m_layer.string_index(vtzero::encoded_property_value{modifier.data(), modifier.size()}));
    }
}; // class TurnsLayerFeatureBuilder

void encodeVectorTile(const DataFacadeBase &facade,
                      unsigned x,
                      unsigned y,
                      unsigned z,
                      const std::vector<RTreeLeaf> &edges,
                      const std::vector<std::size_t> &sorted_edge_indexes,
                      const std::vector<routing_algorithms::TurnData> &all_turn_data,
                      const std::vector<NodeID> &segregated_nodes,
                      std::string &pbf_buffer)
{
    vtzero::tile_builder tile;

    const auto get_geometry_id = [&facade](auto edge) {
        return facade.GetGeometryIndex(edge.forward_segment_id.id).id;
    };

    // Convert tile coordinates into mercator coordinates
    double min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat;
    util::web_mercator::xyzToMercator(
        x, y, z, min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat);
    const BBox tile_bbox{min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat};

    // XXX leaving in some superfluous scopes to make diff easier to read.
    {
        {
            // Begin the layer features block
            {
                SpeedLayer speeds_layer{tile};

                // Each feature gets a unique id, starting at 1
                unsigned id = 1;
                for (const auto &edge_index : sorted_edge_indexes)
                {
                    const auto &edge = edges[edge_index];
                    const auto geometry_id = get_geometry_id(edge);

                    // Get coordinates for start/end nodes of segment (NodeIDs u and v)
                    const auto a = facade.GetCoordinateOfNode(edge.u);
                    const auto b = facade.GetCoordinateOfNode(edge.v);
                    // Calculate the length in meters
                    const double length =
                        osrm::util::coordinate_calculation::haversineDistance(a, b);

                    const auto forward_weight_range =
                        facade.GetUncompressedForwardWeights(geometry_id);
                    const auto reverse_weight_range =
                        facade.GetUncompressedReverseWeights(geometry_id);
                    const auto forward_duration_range =
                        facade.GetUncompressedForwardDurations(geometry_id);
                    const auto reverse_duration_range =
                        facade.GetUncompressedReverseDurations(geometry_id);
                    const auto forward_datasource_range =
                        facade.GetUncompressedForwardDatasources(geometry_id);
                    const auto reverse_datasource_range =
                        facade.GetUncompressedReverseDatasources(geometry_id);
                    const auto forward_weight = forward_weight_range[edge.fwd_segment_position];
                    const auto reverse_weight = reverse_weight_range[reverse_weight_range.size() -
                                                                     edge.fwd_segment_position - 1];

                    const auto forward_duration = forward_duration_range[edge.fwd_segment_position];
                    const auto reverse_duration =
                        reverse_duration_range[reverse_duration_range.size() -
                                               edge.fwd_segment_position - 1];
                    const auto forward_datasource_idx =
                        forward_datasource_range(edge.fwd_segment_position);
                    const auto reverse_datasource_idx = reverse_datasource_range(
                        reverse_datasource_range.size() - edge.fwd_segment_position - 1);

                    const auto is_startpoint = edge.is_startpoint;

                    const auto component_id = facade.GetComponentID(edge.forward_segment_id.id);
                    const auto name_id = facade.GetNameIndex(edge.forward_segment_id.id);
                    auto name = facade.GetNameForID(name_id);

                    // If this is a valid forward edge, go ahead and add it to the tile
                    if (forward_duration != 0 && edge.forward_segment_id.enabled)
                    {
                        // Calculate the speed for this line
                        std::uint32_t speed_kmh_idx =
                            static_cast<std::uint32_t>(round(length / forward_duration * 10 * 3.6));

                        // Rate values are in meters per weight-unit - and similar to speeds, we
                        // present 1 decimal place of precision (these values are added as
                        // double/10) lower down
                        std::uint32_t forward_rate =
                            static_cast<std::uint32_t>(round(length / forward_weight * 10.));

                        auto tile_line = coordinatesToTileLine(a, b, tile_bbox);
                        if (!tile_line.empty())
                        {
                            SpeedLayerFeatureBuilder fbuilder{speeds_layer, id};
                            fbuilder.add_linestring_from_container(tile_line);

                            fbuilder.set_speed(speed_kmh_idx);
                            fbuilder.set_is_small(component_id.is_tiny);
                            fbuilder.set_datasource(
                                facade.GetDatasourceName(forward_datasource_idx).to_string());
                            fbuilder.set_weight(forward_weight / 10.0);
                            fbuilder.set_duration(forward_duration / 10.0);
                            fbuilder.set_name(name);
                            fbuilder.set_rate(forward_rate / 10.0);
                            fbuilder.set_is_startpoint(is_startpoint);

                            fbuilder.commit();
                        }
                    }

                    // Repeat the above for the coordinates reversed and using the `reverse`
                    // properties
                    if (reverse_duration != 0 && edge.reverse_segment_id.enabled)
                    {
                        // Calculate the speed for this line
                        std::uint32_t speed_kmh_idx =
                            static_cast<std::uint32_t>(round(length / reverse_duration * 10 * 3.6));

                        // Rate values are in meters per weight-unit - and similar to speeds, we
                        // present 1 decimal place of precision (these values are added as
                        // double/10) lower down
                        std::uint32_t reverse_rate =
                            static_cast<std::uint32_t>(round(length / reverse_weight * 10.));

                        auto tile_line = coordinatesToTileLine(b, a, tile_bbox);
                        if (!tile_line.empty())
                        {
                            SpeedLayerFeatureBuilder fbuilder{speeds_layer, id};
                            fbuilder.add_linestring_from_container(tile_line);

                            fbuilder.set_speed(speed_kmh_idx);
                            fbuilder.set_is_small(component_id.is_tiny);
                            fbuilder.set_datasource(
                                facade.GetDatasourceName(reverse_datasource_idx).to_string());
                            fbuilder.set_weight(reverse_weight / 10.0);
                            fbuilder.set_duration(reverse_duration / 10.0);
                            fbuilder.set_name(name);
                            fbuilder.set_rate(reverse_rate / 10.0);
                            fbuilder.set_is_startpoint(is_startpoint);

                            fbuilder.commit();
                        }
                    }
                }
            }
        }

        // Only add the turn layer to the tile if it has some features (we sometimes won't
        // for tiles of z<16, and tiles that don't show any intersections)
        if (!all_turn_data.empty())
        {
            TurnsLayer turns_layer{tile};
            uint64_t id = 0;
            for (const auto &turn_data : all_turn_data)
            {
                const auto tile_point = coordinatesToTilePoint(turn_data.coordinate, tile_bbox);
                if (boost::geometry::within(point_t(tile_point.x, tile_point.y), clip_box))
                {
                    TurnsLayerFeatureBuilder fbuilder{turns_layer, ++id};
                    fbuilder.add_point(tile_point);

                    fbuilder.set_bearing_in(turn_data.in_angle);
                    fbuilder.set_turn_angle(turn_data.turn_angle);
                    fbuilder.set_cost(turn_data.duration / 10.0);
                    fbuilder.set_weight(turn_data.weight / 10.0);
                    fbuilder.set_turn(turn_data.turn_instruction);

                    fbuilder.commit();
                }
            }
        }

        // OSM Node tile layer
        {
            std::vector<NodeID> internal_nodes;
            internal_nodes.reserve(edges.size() * 2);
            for (const auto &edge : edges)
            {
                internal_nodes.push_back(edge.u);
                internal_nodes.push_back(edge.v);
            }
            std::sort(internal_nodes.begin(), internal_nodes.end());
            auto new_end = std::unique(internal_nodes.begin(), internal_nodes.end());
            internal_nodes.resize(new_end - internal_nodes.begin());

            vtzero::layer_builder osmnodes_layer{tile, "osmnodes"};

            for (const auto &internal_node : internal_nodes)
            {
                const auto coord = facade.GetCoordinateOfNode(internal_node);
                const auto tile_point = coordinatesToTilePoint(coord, tile_bbox);
                if (!boost::geometry::within(point_t(tile_point.x, tile_point.y), clip_box))
                {
                    continue;
                }

                vtzero::point_feature_builder fbuilder{osmnodes_layer};
                fbuilder.set_id(
                    static_cast<OSMNodeID::value_type>(facade.GetOSMNodeIDOfNode(internal_node)));
                fbuilder.add_point(tile_point);
                fbuilder.commit();
            }
        }

        // Internal nodes tile layer
        {
            vtzero::layer_builder internal_nodes_layer{tile, "internal-nodes"};

            for (auto edgeNodeID : segregated_nodes)
            {
                auto const geomIndex = facade.GetGeometryIndex(edgeNodeID);

                std::vector<util::Coordinate> points;
                if (geomIndex.forward)
                {
                    for (auto const nodeID : facade.GetUncompressedForwardGeometry(geomIndex.id))
                        points.push_back(facade.GetCoordinateOfNode(nodeID));
                }
                else
                {
                    for (auto const nodeID : facade.GetUncompressedReverseGeometry(geomIndex.id))
                        points.push_back(facade.GetCoordinateOfNode(nodeID));
                }

                auto tile_lines = coordinatesToTileLine(points, tile_bbox);
                if (!tile_lines.empty())
                {
                    vtzero::linestring_feature_builder fbuilder{internal_nodes_layer};
                    for (auto const &tile_line : tile_lines)
                    {
                        fbuilder.add_linestring_from_container(tile_line);
                    }
                    fbuilder.commit();
                }
            }
        }
    }

    tile.serialize(pbf_buffer);
}
}

Status TilePlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                 const api::TileParameters &parameters,
                                 std::string &pbf_buffer) const
{
    BOOST_ASSERT(parameters.IsValid());

    const auto &facade = algorithms.GetFacade();
    auto edges = getEdges(facade, parameters.x, parameters.y, parameters.z);
    auto segregated_nodes = getSegregatedNodes(facade, edges);

    auto edge_index = getEdgeIndex(edges);

    std::vector<routing_algorithms::TurnData> turns;

    // If we're zooming into 16 or higher, include turn data.  Why?  Because turns make the map
    // really cramped, so we don't bother including the data for tiles that span a large area.
    if (parameters.z >= MIN_ZOOM_FOR_TURNS && algorithms.HasGetTileTurns())
    {
        turns = algorithms.GetTileTurns(edges, edge_index);
    }

    encodeVectorTile(facade,
                     parameters.x,
                     parameters.y,
                     parameters.z,
                     edges,
                     edge_index,
                     turns,
                     segregated_nodes,
                     pbf_buffer);

    return Status::Ok;
}
}
}
}
