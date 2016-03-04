#ifndef OSRM_ENGINE_BEARING_HPP
#define OSRM_ENGINE_BEARING_HPP

namespace osrm
{
namespace engine
{

struct Bearing
{
    short bearing;
    short range;

    bool IsValid() const { return bearing >= 0 && bearing <= 360 && range >= 0 && range <= 180; }
};

inline bool operator==(const Bearing lhs, const Bearing rhs)
{
    return lhs.bearing == rhs.bearing && lhs.range == rhs.range;
}
inline bool operator!=(const Bearing lhs, const Bearing rhs) { return !(lhs == rhs); }
}
}

#endif
