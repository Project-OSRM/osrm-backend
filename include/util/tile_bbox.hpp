#ifndef TILE_BBOX
#define TILE_BBOX

#include "util/rectangle.hpp"
#include <cmath>

namespace osrm
{
namespace util
{

inline RectangleInt2D TileToBBOX(int z, int x, int y)
{
    double minx = x / pow(2.0, z) * 360 - 180;
    double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
    double miny = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));

    double maxx = (x + 1) / pow(2.0, z) * 360 - 180;
    double mn = M_PI - 2.0 * M_PI * (y + 1) / pow(2.0, z);
    double maxy = 180.0 / M_PI * atan(0.5 * (exp(mn) - exp(-mn)));

    return {
            static_cast<int32_t>(std::min(minx,maxx) * COORDINATE_PRECISION),
            static_cast<int32_t>(std::max(minx,maxx) * COORDINATE_PRECISION),
            static_cast<int32_t>(std::min(miny,maxy) * COORDINATE_PRECISION),
            static_cast<int32_t>(std::min(miny,maxy) * COORDINATE_PRECISION)
    };
}
}
}

#endif
