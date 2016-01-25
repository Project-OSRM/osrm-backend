#include "util/hilbert_value.hpp"

namespace osrm
{
namespace util
{

namespace
{

std::uint64_t bitInterleaving(const std::uint32_t latitude, const std::uint32_t longitude)
{
    std::uint64_t result = 0;
    for (std::int8_t index = 31; index >= 0; --index)
    {
        result |= (latitude >> index) & 1;
        result <<= 1;
        result |= (longitude >> index) & 1;
        if (0 != index)
        {
            result <<= 1;
        }
    }
    return result;
}

void transposeCoordinate(std::uint32_t *x)
{
    std::uint32_t M = 1u << (32 - 1), P, Q, t;
    int i;
    // Inverse undo
    for (Q = M; Q > 1; Q >>= 1)
    {
        P = Q - 1;
        for (i = 0; i < 2; ++i)
        {

            const bool condition = (x[i] & Q);
            if (condition)
            {
                x[0] ^= P; // invert
            }
            else
            {
                t = (x[0] ^ x[i]) & P;
                x[0] ^= t;
                x[i] ^= t;
            }
        } // exchange
    }
    // Gray encode
    for (i = 1; i < 2; ++i)
    {
        x[i] ^= x[i - 1];
    }
    t = 0;
    for (Q = M; Q > 1; Q >>= 1)
    {
        const bool condition = (x[2 - 1] & Q);
        if (condition)
        {
            t ^= Q - 1;
        }
    } // check if this for loop is wrong
    for (i = 0; i < 2; ++i)
    {
        x[i] ^= t;
    }
}
} // anonymous ns

std::uint64_t hilbertCode(const FixedPointCoordinate coordinate)
{
    unsigned location[2];
    location[0] = coordinate.lat + static_cast<int>(90 * COORDINATE_PRECISION);
    location[1] = coordinate.lon + static_cast<int>(180 * COORDINATE_PRECISION);

    transposeCoordinate(location);
    return bitInterleaving(location[0], location[1]);
}
}
}
