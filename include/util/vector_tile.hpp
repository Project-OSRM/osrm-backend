#ifndef OSRM_UTIL_VECTOR_TILE_HPP
#define OSRM_UTIL_VECTOR_TILE_HPP

#include <cstdint>

namespace osrm
{
namespace util
{
namespace vector_tile
{
// Vector tiles are 4096 virtual pixels on each side
const constexpr double EXTENT = 4096.0;
const constexpr double BUFFER = 128.0;
} // namespace vector_tile
} // namespace util
} // namespace osrm
#endif
