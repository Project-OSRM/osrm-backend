#ifndef TILEPLUGIN_HPP
#define TILEPLUGIN_HPP

#include "engine/plugins/plugin_base.hpp"
#include "osrm/json_container.hpp"
#include "util/tile_bbox.hpp"

#include <protozero/varint.hpp>
#include <protozero/pbf_writer.hpp>

#include <string>
#include <cmath>

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
static const double EARTH_RADIUS = 6378137.0;
static const double EARTH_DIAMETER = EARTH_RADIUS * 2.0;
static const double EARTH_CIRCUMFERENCE = EARTH_DIAMETER * M_PI;
static const double MAXEXTENT = EARTH_CIRCUMFERENCE / 2.0;
static const double M_PI_by2 = M_PI / 2.0;
static const double D2R = M_PI / 180.0;
static const double R2D = 180.0 / M_PI;
static const double M_PIby360 = M_PI / 360.0;
static const double MAXEXTENTby180 = MAXEXTENT / 180.0;
static const double MAX_LATITUDE = R2D * (2.0 * std::atan(std::exp(180.0 * D2R)) - M_PI_by2);


// from mapnik-vector-tile
namespace detail_pbf {

    inline unsigned encode_length(unsigned len)
    {
        return (len << 3u) | 2u;
    }
}

inline void lonlat2merc(double & x, double & y)
{
    if (x > 180) x = 180;
    else if (x < -180) x = -180;
    if (y > MAX_LATITUDE) y = MAX_LATITUDE;
    else if (y < -MAX_LATITUDE) y = -MAX_LATITUDE;
    x = x * MAXEXTENTby180;
    y = std::log(std::tan((90 + y) * M_PIby360)) * R2D;
    y = y * MAXEXTENTby180;
}

const static double tile_size_ = 256.0;

void from_pixels(double shift, double & x, double & y)
{
    double b = shift/2.0;
    x = (x - b)/(shift/360.0);
    double g = (y - b)/-(shift/(2 * M_PI));
    y = R2D * (2.0 * std::atan(std::exp(g)) - M_PI_by2);
}

void xyz(int x,
         int y,
         int z,
         double & minx,
         double & miny,
         double & maxx,
         double & maxy)
{
    minx = x * tile_size_;
    miny = (y + 1.0) * tile_size_;
    maxx = (x + 1.0) * tile_size_;
    maxy = y * tile_size_;
    double shift = std::pow(2.0,z) * tile_size_;
    from_pixels(shift,minx,miny);
    from_pixels(shift,maxx,maxy);
    lonlat2merc(minx,miny);
    lonlat2merc(maxx,maxy);
}

void xyz2wsg84(int x,
         int y,
         int z,
         double & minx,
         double & miny,
         double & maxx,
         double & maxy)
{
    minx = x * tile_size_;
    miny = (y + 1.0) * tile_size_;
    maxx = (x + 1.0) * tile_size_;
    maxy = y * tile_size_;
    double shift = std::pow(2.0,z) * tile_size_;
    from_pixels(shift,minx,miny);
    from_pixels(shift,maxx,maxy);
}





// emulates mapbox::box2d
class bbox {
 public:
    double minx;
    double miny;
    double maxx;
    double maxy;
    bbox(double _minx,double _miny,double _maxx,double _maxy) :
      minx(_minx),
      miny(_miny),
      maxx(_maxx),
      maxy(_maxy) { }

    double width() const {
        return maxx - minx;
    }

    double height() const {
        return maxy - miny;
    }
};

// should start using core geometry class across mapnik, osrm, mapbox-gl-native
class point_type_d {
 public:
    double x;
    double y;
    point_type_d(double _x, double _y) :
      x(_x),
      y(_y) {

    }
};

class point_type_i {
 public:
    std::int64_t x;
    std::int64_t y;
    point_type_i(std::int64_t _x, std::int64_t _y) :
      x(_x),
      y(_y) {

    }
};

using line_type = std::vector<point_type_i>;
using line_typed = std::vector<point_type_d>;

// from mapnik-vector-tile
inline bool encode_linestring(line_type line,
                       protozero::packed_field_uint32 & geometry,
                       int32_t & start_x,
                       int32_t & start_y) {
    std::size_t line_size = line.size();
    //line_size -= detail_pbf::repeated_point_count(line);
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

        const double tile_extent = 4096.0;
        double min_lon, min_lat, max_lon, max_lat;

        xyz2wsg84(route_parameters.x, route_parameters.y, route_parameters.z, min_lon, min_lat, max_lon, max_lat);

        FixedPointCoordinate southwest = { static_cast<int32_t>(min_lat * COORDINATE_PRECISION), static_cast<int32_t>(min_lon * COORDINATE_PRECISION) };
        FixedPointCoordinate northeast = { static_cast<int32_t>(max_lat * COORDINATE_PRECISION), static_cast<int32_t>(max_lon * COORDINATE_PRECISION) };

        auto edges = facade->GetEdgesInBox(southwest, northeast);

        xyz(route_parameters.x, route_parameters.y, route_parameters.z, min_lon, min_lat, max_lon, max_lat);

        bbox tile_bbox(min_lon, min_lat, max_lon, max_lat);

        std::string buffer;
        protozero::pbf_writer tile_writer(buffer);
        {
            protozero::pbf_writer layer_writer(tile_writer,3);
            // TODO: don't write a layer if there are no features
            layer_writer.add_uint32(15,2); // version
            layer_writer.add_string(1,"speeds"); // name
            layer_writer.add_uint32(5,4096); // extent

            std::vector<double> speeds;
            std::vector<bool> is_smalls;
            {
                unsigned id = 1;
                for (const auto & edge : edges)
                {
                    const auto a = facade->GetCoordinateOfNode(edge.u);
                    const auto b = facade->GetCoordinateOfNode(edge.v);
                    double length = osrm::util::coordinate_calculation::haversineDistance( a.lon, a.lat, b.lon, b.lat );
                    if (edge.forward_weight != 0 && edge.forward_edge_based_node_id != SPECIAL_NODEID) {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        line_typed geo_line;
                        geo_line.emplace_back(a.lon / COORDINATE_PRECISION, a.lat / COORDINATE_PRECISION);
                        geo_line.emplace_back(b.lon / COORDINATE_PRECISION, b.lat / COORDINATE_PRECISION);

                        double speed = round(length / edge.forward_weight * 10 )  * 3.6;

                        speeds.push_back(speed);
                        is_smalls.push_back(edge.component.is_tiny);

                        line_type tile_line;
                        for (auto const & pt : geo_line) {
                            double px_merc = pt.x;
                            double py_merc = pt.y;
                            lonlat2merc(px_merc,py_merc);
                            // convert to integer tile coordinat
                            const auto px = std::round(((px_merc - tile_bbox.minx) * tile_extent/16.0 / static_cast<double>(tile_bbox.width()))*tile_extent/256.0);
                            const auto py = std::round(((tile_bbox.maxy - py_merc) * tile_extent/16.0 / static_cast<double>(tile_bbox.height()))*tile_extent/256.0);
                            tile_line.emplace_back(px,py);
                        }

                        protozero::pbf_writer feature_writer(layer_writer,2);
                        feature_writer.add_enum(3,2); // geometry type
                        feature_writer.add_uint64(1,id++); // id
                        {
                            protozero::packed_field_uint32 field(feature_writer, 2);
                            field.add_element(0); // "speed" tag key offset
                            field.add_element((speeds.size()-1)*2); // "speed" tag value offset
                            field.add_element(1); // "is_small" tag key offset
                            field.add_element((is_smalls.size()-1)*2+1); // "is_small" tag value offset
                        }
                        {
                            protozero::packed_field_uint32 geometry(feature_writer,4);
                            encode_linestring(tile_line,geometry,start_x,start_y);
                        }
                    }
                    if (edge.reverse_weight != 0 && edge.reverse_edge_based_node_id != SPECIAL_NODEID) {
                        std::int32_t start_x = 0;
                        std::int32_t start_y = 0;

                        line_typed geo_line;
                        geo_line.emplace_back(b.lon / COORDINATE_PRECISION, b.lat / COORDINATE_PRECISION);
                        geo_line.emplace_back(a.lon / COORDINATE_PRECISION, a.lat / COORDINATE_PRECISION);

                        double speed = round(length / edge.reverse_weight * 10 ) * 3.6;

                        speeds.push_back(speed);
                        is_smalls.push_back(edge.component.is_tiny);

                        line_type tile_line;
                        for (auto const & pt : geo_line) {
                            double px_merc = pt.x;
                            double py_merc = pt.y;
                            lonlat2merc(px_merc,py_merc);
                            // convert to integer tile coordinat
                            const auto px = std::round(((px_merc - tile_bbox.minx) * tile_extent/16.0 / static_cast<double>(tile_bbox.width()))*tile_extent/256.0);
                            const auto py = std::round(((tile_bbox.maxy - py_merc) * tile_extent/16.0 / static_cast<double>(tile_bbox.height()))*tile_extent/256.0);
                            tile_line.emplace_back(px,py);
                        }

                        protozero::pbf_writer feature_writer(layer_writer,2);
                        feature_writer.add_enum(3,2); // geometry type
                        feature_writer.add_uint64(1,id++); // id
                        {
                            protozero::packed_field_uint32 field(feature_writer, 2);
                            field.add_element(0); // "speed" tag key offset
                            field.add_element((speeds.size()-1)*2); // "speed" tag value offset
                            field.add_element(1); // "is_small" tag key offset
                            field.add_element((is_smalls.size()-1)*2+1); // "is_small" tag value offset
                        }
                        {
                            protozero::packed_field_uint32 geometry(feature_writer,4);
                            encode_linestring(tile_line,geometry,start_x,start_y);
                        }
                    }

                }
            }
            layer_writer.add_string(3,"speed");
            layer_writer.add_string(3,"is_small");

            for (size_t i=0; i<speeds.size(); i++) {
                {
                    protozero::pbf_writer values_writer(layer_writer,4);
                    values_writer.add_double(3, speeds[i]);
                }
                {
                    protozero::pbf_writer values_writer(layer_writer,4);
                    values_writer.add_bool(7, is_smalls[i]);
                }
            }
        }

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
