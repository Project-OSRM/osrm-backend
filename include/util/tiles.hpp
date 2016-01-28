#ifndef UTIL_TILES_HPP
#define UTIL_TILES_HPP

#include <boost/assert.hpp>

#include <cmath>
#include <tuple>

// This is a port of the tilebelt algorithm https://github.com/mapbox/tilebelt
namespace osrm
{
namespace util
{
namespace tiles
{
struct Tile
{
    unsigned x;
    unsigned y;
    unsigned z;
};

namespace detail
{
// optimized for 32bit integers
static constexpr unsigned MAX_ZOOM = 32;

// Returns 1-indexed 1..32 of MSB if value > 0 or 0 if value == 0
inline unsigned getMSBPosition(std::uint32_t value)
{
    if (value == 0)
        return 0;
    std::uint8_t pos = 1;
    while (value >>= 1)
        pos++;
    return pos;
}

inline unsigned getBBMaxZoom(const Tile top_left, const Tile bottom_left)
{
    auto x_xor = top_left.x ^ bottom_left.x;
    auto y_xor = top_left.y ^ bottom_left.y;
    auto lon_msb = detail::getMSBPosition(x_xor);
    auto lat_msb = detail::getMSBPosition(y_xor);
    return MAX_ZOOM - std::max(lon_msb, lat_msb);
}
}

inline Tile pointToTile(const double lon, const double lat)
{
    auto sin_lat = std::sin(lat * M_PI / 180.);
    auto p2z = std::pow(2, detail::MAX_ZOOM);
    unsigned x = p2z * (lon / 360. + 0.5);
    unsigned y = p2z * (0.5 - 0.25 * std::log((1 + sin_lat) / (1 - sin_lat)) / M_PI);

    return Tile{x, y, detail::MAX_ZOOM};
}

inline Tile getBBMaxZoomTile(const double min_lon,
                             const double min_lat,
                             const double max_lon,
                             const double max_lat)
{
    const auto top_left = pointToTile(min_lon, min_lat);
    const auto bottom_left = pointToTile(max_lon, max_lat);
    BOOST_ASSERT(top_left.z == detail::MAX_ZOOM);
    BOOST_ASSERT(bottom_left.z == detail::MAX_ZOOM);

    const auto max_zoom = detail::getBBMaxZoom(top_left, bottom_left);

    if (max_zoom == 0)
    {
        return Tile{0, 0, 0};
    }

    auto x = top_left.x >> (detail::MAX_ZOOM - max_zoom);
    auto y = top_left.y >> (detail::MAX_ZOOM - max_zoom);

    return Tile{x, y, max_zoom};
}
}
}
}

#endif
