#ifndef XOR_FAST_HASH_HPP
#define XOR_FAST_HASH_HPP

#include <boost/assert.hpp>

#include <random>

#include <cstdint>

namespace osrm::util
{

/**
 * Tabulation Hashing, see:
 * https://opendatastructures.org/ods-cpp/5_2_Linear_Probing.html#SECTION00923000000000000000
 */

class XORFastHash
{
    // 2KB which should comfortably fit into L1 cache
    std::uint16_t tab[4][0x100];

  public:
    XORFastHash()
    {
        std::mt19937_64 generator(69); // impl. defined but deterministic default seed

        for (size_t i = 0; i < 0x100; ++i)
        {
            std::uint64_t rnd = generator();
            tab[0][i] = rnd & 0xFFFF;
            rnd >>= 16;
            tab[1][i] = rnd & 0xFFFF;
            rnd >>= 16;
            tab[2][i] = rnd & 0xFFFF;
            rnd >>= 16;
            tab[3][i] = rnd & 0xFFFF;
        }
    }

    inline std::uint16_t operator()(std::uint32_t input) const
    {
        std::uint16_t hash = tab[0][input & 0xFF];
        input >>= 8;
        hash ^= tab[1][input & 0xFF];
        input >>= 8;
        hash ^= tab[2][input & 0xFF];
        input >>= 8;
        hash ^= tab[3][input & 0xFF];
        return hash;
    }
};
} // namespace osrm::util

#endif // XOR_FAST_HASH_HPP
