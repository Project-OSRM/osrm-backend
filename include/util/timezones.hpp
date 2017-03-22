#ifndef TIMEZONES_HPP
#define TIMEZONES_HPP

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

// Time zone shape polygons loaded in R-tree
// local_time_t is a pair of a time zone shape polygon and the corresponding local time
// rtree_t is a lookup R-tree that maps a geographic point to an index in a local_time_t vector
using point_t = boost::geometry::model::
    point<int32_t, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;
using polygon_t = boost::geometry::model::polygon<point_t>;
using box_t = boost::geometry::model::box<point_t>;
using rtree_t =
    boost::geometry::index::rtree<std::pair<box_t, size_t>, boost::geometry::index::rstar<8>>;
using local_time_t = std::pair<polygon_t, struct tm>;

std::function<struct tm(const point_t &)> LoadLocalTimesRTree(const std::string &tz_shapes_filename,
                                                              std::time_t utc_time);

#endif
