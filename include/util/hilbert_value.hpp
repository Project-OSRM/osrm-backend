#ifndef HILBERT_VALUE_HPP
#define HILBERT_VALUE_HPP

#include "osrm/coordinate.hpp"

#include <climits>
#include <cstdint>

namespace osrm
{
namespace util
{

// Transform x and y to Hilbert SFC linear coordinate
// using N most significant bits of x and y.
//   References:
// [1] Arndt, Jörg. Matters Computational Ideas, Algorithms, Source Code, 2010. 1.31.1 The Hilbert
//     curve p. 86
// [2] FSM implementation from http://www.hackersdelight.org/hdcodetxt/hilbert/hil_s_from_xy.c.txt
//    The method is to employ the following state transition table:
// ----------------------------------------------------------
// If the current   And the next bits   then append  and enter
//   state is        of x and y are      to result     state
// ----------------------------------------------------------
//      A                 (0, 0)            00           B
//      A                 (0, 1)            01           A
//      A                 (1, 0)            11           D
//      A                 (1, 1)            10           A
//      B                 (0, 0)            00           A
//      B                 (0, 1)            11           C
//      B                 (1, 0)            01           B
//      B                 (1, 1)            10           B
//      C                 (0, 0)            10           D
//      C                 (0, 1)            11           B
//      C                 (1, 0)            01           C
//      C                 (1, 1)            00           D
//      D                 (0, 0)            10           C
//      D                 (0, 1)            01           D
//      D                 (1, 0)            11           A
//      D                 (1, 1)            00           C
template <int N = 32, typename T = std::uint32_t, typename R = std::uint64_t>
inline R HilbertToLinear(T x, T y)
{
    static_assert(N <= sizeof(T) * CHAR_BIT, "input type is smaller than N");
    static_assert(2 * N <= sizeof(R) * CHAR_BIT, "output type is smaller than 2N");

    R result = 0;
    unsigned state = 0;
    for (int i = 0; i < N; ++i)
    {
        const unsigned xi = (x >> (sizeof(T) * CHAR_BIT - 1)) & 1;
        const unsigned yi = (y >> (sizeof(T) * CHAR_BIT - 1)) & 1;
        x <<= 1;
        y <<= 1;

        const unsigned row = 4 * state | 2 * xi | yi;
        result = (result << 2) | ((0x361E9CB4 >> 2 * row) & 3);
        state = (0x8FE65831 >> 2 * row) & 3;
    }
    return result;
}

// Computes a 64 bit value that corresponds to the hilbert space filling curve
inline std::uint64_t GetHilbertCode(const Coordinate &coordinate)
{
    const std::uint32_t x = static_cast<std::int32_t>(coordinate.lon) +
                            static_cast<std::int32_t>(180 * COORDINATE_PRECISION);
    const std::uint32_t y = static_cast<std::int32_t>(coordinate.lat) +
                            static_cast<std::int32_t>(90 * COORDINATE_PRECISION);
    return HilbertToLinear(x, y);
}
} // namespace util
} // namespace osrm

#endif /* HILBERT_VALUE_HPP */
