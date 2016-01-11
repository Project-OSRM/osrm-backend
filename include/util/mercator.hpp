#ifndef MERCATOR_HPP
#define MERCATOR_HPP

#include <cmath>

namespace osrm
{
namespace util
{
namespace mercator
{

inline double yToLat(const double value) noexcept
{
    return 180. * M_1_PI * (2. * std::atan(std::exp(value * M_PI / 180.)) - M_PI_2);
}

inline double latToY(const double latitude) noexcept
{
    return 180. * M_1_PI * std::log(std::tan(M_PI_4 + latitude * (M_PI / 180.) / 2.));
}
}
}
}

#endif // MERCATOR_HPP
