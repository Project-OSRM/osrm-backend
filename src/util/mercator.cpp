#include "util/mercator.hpp"

#include <cmath>

double mercator::y2lat(const double value) noexcept
{
    return 180. * M_1_PI * (2. * std::atan(std::exp(value * M_PI / 180.)) - M_PI_2);
}

double mercator::lat2y(const double latitude) noexcept
{
    return 180. * M_1_PI * std::log(std::tan(M_PI_4 + latitude * (M_PI / 180.) / 2.));
}
