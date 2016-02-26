#ifndef TILEPLUGIN_HPP
#define TILEPLUGIN_HPP

#include "engine/plugins/plugin_base.hpp"
#include "osrm/json_container.hpp"

#include <protozero/varint.hpp>
#include <protozero/pbf_writer.hpp>

#include <string>
#include <vector>
#include <utility>

#include <cmath>
#include <cstdint>

/*
 * This plugin generates Mapbox Vector tiles that show the internal
 * routing geometry and speed values on all road segments.
 * You can use this along with a vector-tile viewer, like Mapbox GL,
 * to display maps that show the exact road network that
 * OSRM is routing.  This is very useful for debugging routing
 * errors
 */
namespace osrm
{
namespace engine
{
namespace plugins
{

// from mapnik/well_known_srs.hpp
const constexpr double EARTH_RADIUS = 6378137.0;
const constexpr double EARTH_DIAMETER = EARTH_RADIUS * 2.0;
const constexpr double EARTH_CIRCUMFERENCE = EARTH_DIAMETER * M_PI;
const constexpr double MAXEXTENT = EARTH_CIRCUMFERENCE / 2.0;
const constexpr double M_PI_by2 = M_PI / 2.0;
const constexpr double D2R = M_PI / 180.0;
const constexpr double R2D = 180.0 / M_PI;
const constexpr double M_PIby360 = M_PI / 360.0;
const constexpr double MAXEXTENTby180 = MAXEXTENT / 180.0;
const constexpr double MAX_LATITUDE = R2D * (2.0 * std::atan(std::exp(180.0 * D2R)) - M_PI_by2);

// from mapnik-vector-tile
namespace detail_pbf
{

inline unsigned encode_length(unsigned len) { return (len << 3u) | 2u; }
}

// Converts a regular WSG84 lon/lat pair into
// a mercator coordinate
inline void lonlat2merc(double &x, double &y)
{
    if (x > 180)
        x = 180;
    else if (x < -180)
        x = -180;
    if (y > MAX_LATITUDE)
        y = MAX_LATITUDE;
    else if (y < -MAX_LATITUDE)
        y = -MAX_LATITUDE;
    x = x * MAXEXTENTby180;
    y = std::log(std::tan((90 + y) * M_PIby360)) * R2D;
    y = y * MAXEXTENTby180;
}

// This is the global default tile size for all Mapbox Vector Tiles
const constexpr double tile_size_ = 256.0;

//
inline void from_pixels(double shift, double &x, double &y)
{
    double b = shift / 2.0;
    x = (x - b) / (shift / 360.0);
    double g = (y - b) / -(shift / (2 * M_PI));
    y = R2D * (2.0 * std::atan(std::exp(g)) - M_PI_by2);
}

// Converts a WMS tile coordinate (z,x,y) into a mercator bounding box
inline void
xyz2mercator(int x, int y, int z, double &minx, double &miny, double &maxx, double &maxy)
{
    minx = x * tile_size_;
    miny = (y + 1.0) * tile_size_;
    maxx = (x + 1.0) * tile_size_;
    maxy = y * tile_size_;
    double shift = std::pow(2.0, z) * tile_size_;
    from_pixels(shift, minx, miny);
    from_pixels(shift, maxx, maxy);
    lonlat2merc(minx, miny);
    lonlat2merc(maxx, maxy);
}

// Converts a WMS tile coordinate (z,x,y) into a wsg84 bounding box
inline void xyz2wsg84(int x, int y, int z, double &minx, double &miny, double &maxx, double &maxy)
{
    minx = x * tile_size_;
    miny = (y + 1.0) * tile_size_;
    maxx = (x + 1.0) * tile_size_;
    maxy = y * tile_size_;
    double shift = std::pow(2.0, z) * tile_size_;
    from_pixels(shift, minx, miny);
    from_pixels(shift, maxx, maxy);
}

// emulates mapbox::box2d, just a simple container for
// a box
class bbox
{
  public:
    double minx;
    double miny;
    double maxx;
    double maxy;
    bbox(double _minx, double _miny, double _maxx, double _maxy)
        : minx(_minx), miny(_miny), maxx(_maxx), maxy(_maxy)
    {
    }

    double width() const { return maxx - minx; }

    double height() const { return maxy - miny; }
};

// Simple container class for WSG84 coordinates
class point_type_d
{
  public:
    double x;
    double y;
    point_type_d(double _x, double _y) : x(_x), y(_y) {}
};

// Simple container for integer coordinates (i.e. pixel coords)
class point_type_i
{
  public:
    std::int64_t x;
    std::int64_t y;
    point_type_i(std::int64_t _x, std::int64_t _y) : x(_x), y(_y) {}
};

using line_type = std::vector<point_type_i>;
using line_typed = std::vector<point_type_d>;

// from mapnik-vector-tile
// Encodes a linestring using protobuf zigzag encoding
inline bool encode_linestring(line_type line,
                              protozero::packed_field_uint32 &geometry,
                              int32_t &start_x,
                              int32_t &start_y)
{
    std::size_t line_size = line.size();
    // line_size -= detail_pbf::repeated_point_count(line);
    if (line_size < 2)
    {
        return false;
    }

    unsigned line_to_length = static_cast<unsigned>(line_size) - 1;

    auto pt = line.begin();
    geometry.add_element(9); // move_to | (1 << 3)
    geometry.add_element(protozero::encode_zigzag32(pt->x - start_x));
    geometry.add_element(protozero::encode_zigzag32(pt->y - start_y));
    start_x = pt->x;
    start_y = pt->y;
    geometry.add_element(detail_pbf::encode_length(line_to_length));
    for (++pt; pt != line.end(); ++pt)
    {
        int32_t dx = pt->x - start_x;
        int32_t dy = pt->y - start_y;
        /*if (dx == 0 && dy == 0)
        {
            continue;
        }*/
        geometry.add_element(protozero::encode_zigzag32(dx));
        geometry.add_element(protozero::encode_zigzag32(dy));
        start_x = pt->x;
        start_y = pt->y;
    }
    return true;
}

template <class DataFacadeT> class TilePlugin final : public BasePlugin
{
  public:
    explicit TilePlugin(DataFacadeT *facade) : facade(facade), descriptor_string("tile") {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    Status HandleRequest(const RouteParameters &route_parameters,
                         util::json::Object &json_result) override final
    {

        // Vector tiles are 4096 virtual pixels on each side
        const double tile_extent = 4096.0;
        double min_lon, min_lat, max_lon, max_lat;

        // Convert the z,x,y mercator tile coordinates into WSG84 lon/lat values
        xyz2wsg84(route_parameters.x, route_parameters.y, route_parameters.z, min_lon, min_lat,
                  max_lon, max_lat);

        FixedPointCoordinate southwest = {static_cast<int32_t>(min_lat * COORDINATE_PRECISION),
                                          static_cast<int32_t>(min_lon * COORDINATE_PRECISION)};
        FixedPointCoordinate northeast = {static_cast<int32_t>(max_lat * COORDINATE_PRECISION),
                                          static_cast<int32_t>(max_lon * COORDINATE_PRECISION)};

        // Fetch all the segments that are in our bounding box.
        // This hits the OSRM StaticRTree
        auto edges = facade->GetEdgesInBox(southwest, northeast);

        // TODO: extract speed values for compressed and uncompressed geometries

        // Convert tile coordinates into mercator coordinates
        xyz2mercator(route_parameters.x, route_parameters.y, route_parameters.z, min_lon, min_lat,
                     max_lon, max_lat);
        bbox tile_bbox(min_lon, min_lat, max_lon, max_lat);

        // Protobuf serialized blocks when objects go out of scope, hence
        // the extra scoping below.
        std::string buffer;
        protozero::pbf_writer tile_writer(buffer);
        {
            // Add a layer object to the PBF stream.  3=='layer' from the vector tile spec (2.1)
            protozero::pbf_writer layer_writer(tile_writer, 3);
            // TODO: don't write a layer if there are no features
            // Field 15 is the "version field, and it's a uint32
            layer_writer.add_uint32(15, 2); // version
            // Field 1 is the "layer name" field, it's a string
            layer_writer.add_string(1, "speeds"); // name
            // Field 5 is the tile extent.  It's a uint32 and should be set to 4096
            // for normal vector tiles.
            layer_writer.add_uint32(5, 4096); // extent

            // Begin the layer features block
            {
                // Each feature gets a unique id, starting at 1
                unsigned id = 1;
                for (const auto &edge : edges)
                {
                    // Get coordinates for start/end nodes of segmet (NodeIDs u and v)
                    const auto a = facade->GetCoordinateOfNode(edge.u);
                    const auto b = facade->GetCoordinateOfNode(edge.v);
                    // Calculate the length in meters
                    double length = osrm::util::coordinate_calculation::haversineDistance(
                        a.lon, a.lat, b.lon, b.lat);

                    // If this is a valid forward edge, go ahead and add it to the tile
                    if (edge.forward_weight != 0 &&
                        edge.forward_edge_based_node_id != SPECIAL_NODEID)
                    {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        line_typed geo_line;
                        geo_line.emplace_back(a.lon / COORDINATE_PRECISION,
                                              a.lat / COORDINATE_PRECISION);
                        geo_line.emplace_back(b.lon / COORDINATE_PRECISION,
                                              b.lat / COORDINATE_PRECISION);

                        // Calculate the speed for this line
                        uint32_t speed =
                            static_cast<uint32_t>(round(length / edge.forward_weight * 10 * 3.6));

                        line_type tile_line;
                        for (auto const &pt : geo_line)
                        {
                            double px_merc = pt.x;
                            double py_merc = pt.y;
                            lonlat2merc(px_merc, py_merc);
                            // convert lon/lat to tile coordinates
                            const auto px = std::round(((px_merc - tile_bbox.minx) * tile_extent /
                                                        16.0 / tile_bbox.width()) *
                                                       tile_extent / 256.0);
                            const auto py = std::round(((tile_bbox.maxy - py_merc) * tile_extent /
                                                        16.0 / tile_bbox.height()) *
                                                       tile_extent / 256.0);
                            tile_line.emplace_back(px, py);
                        }

                        // Here, we save the two attributes for our feature: the speed and the
                        // is_small
                        // boolean.  We onl serve up speeds from 0-139, so all we do is save the
                        // first
                        protozero::pbf_writer feature_writer(layer_writer, 2);
                        // Field 3 is the "geometry type" field.  Value 2 is "line"
                        feature_writer.add_enum(3, 2); // geometry type
                        // Field 1 for the feature is the "id" field.
                        feature_writer.add_uint64(1, id++); // id
                        {
                            // When adding attributes to a feature, we have to write
                            // pairs of numbers.  The first value is the index in the
                            // keys array (written later), and the second value is the
                            // index into the "values" array (also written later).  We're
                            // not writing the actual speed or bool value here, we're saving
                            // an index into the "values" array.  This means many features
                            // can share the same value data, leading to smaller tiles.
                            protozero::packed_field_uint32 field(feature_writer, 2);

                            field.add_element(0); // "speed" tag key offset
                            field.add_element(
                                std::min(speed, 127u)); // save the speed value, capped at 127
                            field.add_element(1);       // "is_small" tag key offset
                            field.add_element(edge.component.is_tiny ? 0 : 1); // is_small feature
                        }
                        {
                            // Encode the geometry for the feature
                            protozero::packed_field_uint32 geometry(feature_writer, 4);
                            encode_linestring(tile_line, geometry, start_x, start_y);
                        }
                    }

                    // Repeat the above for the coordinates reversed and using the `reverse`
                    // properties
                    if (edge.reverse_weight != 0 &&
                        edge.reverse_edge_based_node_id != SPECIAL_NODEID)
                    {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        line_typed geo_line;
                        geo_line.emplace_back(b.lon / COORDINATE_PRECISION,
                                              b.lat / COORDINATE_PRECISION);
                        geo_line.emplace_back(a.lon / COORDINATE_PRECISION,
                                              a.lat / COORDINATE_PRECISION);

                        uint32_t speed =
                            static_cast<uint32_t>(round(length / edge.forward_weight * 10 * 3.6));

                        line_type tile_line;
                        for (auto const &pt : geo_line)
                        {
                            double px_merc = pt.x;
                            double py_merc = pt.y;
                            lonlat2merc(px_merc, py_merc);
                            // convert to integer tile coordinat
                            const auto px = std::round(((px_merc - tile_bbox.minx) * tile_extent /
                                                        16.0 / tile_bbox.width()) *
                                                       tile_extent / 256.0);
                            const auto py = std::round(((tile_bbox.maxy - py_merc) * tile_extent /
                                                        16.0 / tile_bbox.height()) *
                                                       tile_extent / 256.0);
                            tile_line.emplace_back(px, py);
                        }

                        protozero::pbf_writer feature_writer(layer_writer, 2);
                        feature_writer.add_enum(3, 2);      // geometry type
                        feature_writer.add_uint64(1, id++); // id
                        {
                            protozero::packed_field_uint32 field(feature_writer, 2);
                            field.add_element(0); // "speed" tag key offset
                            field.add_element(
                                std::min(speed, 127u)); // save the speed value, capped at 127
                            field.add_element(1);       // "is_small" tag key offset
                            field.add_element(edge.component.is_tiny ? 0 : 1); // is_small feature
                        }
                        {
                            protozero::packed_field_uint32 geometry(feature_writer, 4);
                            encode_linestring(tile_line, geometry, start_x, start_y);
                        }
                    }
                }
            }

            // Field id 3 is the "keys" attribute
            // We need two "key" fields, these are referred to with 0 and 1 (their array indexes)
            // earlier
            layer_writer.add_string(3, "speed");
            layer_writer.add_string(3, "is_small");

            // Now, we write out the possible speed value arrays and possible is_tiny
            // values.  Field type 4 is the "values" field.  It's a variable type field,
            // so requires a two-step write (create the field, then write its value)
            for (size_t i = 0; i < 128; i++)
            {
                {
                    // Writing field type 4 == variant type
                    protozero::pbf_writer values_writer(layer_writer, 4);
                    // Attribute value 5 == uin64 type
                    values_writer.add_uint64(5, i);
                }
            }
            {
                protozero::pbf_writer values_writer(layer_writer, 4);
                // Attribute value 7 == bool type
                values_writer.add_bool(7, true);
            }
            {
                protozero::pbf_writer values_writer(layer_writer, 4);
                // Attribute value 7 == bool type
                values_writer.add_bool(7, false);
            }
        }

        // Encode the PBF result as a special Buffer object on the response.
        // This will allow downstream consumers to handle this type differently
        // to the String type.
        json_result.values["pbf"] = osrm::util::json::Buffer(buffer);

        return Status::Ok;
    }

  private:
    DataFacadeT *facade;
    std::string descriptor_string;
};
}
}
}

#endif /* TILEPLUGIN_HPP */
