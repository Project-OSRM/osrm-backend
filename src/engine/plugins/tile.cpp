#include "engine/plugins/tile.hpp"
#include "engine/plugins/plugin_base.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/vector_tile.hpp"
#include "util/web_mercator.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>

#include <protozero/pbf_writer.hpp>
#include <protozero/varint.hpp>

#include <string>
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
namespace detail
{
// Simple container class for WGS84 coordinates
template <typename T> struct Point final
{
    Point(T _x, T _y) : x(_x), y(_y) {}

    const T x;
    const T y;
};

// from mapnik-vector-tile
namespace pbf
{
inline unsigned encode_length(const unsigned len) { return (len << 3u) | 2u; }
}

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

using FixedLine = std::vector<detail::Point<std::int32_t>>;
using FloatLine = std::vector<detail::Point<double>>;

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

    const unsigned line_to_length = static_cast<const unsigned>(line_size) - 1;

    auto pt = line.begin();
    geometry.add_element(9); // move_to | (1 << 3)
    geometry.add_element(protozero::encode_zigzag32(pt->x - start_x));
    geometry.add_element(protozero::encode_zigzag32(pt->y - start_y));
    start_x = pt->x;
    start_y = pt->y;
    geometry.add_element(detail::pbf::encode_length(line_to_length));
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

FixedLine coordinatesToTileLine(const util::Coordinate start,
                                const util::Coordinate target,
                                const detail::BBox &tile_bbox)
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
}

Status TilePlugin::HandleRequest(const api::TileParameters &parameters, std::string &pbf_buffer)
{
    BOOST_ASSERT(parameters.IsValid());

    double min_lon, min_lat, max_lon, max_lat;

    // Convert the z,x,y mercator tile coordinates into WGS84 lon/lat values
    util::web_mercator::xyzToWGS84(
        parameters.x, parameters.y, parameters.z, min_lon, min_lat, max_lon, max_lat);

    util::Coordinate southwest{util::FloatLongitude{min_lon}, util::FloatLatitude{min_lat}};
    util::Coordinate northeast{util::FloatLongitude{max_lon}, util::FloatLatitude{max_lat}};

    // Fetch all the segments that are in our bounding box.
    // This hits the OSRM StaticRTree
    const auto edges = facade.GetEdgesInBox(southwest, northeast);

    std::vector<int> used_weights;
    std::unordered_map<int, std::size_t> weight_offsets;
    uint8_t max_datasource_id = 0;
    std::vector<std::string> names;
    std::unordered_map<std::string, std::size_t> name_offsets;

    // Loop over all edges once to tally up all the attributes we'll need.
    // We need to do this so that we know the attribute offsets to use
    // when we encode each feature in the tile.
    for (const auto &edge : edges)
    {
        int forward_weight = 0, reverse_weight = 0;
        uint8_t forward_datasource = 0;
        uint8_t reverse_datasource = 0;

        if (edge.forward_packed_geometry_id != SPECIAL_EDGEID)
        {
            std::vector<EdgeWeight> forward_weight_vector;
            facade.GetUncompressedWeights(edge.forward_packed_geometry_id, forward_weight_vector);
            forward_weight = forward_weight_vector[edge.fwd_segment_position];

            std::vector<uint8_t> forward_datasource_vector;
            facade.GetUncompressedDatasources(edge.forward_packed_geometry_id,
                                              forward_datasource_vector);
            forward_datasource = forward_datasource_vector[edge.fwd_segment_position];

            if (weight_offsets.find(forward_weight) == weight_offsets.end())
            {
                used_weights.push_back(forward_weight);
                weight_offsets[forward_weight] = used_weights.size() - 1;
            }
        }

        if (edge.reverse_packed_geometry_id != SPECIAL_EDGEID)
        {
            std::vector<EdgeWeight> reverse_weight_vector;
            facade.GetUncompressedWeights(edge.reverse_packed_geometry_id, reverse_weight_vector);

            BOOST_ASSERT(edge.fwd_segment_position < reverse_weight_vector.size());

            reverse_weight =
                reverse_weight_vector[reverse_weight_vector.size() - edge.fwd_segment_position - 1];

            if (weight_offsets.find(reverse_weight) == weight_offsets.end())
            {
                used_weights.push_back(reverse_weight);
                weight_offsets[reverse_weight] = used_weights.size() - 1;
            }
            std::vector<uint8_t> reverse_datasource_vector;
            facade.GetUncompressedDatasources(edge.reverse_packed_geometry_id,
                                              reverse_datasource_vector);
            reverse_datasource = reverse_datasource_vector[reverse_datasource_vector.size() -
                                                           edge.fwd_segment_position - 1];
        }
        // Keep track of the highest datasource seen so that we don't write unnecessary
        // data to the layer attribute values
        max_datasource_id = std::max(max_datasource_id, forward_datasource);
        max_datasource_id = std::max(max_datasource_id, reverse_datasource);

        std::string name = facade.GetNameForID(edge.name_id);

        if (name_offsets.find(name) == name_offsets.end())
        {
            names.push_back(name);
            name_offsets[name] = names.size() - 1;
        }
    }

    // TODO: extract speed values for compressed and uncompressed geometries

    // Convert tile coordinates into mercator coordinates
    util::web_mercator::xyzToMercator(
        parameters.x, parameters.y, parameters.z, min_lon, min_lat, max_lon, max_lat);
    const detail::BBox tile_bbox{min_lon, min_lat, max_lon, max_lat};

    // Protobuf serialized blocks when objects go out of scope, hence
    // the extra scoping below.
    protozero::pbf_writer tile_writer{pbf_buffer};
    {
        // Add a layer object to the PBF stream.  3=='layer' from the vector tile spec (2.1)
        protozero::pbf_writer layer_writer(tile_writer, util::vector_tile::LAYER_TAG);
        // TODO: don't write a layer if there are no features

        layer_writer.add_uint32(util::vector_tile::VERSION_TAG, 2); // version
        // Field 1 is the "layer name" field, it's a string
        layer_writer.add_string(util::vector_tile::NAME_TAG, "speeds"); // name
        // Field 5 is the tile extent.  It's a uint32 and should be set to 4096
        // for normal vector tiles.
        layer_writer.add_uint32(util::vector_tile::EXTEND_TAG, util::vector_tile::EXTENT); // extent

        // Begin the layer features block
        {
            // Each feature gets a unique id, starting at 1
            unsigned id = 1;
            for (const auto &edge : edges)
            {
                // Get coordinates for start/end nodes of segmet (NodeIDs u and v)
                const auto a = facade.GetCoordinateOfNode(edge.u);
                const auto b = facade.GetCoordinateOfNode(edge.v);
                // Calculate the length in meters
                const double length = osrm::util::coordinate_calculation::haversineDistance(a, b);

                int forward_weight = 0;
                int reverse_weight = 0;

                uint8_t forward_datasource = 0;
                uint8_t reverse_datasource = 0;

                std::string name = facade.GetNameForID(edge.name_id);

                if (edge.forward_packed_geometry_id != SPECIAL_EDGEID)
                {
                    std::vector<EdgeWeight> forward_weight_vector;
                    facade.GetUncompressedWeights(edge.forward_packed_geometry_id,
                                                  forward_weight_vector);
                    forward_weight = forward_weight_vector[edge.fwd_segment_position];

                    std::vector<uint8_t> forward_datasource_vector;
                    facade.GetUncompressedDatasources(edge.forward_packed_geometry_id,
                                                      forward_datasource_vector);
                    forward_datasource = forward_datasource_vector[edge.fwd_segment_position];
                }

                if (edge.reverse_packed_geometry_id != SPECIAL_EDGEID)
                {
                    std::vector<EdgeWeight> reverse_weight_vector;
                    facade.GetUncompressedWeights(edge.reverse_packed_geometry_id,
                                                  reverse_weight_vector);

                    BOOST_ASSERT(edge.fwd_segment_position < reverse_weight_vector.size());

                    reverse_weight = reverse_weight_vector[reverse_weight_vector.size() -
                                                           edge.fwd_segment_position - 1];

                    std::vector<uint8_t> reverse_datasource_vector;
                    facade.GetUncompressedDatasources(edge.reverse_packed_geometry_id,
                                                      reverse_datasource_vector);
                    reverse_datasource =
                        reverse_datasource_vector[reverse_datasource_vector.size() -
                                                  edge.fwd_segment_position - 1];
                }

                // Keep track of the highest datasource seen so that we don't write unnecessary
                // data to the layer attribute values
                max_datasource_id = std::max(max_datasource_id, forward_datasource);
                max_datasource_id = std::max(max_datasource_id, reverse_datasource);

                const auto encode_tile_line = [&layer_writer,
                                               &edge,
                                               &id,
                                               &max_datasource_id,
                                               &used_weights](const detail::FixedLine &tile_line,
                                                              const std::uint32_t speed_kmh,
                                                              const std::size_t duration,
                                                              const DatasourceID datasource,
                                                              const std::size_t name,
                                                              std::int32_t &start_x,
                                                              std::int32_t &start_y) {
                    // Here, we save the two attributes for our feature: the speed and the
                    // is_small
                    // boolean.  We onl serve up speeds from 0-139, so all we do is save the
                    // first
                    protozero::pbf_writer feature_writer(layer_writer,
                                                         util::vector_tile::FEATURE_TAG);
                    // Field 3 is the "geometry type" field.  Value 2 is "line"
                    feature_writer.add_enum(util::vector_tile::GEOMETRY_TAG,
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
                        field.add_element(3);                // "duration" tag key offset
                        field.add_element(130 + max_datasource_id + 1 +
                                          duration); // duration value offset
                        field.add_element(4);        // "name" tag key offset

                        field.add_element(130 + max_datasource_id + 1 + used_weights.size() +
                                          name); // name value offset
                    }
                    {

                        // Encode the geometry for the feature
                        protozero::packed_field_uint32 geometry(
                            feature_writer, util::vector_tile::FEATURE_GEOMETRIES_TAG);
                        encodeLinestring(tile_line, geometry, start_x, start_y);
                    }
                };

                // If this is a valid forward edge, go ahead and add it to the tile
                if (forward_weight != 0 && edge.forward_segment_id.enabled)
                {
                    std::int32_t start_x = 0;
                    std::int32_t start_y = 0;

                    // Calculate the speed for this line
                    std::uint32_t speed_kmh =
                        static_cast<std::uint32_t>(round(length / forward_weight * 10 * 3.6));

                    auto tile_line = coordinatesToTileLine(a, b, tile_bbox);
                    if (!tile_line.empty())
                    {
                        encode_tile_line(tile_line,
                                         speed_kmh,
                                         weight_offsets[forward_weight],
                                         forward_datasource,
                                         name_offsets[name],
                                         start_x,
                                         start_y);
                    }
                }

                // Repeat the above for the coordinates reversed and using the `reverse`
                // properties
                if (reverse_weight != 0 && edge.reverse_segment_id.enabled)
                {
                    std::int32_t start_x = 0;
                    std::int32_t start_y = 0;

                    // Calculate the speed for this line
                    std::uint32_t speed_kmh =
                        static_cast<std::uint32_t>(round(length / reverse_weight * 10 * 3.6));

                    auto tile_line = coordinatesToTileLine(b, a, tile_bbox);
                    if (!tile_line.empty())
                    {
                        encode_tile_line(tile_line,
                                         speed_kmh,
                                         weight_offsets[reverse_weight],
                                         reverse_datasource,
                                         name_offsets[name],
                                         start_x,
                                         start_y);
                    }
                }
            }
        }

        // Field id 3 is the "keys" attribute
        // We need two "key" fields, these are referred to with 0 and 1 (their array indexes)
        // earlier
        layer_writer.add_string(util::vector_tile::KEY_TAG, "speed");
        layer_writer.add_string(util::vector_tile::KEY_TAG, "is_small");
        layer_writer.add_string(util::vector_tile::KEY_TAG, "datasource");
        layer_writer.add_string(util::vector_tile::KEY_TAG, "duration");
        layer_writer.add_string(util::vector_tile::KEY_TAG, "name");

        // Now, we write out the possible speed value arrays and possible is_tiny
        // values.  Field type 4 is the "values" field.  It's a variable type field,
        // so requires a two-step write (create the field, then write its value)
        for (std::size_t i = 0; i < 128; i++)
        {
            // Writing field type 4 == variant type
            protozero::pbf_writer values_writer(layer_writer, util::vector_tile::VARIANT_TAG);
            // Attribute value 5 == uin64 type
            values_writer.add_uint64(util::vector_tile::VARIANT_TYPE_UINT32, i);
        }
        {
            protozero::pbf_writer values_writer(layer_writer, util::vector_tile::VARIANT_TAG);
            // Attribute value 7 == bool type
            values_writer.add_bool(util::vector_tile::VARIANT_TYPE_BOOL, true);
        }
        {
            protozero::pbf_writer values_writer(layer_writer, util::vector_tile::VARIANT_TAG);
            // Attribute value 7 == bool type
            values_writer.add_bool(util::vector_tile::VARIANT_TYPE_BOOL, false);
        }
        for (std::size_t i = 0; i <= max_datasource_id; i++)
        {
            // Writing field type 4 == variant type
            protozero::pbf_writer values_writer(layer_writer, util::vector_tile::VARIANT_TAG);
            // Attribute value 1 == string type
            values_writer.add_string(util::vector_tile::VARIANT_TYPE_STRING,
                                     facade.GetDatasourceName(i));
        }
        for (auto weight : used_weights)
        {
            // Writing field type 4 == variant type
            protozero::pbf_writer values_writer(layer_writer, util::vector_tile::VARIANT_TAG);
            // Attribute value 2 == float type
            // Durations come out of OSRM in integer deciseconds, so we convert them
            // to seconds with a simple /10 for display
            values_writer.add_double(util::vector_tile::VARIANT_TYPE_DOUBLE, weight / 10.);
        }

        for (const auto &name : names)
        {
            // Writing field type 4 == variant type
            protozero::pbf_writer values_writer(layer_writer, util::vector_tile::VARIANT_TAG);
            // Attribute value 1 == string type
            values_writer.add_string(util::vector_tile::VARIANT_TYPE_STRING, name);
        }
    }

    return Status::Ok;
}
}
}
}
