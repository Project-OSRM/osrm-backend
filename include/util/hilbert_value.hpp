#ifndef HILBERT_VALUE_HPP
#define HILBERT_VALUE_HPP

#include "osrm/coordinate.hpp"

#include <cstdint>

namespace osrm
{
namespace util
{

// Computes a 64 bit value that corresponds to the hilbert space filling curve
std::uint64_t hilbertCode(const Coordinate &coordinate);
std::uint64_t hilbertCode(const FixedLongitude &lon, const FixedLatitude &lat);
}
}

#endif /* HILBERT_VALUE_HPP */
