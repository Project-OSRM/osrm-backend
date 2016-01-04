#include "util/hilbert_value.hpp"

#include "osrm/coordinate.hpp"

uint64_t HilbertCode::operator()(const FixedPointCoordinate &current_coordinate) const
{
    unsigned location[2];
    location[0] = current_coordinate.lat + static_cast<int>(90 * COORDINATE_PRECISION);
    location[1] = current_coordinate.lon + static_cast<int>(180 * COORDINATE_PRECISION);

    TransposeCoordinate(location);
    return BitInterleaving(location[0], location[1]);
}

uint64_t HilbertCode::BitInterleaving(const uint32_t latitude, const uint32_t longitude) const
{
    uint64_t result = 0;
    for (int8_t index = 31; index >= 0; --index)
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

void HilbertCode::TransposeCoordinate(uint32_t *X) const
{
    uint32_t M = 1u << (32 - 1), P, Q, t;
    int i;
    // Inverse undo
    for (Q = M; Q > 1; Q >>= 1)
    {
        P = Q - 1;
        for (i = 0; i < 2; ++i)
        {

            const bool condition = (X[i] & Q);
            if (condition)
            {
                X[0] ^= P; // invert
            }
            else
            {
                t = (X[0] ^ X[i]) & P;
                X[0] ^= t;
                X[i] ^= t;
            }
        } // exchange
    }
    // Gray encode
    for (i = 1; i < 2; ++i)
    {
        X[i] ^= X[i - 1];
    }
    t = 0;
    for (Q = M; Q > 1; Q >>= 1)
    {
        const bool condition = (X[2 - 1] & Q);
        if (condition)
        {
            t ^= Q - 1;
        }
    } // check if this for loop is wrong
    for (i = 0; i < 2; ++i)
    {
        X[i] ^= t;
    }
}
