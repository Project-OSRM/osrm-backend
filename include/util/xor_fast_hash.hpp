#ifndef XOR_FAST_HASH_HPP
#define XOR_FAST_HASH_HPP

#include <boost/assert.hpp>

#include <algorithm>
#include <array>
#include <iterator>
#include <numeric>
#include <random>
#include <vector>

#include <cstdint>

namespace osrm::util
{

/*
    This is an implementation of Tabulation hashing, which has suprising properties like
   universality.
    The space requirement is 2*2^16 = 256 kb of memory, which fits into L2 cache.
    Evaluation boils down to 10 or less assembly instruction on any recent X86 CPU:

    1: movq    table2(%rip), %rdx
    2: movl    %edi, %eax
    3: movzwl  %di, %edi
    4: shrl    $16, %eax
    5: movzwl  %ax, %eax
    6: movzbl  (%rdx,%rax), %eax
    7: movq    table1(%rip), %rdx
    8: xorb    (%rdx,%rdi), %al
    9: movzbl  %al, %eax
    10: ret

*/
template <std::size_t MaxNumElements = (1u << 16u)> class XORFastHash
{
    static_assert(MaxNumElements <= (1u << 16u), "only 65536 elements indexable with uint16_t");

    std::array<std::uint16_t, MaxNumElements> table1;
    std::array<std::uint16_t, MaxNumElements> table2;

  public:
    XORFastHash()
    {
        std::mt19937 generator; // impl. defined but deterministic default seed

        std::iota(begin(table1), end(table1), 0u);
        std::shuffle(begin(table1), end(table1), generator);

        std::iota(begin(table2), end(table2), 0u);
        std::shuffle(begin(table2), end(table2), generator);
    }

    inline std::uint16_t operator()(const std::uint32_t originalValue) const
    {
        std::uint16_t lsb = originalValue & 0xffffu;
        std::uint16_t msb = originalValue >> 16u;

        BOOST_ASSERT(lsb < table1.size());
        BOOST_ASSERT(msb < table2.size());

        return table1[lsb] ^ table2[msb];
    }
};
} // namespace osrm::util

#endif // XOR_FAST_HASH_HPP
