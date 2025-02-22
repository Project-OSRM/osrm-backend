#ifndef OSRM_TIMEZONES_HPP
#define OSRM_TIMEZONES_HPP

#include "util/log.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <rapidjson/document.h>

#include <chrono>
#include <filesystem>
#include <optional>

namespace osrm::updater
{

// Time zone shape polygons loaded in R-tree
// local_time_t is a pair of a time zone shape polygon and the corresponding local time
// rtree_t is a lookup R-tree that maps a geographic point to an index in a local_time_t vector
using point_t = boost::geometry::model::
    point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;
using polygon_t = boost::geometry::model::polygon<point_t>;
using box_t = boost::geometry::model::box<point_t>;
using rtree_t =
    boost::geometry::index::rtree<std::pair<box_t, size_t>, boost::geometry::index::rstar<8>>;
using local_time_t = std::pair<polygon_t, struct tm>;

class Timezoner
{
  public:
    Timezoner() = default;

    Timezoner(const char geojson[], std::time_t utc_time_now);
    Timezoner(const std::filesystem::path &tz_shapes_filename, std::time_t utc_time_now);

    std::optional<struct tm> operator()(const point_t &point) const;

  private:
    void LoadLocalTimesRTree(rapidjson::Document &geojson, std::time_t utc_time);

    rtree_t rtree;
    std::vector<local_time_t> local_times;
};
} // namespace osrm::updater

#endif
