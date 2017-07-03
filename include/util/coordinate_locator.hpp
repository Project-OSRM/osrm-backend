#ifndef OSRM_TIMEZONES_HPP
#define OSRM_TIMEZONES_HPP

#include <boost/filesystem/path.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/optional.hpp>

#include <rapidjson/document.h>
#include <string>

namespace osrm
{
namespace util
{

// Time zone shape polygons loaded in R-tree
// local_time_t is a pair of a time zone shape polygon and the corresponding local time
// rtree_t is a lookup R-tree that maps a geographic point to an index in a local_time_t vector

/**
 * Builds an in-memory RTree specialized for point-in-polygon searches.
 * Once constructed, allows you to find which polygon a point is inside.
 *
 * value_t should map to the GeoJSON type of the property you want to index.
 * (i.e. strings or numbers)
 */
class CoordinateLocator
{
  public:
    using point_t = boost::geometry::model::
        point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;

    CoordinateLocator() = default;

    // CoordinateLocator(const std::string &geojson, const std::string &property_to_index);
    CoordinateLocator(const boost::filesystem::path &geojson_filename,
                      const std::string &property_to_index);

    boost::optional<std::string> find(const point_t &point) const;

  private:
    using polygon_t = boost::geometry::model::polygon<point_t>;
    using box_t = boost::geometry::model::box<point_t>;

    using polygon_position_t = std::size_t;
    using rtree_t = boost::geometry::index::rtree<std::pair<box_t, polygon_position_t>,
                                                  boost::geometry::index::rstar<8>>;

    using object_t = std::pair<polygon_t, std::string>;

    void ConstructRTree(rapidjson::Document &geojson, const std::string &property_to_index);

    rtree_t rtree;
    std::vector<object_t> polygons;
};
}
}

#endif
