#ifndef HILBERT_VALUE_HPP
#define HILBERT_VALUE_HPP

#include "osrm/coordinate.hpp"

#include <cstdint>

namespace osrm
{
namespace util
{

// computes a 64 bit value that corresponds to the hilbert space filling curve
class HilbertCode
{
  public:
    std::uint64_t operator()(const FixedPointCoordinate current_coordinate) const;
    HilbertCode() {}
    HilbertCode(const HilbertCode &) = delete;

  private:
    inline std::uint64_t BitInterleaving(const std::uint32_t a, const std::uint32_t b) const;
    inline void TransposeCoordinate(std::uint32_t *x) const;
};
}
}

#endif /* HILBERT_VALUE_HPP */
