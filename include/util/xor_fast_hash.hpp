#ifndef XOR_FAST_HASH_HPP
#define XOR_FAST_HASH_HPP

#include <boost/assert.hpp>

#include <array>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <random>
#include <vector>

namespace osrm
{
namespace util
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
class XORFastHash
{ // 65k entries
    std::array<unsigned short, (2 << 16)> table1;
    std::array<unsigned short, (2 << 16)> table2;

  public:
    XORFastHash()
    {
        std::mt19937 generator; // impl. defined but deterministic default seed

        std::iota(begin(table1), end(table1), 0u);
        std::shuffle(begin(table1), end(table1), generator);

        std::iota(begin(table2), end(table2), 0u);
        std::shuffle(begin(table2), end(table2), generator);
    }

    inline unsigned short operator()(const unsigned originalValue) const
    {
        unsigned short lsb = ((originalValue)&0xffff);
        unsigned short msb = (((originalValue) >> 16) & 0xffff);

        BOOST_ASSERT(lsb < table1.size());
        BOOST_ASSERT(msb < table2.size());

        return table1[lsb] ^ table2[msb];
    }
};
}
}

#endif // XOR_FAST_HASH_HPP
