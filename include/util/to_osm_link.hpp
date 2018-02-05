#ifndef OSRM_UTIL_TO_OSM_LINK_HPP
#define OSRM_UTIL_TO_OSM_LINK_HPP

#include "util/coordinate.hpp"

#include <string>

namespace osrm
{
namespace util
{
inline std::string toOSMLink(const util::FloatCoordinate &c)
{
    std::stringstream link;
    link << "http://www.openstreetmap.org/?mlat=" << c.lat << "&mlon=" << c.lon << "#map=19/"
         << c.lat << "/" << c.lon;
    return link.str();
}

inline std::string toOSMLink(const util::Coordinate &c)
{
    std::stringstream link;
    link << "http://www.openstreetmap.org/?mlat=" << toFloating(c.lat)
         << "&mlon=" << toFloating(c.lon) << "#map=19/" << toFloating(c.lat) << "/"
         << toFloating(c.lon);
    return link.str();
}
}
}

#endif
