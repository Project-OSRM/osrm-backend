#ifndef OSRM_UTIL_VECTOR_TILE_HPP
#define OSRM_UTIL_VECTOR_TILE_HPP

#include <cstdint>

namespace osrm
{
namespace util
{
namespace vector_tile
{

const constexpr std::uint32_t LAYER_TAG = 3;
const constexpr std::uint32_t NAME_TAG = 1;
const constexpr std::uint32_t VERSION_TAG = 15;
const constexpr std::uint32_t EXTEND_TAG = 5;
const constexpr std::uint32_t FEATURE_TAG = 2;
const constexpr std::uint32_t GEOMETRY_TAG = 3;
const constexpr std::uint32_t VARIANT_TAG = 4;
const constexpr std::uint32_t KEY_TAG = 3;
const constexpr std::uint32_t ID_TAG = 1;
const constexpr std::uint32_t GEOMETRY_TYPE_LINE = 2;
const constexpr std::uint32_t FEATURE_ATTRIBUTES_TAG = 2;
const constexpr std::uint32_t FEATURE_GEOMETRIES_TAG = 4;
const constexpr std::uint32_t VARIANT_TYPE_UINT32 = 5;
const constexpr std::uint32_t VARIANT_TYPE_BOOL = 7;
const constexpr std::uint32_t VARIANT_TYPE_STRING = 1;
const constexpr std::uint32_t VARIANT_TYPE_DOUBLE = 3;

// Vector tiles are 4096 virtual pixels on each side
const constexpr double EXTENT = 4096.0;
const constexpr double BUFFER = 128.0;
}
}
}
#endif
