#ifndef OSRM_UTIL_VECTOR_TILE_HPP
#define OSRM_UTIL_VECTOR_TILE_HPP

namespace osrm::util::vector_tile
{
// Vector tiles are 4096 virtual pixels on each side
const constexpr double EXTENT = 4096.0;
const constexpr double BUFFER = 128.0;
} // namespace osrm::util::vector_tile
#endif
