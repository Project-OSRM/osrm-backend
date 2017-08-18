#ifndef OSRM_LOCATION_DEPENDENT_DATA_HPP
#define OSRM_LOCATION_DEPENDENT_DATA_HPP

#include <boost/filesystem/path.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <osmium/osm/way.hpp>

#include <sol2/sol.hpp>

#include <string>
#include <unordered_map>

namespace osrm
{
namespace extractor
{
struct LocationDependentData
{
    using point_t = boost::geometry::model::
        point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;
    using polygon_t = boost::geometry::model::polygon<point_t>;
    using box_t = boost::geometry::model::box<point_t>;

    using polygon_position_t = std::size_t;
    using rtree_t = boost::geometry::index::rtree<std::pair<box_t, polygon_position_t>,
                                                  boost::geometry::index::rstar<8>>;

    using property_t = boost::variant<boost::blank, double, std::string, bool>;
    using properties_t = std::unordered_map<std::string, property_t>;

    LocationDependentData(const std::vector<boost::filesystem::path> &file_paths);

    sol::table operator()(sol::state &state, const osmium::Way &way) const;

  private:
    void loadLocationDependentData(const boost::filesystem::path &file_path);

    rtree_t rtree;
    std::vector<std::pair<polygon_t, std::size_t>> polygons;
    std::vector<properties_t> properties;
};
}
}

#endif
