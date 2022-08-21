#ifndef OSRM_UTIL_TO_OSM_LINK_HPP
#define OSRM_UTIL_TO_OSM_LINK_HPP

#include "util/coordinate.hpp"

#include <iomanip>
#include <string>

namespace osrm
{
namespace util
{
inline std::string toOSMLink(const util::FloatCoordinate &c)
{
    std::stringstream link;
    link << "http://www.openstreetmap.org/?zoom=18&mlat=" << std::setprecision(10) << c.lat
         << "&mlon=" << c.lon;
    return link.str();
}

inline std::string toOSMLink(const util::Coordinate &c)
{
    std::stringstream link;
    link << "http://www.openstreetmap.org/?zoom=18&mlat=" << std::setprecision(10)
         << toFloating(c.lat) << "&mlon=" << toFloating(c.lon);
    return link.str();
}
} // namespace util
} // namespace osrm

#endif
