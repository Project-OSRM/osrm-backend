#ifndef HILBERT_VALUE_HPP
#define HILBERT_VALUE_HPP

#include <cstdint>

namespace osrm
{
namespace util
{

// computes a 64 bit value that corresponds to the hilbert space filling curve

struct FixedPointCoordinate;

class HilbertCode
{
  public:
    uint64_t operator()(const FixedPointCoordinate &current_coordinate) const;
    HilbertCode() {}
    HilbertCode(const HilbertCode &) = delete;

  private:
    inline uint64_t BitInterleaving(const uint32_t a, const uint32_t b) const;
    inline void TransposeCoordinate(uint32_t *X) const;
};
}
}

#endif /* HILBERT_VALUE_HPP */
