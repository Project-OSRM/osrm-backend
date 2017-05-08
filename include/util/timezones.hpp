#ifndef OSRM_TIMEZONES_HPP
#define OSRM_TIMEZONES_HPP

#include "util/log.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <chrono>

namespace osrm
{
namespace updater
{

using point_t = boost::geometry::model::
    point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;

bool SupportsShapefiles();

class Timezoner
{
  public:
    Timezoner() = default;

    Timezoner(std::string tz_filename, std::time_t utc_time_now);

    Timezoner(std::string tz_filename);
    std::function<struct tm(const point_t &)> GetLocalTime;
};
}
}

#endif
