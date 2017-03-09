#include "engine/plugins/tile.hpp"
#include "engine/plugins/plugin_base.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/string_view.hpp"
#include "util/vector_tile.hpp"
#include "util/web_mercator.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>

#include <protozero/pbf_writer.hpp>
#include <protozero/varint.hpp>

#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
// TODO: Port all this encoding logic to https://github.com/mapbox/vector-tile, which wasn't
// available when this code was originally written.

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

// from mapnik-vector-tile
// Encodes a linestring using protobuf zigzag encoding
inline bool encodeLinestring(const FixedLine &line,
                             protozero::packed_field_uint32 &geometry,
                             std::int32_t &start_x,
                             std::int32_t &start_y)
{
    const std::size_t line_size = line.size();
    if (line_size < 2)
    {
        return false;
    }

    const unsigned lineto_count = static_cast<const unsigned>(line_size) - 1;

    auto pt = line.begin();
    const constexpr int MOVETO_COMMAND = 9;
    geometry.add_element(MOVETO_COMMAND); // move_to | (1 << 3)
    geometry.add_element(protozero::encode_zigzag32(pt->x - start_x));
    geometry.add_element(protozero::encode_zigzag32(pt->y - start_y));
    start_x = pt->x;
    start_y = pt->y;
    // This means LINETO repeated N times
    // See: https://github.com/mapbox/vector-tile-spec/tree/master/2.1#example-command-integers
    geometry.add_element((lineto_count << 3u) | 2u);
    // Now that we've issued the LINETO REPEAT N command, we append
    // N coordinate pairs immediately after the command.
    for (++pt; pt != line.end(); ++pt)
    {
        const std::int32_t dx = pt->x - start_x;
        const std::int32_t dy = pt->y - start_y;
        geometry.add_element(protozero::encode_zigzag32(dx));
        geometry.add_element(protozero::encode_zigzag32(dy));
        start_x = pt->x;
        start_y = pt->y;
    }
    return true;
}

// from mapnik-vctor-tile
// Encodes a point
inline void encodePoint(const FixedPoint &pt, protozero::packed_field_uint32 &geometry)
{
    const constexpr int MOVETO_COMMAND = 9;
    geometry.add_element(MOVETO_COMMAND);
    const std::int32_t dx = pt.x;
    const std::int32_t dy = pt.y;
    // Manual zigzag encoding.
    geometry.add_element(protozero::encode_zigzag32(dx));
    geometry.add_element(protozero::encode_zigzag32(dy));
}

/**
 * Returnx the x1,y1,x2,y2 pixel coordinates of a line in a given
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

    multi_linestring_t clipped_line;

    boost::geometry::intersection(clip_box, unclipped_line, clipped_line);

    FixedLine tile_line;

    // b::g::intersection might return a line with one point if the
    // original line was very short and coords were dupes
    if (!clipped_line.empty() && clipped_line[0].size() == 2)
    {
        if (clipped_line[0].size() == 2)
        {
            for (const auto &p : clipped_line[0])
            {
                tile_line.emplace_back(p.get<0>(), p.get<1>());
            }
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

std::vector<RTreeLeaf> getEdges(const datafacade::ContiguousInternalMemoryDataFacadeBase &facade,
                                unsigned x,
                                unsigned y,
                                unsigned z)
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

void encodeVectorTile(const datafacade::ContiguousInternalMemoryDataFacadeBase &facade,
                      unsigned x,
                      unsigned y,
                      unsigned z,
                      const std::vector<RTreeLeaf> &edges,
                      const std::vector<std::size_t> &sorted_edge_indexes,
                      const std::vector<routing_algorithms::TurnData> &all_turn_data,
                      std::string &pbf_buffer)
{

    // Vector tiles encode properties as references to a common lookup table.
    // When we add a property to a "feature", we actually attach the index of the value
    // rather than the value itself.  Thus, we need to keep a list of the unique
    // values we need, and we add this list to the tile as a lookup table.  This
    // vector holds all the actual used values, the feature refernce offsets in
    // this vector.
    // for integer values
    std::vector<int> used_line_ints;
    // While constructing the tile, we keep track of which integers we have in our table
    // and their offsets, so multiple features can re-use the same values
    std::unordered_map<int, std::size_t> line_int_offsets;

    // Same idea for street names - one lookup table for names for all features
    std::vector<util::StringView> names;
    std::unordered_map<util::StringView, std::size_t> name_offsets;

    // And again for integer values used by points.
    std::vector<int> used_point_ints;
    std::unordered_map<int, std::size_t> point_int_offsets;

    // And again for float values used by points
    std::vector<float> used_point_floats;
    std::unordered_map<float, std::size_t> point_float_offsets;

    std::uint8_t max_datasource_id = 0;

    // This is where we accumulate information on turns

    // Helper function for adding a new value to the line_ints lookup table.  Returns
    // the index of the value in the table, adding the value if it doesn't already
    // exist
    const auto use_line_value = [&used_line_ints, &line_int_offsets](const int value) {
        const auto found = line_int_offsets.find(value);

        if (found == line_int_offsets.end())
        {
            used_line_ints.push_back(value);
            line_int_offsets[value] = used_line_ints.size() - 1;
        }

        return;
    };

    // Same again
    const auto use_point_int_value = [&used_point_ints, &point_int_offsets](const int value) {
        const auto found = point_int_offsets.find(value);
        std::size_t offset;

        if (found == point_int_offsets.end())
        {
            used_point_ints.push_back(value);
            offset = used_point_ints.size() - 1;
            point_int_offsets[value] = offset;
        }
        else
        {
            offset = found->second;
        }

        return offset;
    };

    // And a third time, should probably template this....
    const auto use_point_float_value = [&used_point_floats,
                                        &point_float_offsets](const float value) {
        const auto found = point_float_offsets.find(value);
        std::size_t offset;

        if (found == point_float_offsets.end())
        {
            used_point_floats.push_back(value);
            offset = used_point_floats.size() - 1;
            point_float_offsets[value] = offset;
        }
        else
        {
            offset = found->second;
        }

        return offset;
    };

    // Vector tiles encode feature properties as indexes into a lookup table.  So, we need
    // to "pre-loop" over all the edges to create the lookup tables.  Once we have those, we
    // can then encode the features, and we'll know the indexes that feature properties
    // need to refer to.
    for (const auto &edge_index : sorted_edge_indexes)
    {
        const auto &edge = edges[edge_index];

        const auto forward_datasource_vector =
            facade.GetUncompressedForwardDatasources(edge.packed_geometry_id);
        const auto reverse_datasource_vector =
            facade.GetUncompressedReverseDatasources(edge.packed_geometry_id);

        BOOST_ASSERT(edge.fwd_segment_position < forward_datasource_vector.size());
        const auto forward_datasource = forward_datasource_vector[edge.fwd_segment_position];
        BOOST_ASSERT(edge.fwd_segment_position < reverse_datasource_vector.size());
        const auto reverse_datasource = reverse_datasource_vector[reverse_datasource_vector.size() -
                                                                  edge.fwd_segment_position - 1];

        // Keep track of the highest datasource seen so that we don't write unnecessary
        // data to the layer attribute values
        max_datasource_id = std::max(max_datasource_id, forward_datasource);
        max_datasource_id = std::max(max_datasource_id, reverse_datasource);
    }

    // Convert tile coordinates into mercator coordinates
    double min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat;
    util::web_mercator::xyzToMercator(
        x, y, z, min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat);
    const BBox tile_bbox{min_mercator_lon, min_mercator_lat, max_mercator_lon, max_mercator_lat};

    // Protobuf serializes blocks when objects go out of scope, hence
    // the extra scoping below.
    protozero::pbf_writer tile_writer{pbf_buffer};
    {
        {
            // Add a layer object to the PBF stream.  3=='layer' from the vector tile spec
            // (2.1)
            protozero::pbf_writer line_layer_writer(tile_writer, util::vector_tile::LAYER_TAG);
            // TODO: don't write a layer if there are no features

            line_layer_writer.add_uint32(util::vector_tile::VERSION_TAG, 2); // version
            // Field 1 is the "layer name" field, it's a string
            line_layer_writer.add_string(util::vector_tile::NAME_TAG, "speeds"); // name
            // Field 5 is the tile extent.  It's a uint32 and should be set to 4096
            // for normal vector tiles.
            line_layer_writer.add_uint32(util::vector_tile::EXTENT_TAG,
                                         util::vector_tile::EXTENT); // extent

            // Because we need to know the indexes into the vector tile lookup table,
            // we need to do an initial pass over the data and create the complete
            // index of used values.
            for (const auto &edge_index : sorted_edge_indexes)
            {
                const auto &edge = edges[edge_index];

                // Weight values
                const auto forward_weight_vector =
                    facade.GetUncompressedForwardWeights(edge.packed_geometry_id);
                const auto reverse_weight_vector =
                    facade.GetUncompressedReverseWeights(edge.packed_geometry_id);
                const auto forward_weight = forward_weight_vector[edge.fwd_segment_position];
                const auto reverse_weight = reverse_weight_vector[reverse_weight_vector.size() -
                                                                  edge.fwd_segment_position - 1];
                use_line_value(forward_weight);
                use_line_value(reverse_weight);

                // Duration values
                const auto forward_duration_vector =
                    facade.GetUncompressedForwardDurations(edge.packed_geometry_id);
                const auto reverse_duration_vector =
                    facade.GetUncompressedReverseDurations(edge.packed_geometry_id);
                const auto forward_duration = forward_duration_vector[edge.fwd_segment_position];
                const auto reverse_duration =
                    reverse_duration_vector[reverse_duration_vector.size() -
                                            edge.fwd_segment_position - 1];
                use_line_value(forward_duration);
                use_line_value(reverse_duration);
            }

            // Begin the layer features block
            {
                // Each feature gets a unique id, starting at 1
                unsigned id = 1;
                for (const auto &edge_index : sorted_edge_indexes)
                {
                    const auto &edge = edges[edge_index];
                    // Get coordinates for start/end nodes of segment (NodeIDs u and v)
                    const auto a = facade.GetCoordinateOfNode(edge.u);
                    const auto b = facade.GetCoordinateOfNode(edge.v);
                    // Calculate the length in meters
                    const double length =
                        osrm::util::coordinate_calculation::haversineDistance(a, b);

                    const auto forward_weight_vector =
                        facade.GetUncompressedForwardWeights(edge.packed_geometry_id);
                    const auto reverse_weight_vector =
                        facade.GetUncompressedReverseWeights(edge.packed_geometry_id);
                    const auto forward_duration_vector =
                        facade.GetUncompressedForwardDurations(edge.packed_geometry_id);
                    const auto reverse_duration_vector =
                        facade.GetUncompressedReverseDurations(edge.packed_geometry_id);
                    const auto forward_datasource_vector =
                        facade.GetUncompressedForwardDatasources(edge.packed_geometry_id);
                    const auto reverse_datasource_vector =
                        facade.GetUncompressedReverseDatasources(edge.packed_geometry_id);
                    const auto forward_weight = forward_weight_vector[edge.fwd_segment_position];
                    const auto reverse_weight =
                        reverse_weight_vector[reverse_weight_vector.size() -
                                              edge.fwd_segment_position - 1];
                    const auto forward_duration =
                        forward_duration_vector[edge.fwd_segment_position];
                    const auto reverse_duration =
                        reverse_duration_vector[reverse_duration_vector.size() -
                                                edge.fwd_segment_position - 1];
                    const auto forward_datasource =
                        forward_datasource_vector[edge.fwd_segment_position];
                    const auto reverse_datasource =
                        reverse_datasource_vector[reverse_datasource_vector.size() -
                                                  edge.fwd_segment_position - 1];

                    auto name = facade.GetNameForID(edge.name_id);

                    const auto name_offset = [&name, &names, &name_offsets]() {
                        auto iter = name_offsets.find(name);
                        if (iter == name_offsets.end())
                        {
                            auto offset = names.size();
                            name_offsets[name] = offset;
                            names.push_back(name);
                            return offset;
                        }
                        return iter->second;
                    }();

                    const auto encode_tile_line = [&line_layer_writer,
                                                   &edge,
                                                   &id,
                                                   &max_datasource_id,
                                                   &used_line_ints](const FixedLine &tile_line,
                                                                    const std::uint32_t speed_kmh,
                                                                    const std::size_t weight,
                                                                    const std::size_t duration,
                                                                    const DatasourceID datasource,
                                                                    const std::size_t name_idx,
                                                                    std::int32_t &start_x,
                                                                    std::int32_t &start_y) {
                        // Here, we save the two attributes for our feature: the speed and
                        // the is_small boolean.  We only serve up speeds from 0-139, so all we
                        // do is save the first
                        protozero::pbf_writer feature_writer(line_layer_writer,
                                                             util::vector_tile::FEATURE_TAG);
                        // Field 3 is the "geometry type" field.  Value 2 is "line"
                        feature_writer.add_enum(
                            util::vector_tile::GEOMETRY_TAG,
                            util::vector_tile::GEOMETRY_TYPE_LINE); // geometry type
                        // Field 1 for the feature is the "id" field.
                        feature_writer.add_uint64(util::vector_tile::ID_TAG, id++); // id
                        {
                            // When adding attributes to a feature, we have to write
                            // pairs of numbers.  The first value is the index in the
                            // keys array (written later), and the second value is the
                            // index into the "values" array (also written later).  We're
                            // not writing the actual speed or bool value here, we're saving
                            // an index into the "values" array.  This means many features
                            // can share the same value data, leading to smaller tiles.
                            protozero::packed_field_uint32 field(
                                feature_writer, util::vector_tile::FEATURE_ATTRIBUTES_TAG);

                            field.add_element(0); // "speed" tag key offset
                            field.add_element(
                                std::min(speed_kmh, 127u)); // save the speed value, capped at 127
                            field.add_element(1);           // "is_small" tag key offset
                            field.add_element(128 +
                                              (edge.component.is_tiny ? 0 : 1)); // is_small feature
                            field.add_element(2);                // "datasource" tag key offset
                            field.add_element(130 + datasource); // datasource value offset
                            field.add_element(3);                // "weight" tag key offset
                            field.add_element(130 + max_datasource_id + 1 +
                                              weight); // weight value offset
                            field.add_element(4);      // "duration" tag key offset
                            field.add_element(130 + max_datasource_id + 1 +
                                              duration); // duration value offset
                            field.add_element(5);        // "name" tag key offset

                            field.add_element(130 + max_datasource_id + 1 + used_line_ints.size() +
                                              name_idx); // name value offset
                        }
                        {

                            // Encode the geometry for the feature
                            protozero::packed_field_uint32 geometry(
                                feature_writer, util::vector_tile::FEATURE_GEOMETRIES_TAG);
                            encodeLinestring(tile_line, geometry, start_x, start_y);
                        }
                    };

                    // If this is a valid forward edge, go ahead and add it to the tile
                    if (forward_duration != 0 && edge.forward_segment_id.enabled)
                    {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        // Calculate the speed for this line
                        std::uint32_t speed_kmh =
                            static_cast<std::uint32_t>(round(length / forward_duration * 10 * 3.6));

                        auto tile_line = coordinatesToTileLine(a, b, tile_bbox);
                        if (!tile_line.empty())
                        {
                            encode_tile_line(tile_line,
                                             speed_kmh,
                                             line_int_offsets[forward_weight],
                                             line_int_offsets[forward_duration],
                                             forward_datasource,
                                             name_offset,
                                             start_x,
                                             start_y);
                        }
                    }

                    // Repeat the above for the coordinates reversed and using the `reverse`
                    // properties
                    if (reverse_duration != 0 && edge.reverse_segment_id.enabled)
                    {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        // Calculate the speed for this line
                        std::uint32_t speed_kmh =
                            static_cast<std::uint32_t>(round(length / reverse_duration * 10 * 3.6));

                        auto tile_line = coordinatesToTileLine(b, a, tile_bbox);
                        if (!tile_line.empty())
                        {
                            encode_tile_line(tile_line,
                                             speed_kmh,
                                             line_int_offsets[reverse_weight],
                                             line_int_offsets[reverse_duration],
                                             reverse_datasource,
                                             name_offset,
                                             start_x,
                                             start_y);
                        }
                    }
                }
            }

            // Field id 3 is the "keys" attribute
            // We need two "key" fields, these are referred to with 0 and 1 (their array
            // indexes) earlier
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "speed");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "is_small");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "datasource");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "weight");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "duration");
            line_layer_writer.add_string(util::vector_tile::KEY_TAG, "name");

            // Now, we write out the possible speed value arrays and possible is_tiny
            // values.  Field type 4 is the "values" field.  It's a variable type field,
            // so requires a two-step write (create the field, then write its value)
            for (std::size_t i = 0; i < 128; i++)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 5 == uint64 type
                values_writer.add_uint64(util::vector_tile::VARIANT_TYPE_UINT64, i);
            }
            {
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 7 == bool type
                values_writer.add_bool(util::vector_tile::VARIANT_TYPE_BOOL, true);
            }
            {
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 7 == bool type
                values_writer.add_bool(util::vector_tile::VARIANT_TYPE_BOOL, false);
            }
            for (std::size_t i = 0; i <= max_datasource_id; i++)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 1 == string type
                values_writer.add_string(util::vector_tile::VARIANT_TYPE_STRING,
                                         facade.GetDatasourceName(i).to_string());
            }
            for (auto value : used_line_ints)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 2 == float type
                // Durations come out of OSRM in integer deciseconds, so we convert them
                // to seconds with a simple /10 for display
                values_writer.add_double(util::vector_tile::VARIANT_TYPE_DOUBLE, value / 10.);
            }

            for (const auto &name : names)
            {
                // Writing field type 4 == variant type
                protozero::pbf_writer values_writer(line_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                // Attribute value 1 == string type
                values_writer.add_string(
                    util::vector_tile::VARIANT_TYPE_STRING, name.data(), name.size());
            }
        }

        // Only add the turn layer to the tile if it has some features (we sometimes won't
        // for tiles of z<16, and tiles that don't show any intersections)
        if (!all_turn_data.empty())
        {
            // we need to pre-encode all values here because we need the full offsets later
            // for encoding the actual features.
            std::vector<std::tuple<util::Coordinate, unsigned, unsigned, unsigned>>
                encoded_turn_data(all_turn_data.size());
            std::transform(
                all_turn_data.begin(),
                all_turn_data.end(),
                encoded_turn_data.begin(),
                [&](const routing_algorithms::TurnData &t) {
                    auto angle_idx = use_point_int_value(t.in_angle);
                    auto turn_idx = use_point_int_value(t.turn_angle);
                    auto duration_idx =
                        use_point_float_value(t.duration / 10.0); // Note conversion to float here
                    return std::make_tuple(t.coordinate, angle_idx, turn_idx, duration_idx);
                });

            // Now write the points layer for turn penalty data:
            // Add a layer object to the PBF stream.  3=='layer' from the vector tile spec
            // (2.1)
            protozero::pbf_writer point_layer_writer(tile_writer, util::vector_tile::LAYER_TAG);
            point_layer_writer.add_uint32(util::vector_tile::VERSION_TAG, 2);    // version
            point_layer_writer.add_string(util::vector_tile::NAME_TAG, "turns"); // name
            point_layer_writer.add_uint32(util::vector_tile::EXTENT_TAG,
                                          util::vector_tile::EXTENT); // extent

            // Begin writing the set of point features
            {
                // Start each features with an ID starting at 1
                int id = 1;

                // Helper function to encode a new point feature on a vector tile.
                const auto encode_tile_point = [&](const FixedPoint &tile_point,
                                                   const auto &point_turn_data) {
                    protozero::pbf_writer feature_writer(point_layer_writer,
                                                         util::vector_tile::FEATURE_TAG);
                    // Field 3 is the "geometry type" field.  Value 1 is "point"
                    feature_writer.add_enum(
                        util::vector_tile::GEOMETRY_TAG,
                        util::vector_tile::GEOMETRY_TYPE_POINT);                // geometry type
                    feature_writer.add_uint64(util::vector_tile::ID_TAG, id++); // id
                    {
                        // Write out the 3 properties we want on the feature.  These
                        // refer to indexes in the properties lookup table, which we
                        // add to the tile after we add all features.
                        protozero::packed_field_uint32 field(
                            feature_writer, util::vector_tile::FEATURE_ATTRIBUTES_TAG);
                        field.add_element(0); // "bearing_in" tag key offset
                        field.add_element(std::get<1>(point_turn_data));
                        field.add_element(1); // "turn_angle" tag key offset
                        field.add_element(std::get<2>(point_turn_data));
                        field.add_element(2); // "cost" tag key offset
                        field.add_element(used_point_ints.size() + std::get<3>(point_turn_data));
                    }
                    {
                        // Add the geometry as the last field in this feature
                        protozero::packed_field_uint32 geometry(
                            feature_writer, util::vector_tile::FEATURE_GEOMETRIES_TAG);
                        encodePoint(tile_point, geometry);
                    }
                };

                // Loop over all the turns we found and add them as features to the layer
                for (const auto &turndata : encoded_turn_data)
                {
                    const auto tile_point =
                        coordinatesToTilePoint(std::get<0>(turndata), tile_bbox);
                    if (!boost::geometry::within(point_t(tile_point.x, tile_point.y), clip_box))
                    {
                        continue;
                    }
                    encode_tile_point(tile_point, turndata);
                }
            }

            // Add the names of the three attributes we added to all the turn penalty
            // features previously.  The indexes used there refer to these keys.
            point_layer_writer.add_string(util::vector_tile::KEY_TAG, "bearing_in");
            point_layer_writer.add_string(util::vector_tile::KEY_TAG, "turn_angle");
            point_layer_writer.add_string(util::vector_tile::KEY_TAG, "cost");

            // Now, save the lists of integers and floats that our features refer to.
            for (const auto &value : used_point_ints)
            {
                protozero::pbf_writer values_writer(point_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                values_writer.add_sint64(util::vector_tile::VARIANT_TYPE_SINT64, value);
            }
            for (const auto &value : used_point_floats)
            {
                protozero::pbf_writer values_writer(point_layer_writer,
                                                    util::vector_tile::VARIANT_TAG);
                values_writer.add_float(util::vector_tile::VARIANT_TYPE_FLOAT, value);
            }
        }
    }
    // protozero serializes data during object destructors, so once the scope closes,
    // our result buffer will have all the tile data encoded into it.
}
}

Status TilePlugin::HandleRequest(const datafacade::ContiguousInternalMemoryDataFacadeBase &facade,
                                 const RoutingAlgorithmsInterface &algorithms,
                                 const api::TileParameters &parameters,
                                 std::string &pbf_buffer) const
{
    BOOST_ASSERT(parameters.IsValid());

    auto edges = getEdges(facade, parameters.x, parameters.y, parameters.z);

    auto edge_index = getEdgeIndex(edges);

    std::vector<routing_algorithms::TurnData> turns;

    // If we're zooming into 16 or higher, include turn data.  Why?  Because turns make the map
    // really cramped, so we don't bother including the data for tiles that span a large area.
    if (parameters.z >= MIN_ZOOM_FOR_TURNS && algorithms.HasGetTileTurns())
    {
        turns = algorithms.GetTileTurns(edges, edge_index);
    }

    encodeVectorTile(
        facade, parameters.x, parameters.y, parameters.z, edges, edge_index, turns, pbf_buffer);

    return Status::Ok;
}
}
}
}
