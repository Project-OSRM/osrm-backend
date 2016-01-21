#include "util/hilbert_value.hpp"

namespace osrm
{
namespace util
{

std::uint64_t HilbertCode::operator()(const FixedPointCoordinate current_coordinate) const
{
    unsigned location[2];
    location[0] = current_coordinate.lat + static_cast<int>(90 * COORDINATE_PRECISION);
    location[1] = current_coordinate.lon + static_cast<int>(180 * COORDINATE_PRECISION);

    TransposeCoordinate(location);
    return BitInterleaving(location[0], location[1]);
}

std::uint64_t HilbertCode::BitInterleaving(const std::uint32_t latitude,
                                           const std::uint32_t longitude) const
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

void HilbertCode::TransposeCoordinate(std::uint32_t *x) const
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
}
}
