#ifndef __RANGE_TABLE_H__
#define __RANGE_TABLE_H__

#include <boost/range/irange.hpp>

#include <fstream>
#include <vector>

#if defined(__GNUC__) && defined(__SSE2__)
#define OSRM_USE_SSE
#include <xmmintrin.h>
#endif

#include "SharedMemoryFactory.h"
#include "SharedMemoryVectorWrapper.h"

/*
 * These pre-declarations are needed because parsing C++ is hard
 * and otherwise the compiler gets confused.
 */

template<unsigned BLOCK_SIZE=16, bool USE_SHARED_MEMORY = false> class RangeTable;

template<unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::ostream& operator<<(std::ostream &out, const RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table);

template<unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::istream& operator>>(std::istream &in, RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table);

/**
 * Stores adjacent ranges in a compressed format.
 *
 * Maximum supported length of a range is 255.
 *
 * Note: BLOCK_SIZE is the number of differential encodoed values.
 * But each block consists of an absolute value and BLOCK_SIZE differential values.
 * So the effective block size is sizeof(unsigned) + BLOCK_SIZE.
 */
template<unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
class RangeTable
{
public:
    union BlockT
    {
        unsigned char uint8_blocks[BLOCK_SIZE];
#ifdef OSRM_USE_SSE
        static_assert(BLOCK_SIZE % 16 == 0,
        "If SSE instructions are enabled, only multiples of 16 are supported as BLOCK_SIZE");
        __m128i uint128_blocks[BLOCK_SIZE/16];
#endif
    };

    typedef typename ShM<BlockT, USE_SHARED_MEMORY>::vector   BlockContainerT;
    typedef typename ShM<unsigned, USE_SHARED_MEMORY>::vector OffsetContainerT;
    typedef decltype(boost::irange(0u,0u))                    RangeT;

    friend std::ostream& operator<< <>(std::ostream &out, const RangeTable &table);
    friend std::istream& operator>> <>(std::istream &in, RangeTable &table);

    RangeTable() {}

    // for loading from shared memory
    explicit RangeTable(OffsetContainerT& external_offsets, BlockContainerT& external_blocks, const unsigned sum_lengths)
    : sum_lengths(sum_lengths)
    {
        block_offsets.swap(external_offsets);
        diff_blocks.swap(external_blocks);
    }

    // construct table from length vector
    explicit RangeTable(const std::vector<unsigned>& lengths)
    {
        const unsigned number_of_blocks = [&lengths]() {
            unsigned num = (lengths.size() + 1) / (BLOCK_SIZE + 1);
            if ((lengths.size() + 1) % (BLOCK_SIZE + 1) != 0)
            {
                num += 1;
            }
            return num;
        }();

        block_offsets.reserve(number_of_blocks);
        diff_blocks.reserve(number_of_blocks);

        unsigned last_length = 0;
        unsigned lengths_prefix_sum = 0;
        unsigned block_idx = 0;
        unsigned block_counter = 0;
        BlockT block;
        unsigned block_sum = 0;
        for (const unsigned l : lengths)
        {
            // first entry of a block: encode absolute offset
            if (block_idx == 0)
            {
                block_offsets.push_back(lengths_prefix_sum);
                block_sum = 0;
            }
            else
            {
                block.uint8_blocks[block_idx - 1] = last_length;
                block_sum += last_length;
            }

            BOOST_ASSERT((block_idx == 0 && block_offsets[block_counter] == lengths_prefix_sum)
                || lengths_prefix_sum == (block_offsets[block_counter]+block_sum));

            // block is full
            if (block_idx == BLOCK_SIZE)
            {
                diff_blocks.push_back(block);
                block_counter++;
            }

            // we can only store strings with length 255
            BOOST_ASSERT(l <= 255);

            lengths_prefix_sum += l;
            last_length = l;

            block_idx = (block_idx + 1) % (BLOCK_SIZE + 1);
        }

        // Last block can't be finished because we didn't add the sentinel
        BOOST_ASSERT (block_counter == (number_of_blocks - 1));

        // one block missing: starts with guard value
        if (block_idx == 0)
        {
            // the last value is used as sentinel
            block_offsets.push_back(lengths_prefix_sum);
            block_idx = (block_idx + 1) % BLOCK_SIZE;
        }

        while (block_idx != 0)
        {
            block.uint8_blocks[block_idx - 1] = last_length;
            last_length = 0;
            block_idx = (block_idx + 1) % (BLOCK_SIZE + 1);
        }
        diff_blocks.push_back(block);

        BOOST_ASSERT(diff_blocks.size() == number_of_blocks && block_offsets.size() == number_of_blocks);

        sum_lengths = lengths_prefix_sum;
    }

    inline RangeT GetRange(const unsigned id) const
    {
        BOOST_ASSERT(id < block_offsets.size() + diff_blocks.size() * BLOCK_SIZE);
        // internal_idx 0 is implicitly stored in block_offsets[block_idx]
        const unsigned internal_idx = id % (BLOCK_SIZE + 1);
        const unsigned block_idx = id / (BLOCK_SIZE + 1);

        BOOST_ASSERT(block_idx < diff_blocks.size());

        unsigned begin_idx = 0;
        unsigned end_idx = 0;
        begin_idx = block_offsets[block_idx];
        const BlockT& block = diff_blocks[block_idx];
        if (internal_idx > 0)
        {
            begin_idx += PrefixSumAtIndex(internal_idx - 1, block);
        }

        // next index inside current block
        if (internal_idx < BLOCK_SIZE)
        {
            // note internal_idx - 1 is the *current* index for uint8_blocks
            end_idx = begin_idx + block.uint8_blocks[internal_idx];
        }
        else
        {
            BOOST_ASSERT(block_idx < block_offsets.size() - 1);
            end_idx = block_offsets[block_idx + 1];
        }

        BOOST_ASSERT(begin_idx < sum_lengths && end_idx <= sum_lengths);
        BOOST_ASSERT(begin_idx <= end_idx);

        return boost::irange(begin_idx, end_idx);
    }
private:

    inline unsigned PrefixSumAtIndex(int index, const BlockT& block) const;

    // contains offset for each differential block
    OffsetContainerT block_offsets;
    // blocks of differential encoded offsets, should be aligned
    BlockContainerT diff_blocks;
    unsigned sum_lengths;
};

#ifdef OSRM_USE_SSE
// For blocksize 16 we can use SSE instructions
// FIXME only implemented for non-shared memory
template<>
unsigned RangeTable<16>::PrefixSumAtIndex(int index, const BlockT& block) const
{
    union OffsetT
    {
        unsigned short  u16[8];
        __m128i         u128;
    };
    OffsetT offsets;

    // converts lower 8 bytes to 8 shorts
    offsets.u128 = _mm_unpacklo_epi8(block.uint128_blocks[0], _mm_set1_epi8(0));
    offsets.u128 = _mm_add_epi16(offsets.u128, _mm_slli_si128(offsets.u128, 2));
    if (index < 2)
        return offsets.u16[index];
    offsets.u128 = _mm_add_epi16(offsets.u128, _mm_slli_si128(offsets.u128, 4));
    if (index < 4)
        return offsets.u16[index];
    offsets.u128 = _mm_add_epi16(offsets.u128, _mm_slli_si128(offsets.u128, 8));

    if (index < 8)
        return offsets.u16[index];
    unsigned temp = offsets.u16[7];
    index -= 8;

    // converts upper 8 bytes to 8 shorts
    offsets.u128 = _mm_unpackhi_epi8(block.uint128_blocks[0], _mm_set1_epi8(0));
    offsets.u128 = _mm_add_epi16(offsets.u128, _mm_slli_si128(offsets.u128, 2));
    if (index < 2)
        return (temp + offsets.u16[index]);
    offsets.u128 = _mm_add_epi16(offsets.u128, _mm_slli_si128(offsets.u128, 4));
    if (index < 4)
        return (temp + offsets.u16[index]);
    offsets.u128 = _mm_add_epi16(offsets.u128, _mm_slli_si128(offsets.u128, 8));

    return (temp + offsets.u16[index]);
}
#endif

template<unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
unsigned RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY>::PrefixSumAtIndex(int index, const BlockT& block) const
{
    unsigned sum = 0;
    for (int i = 0; i <= index; i++)
        sum += block.uint8_blocks[i];

    return sum;
}

template<unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::ostream& operator<<(std::ostream &out, const RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table)
{
    // write number of block
    const unsigned number_of_blocks = table.diff_blocks.size();
    out.write((char *) &number_of_blocks, sizeof(unsigned));
    // write total length
    out.write((char *) &table.sum_lengths, sizeof(unsigned));
    // write block offsets
    out.write((char *) table.block_offsets.data(), sizeof(unsigned) * table.block_offsets.size());
    // write blocks
    out.write((char *) table.diff_blocks.data(), BLOCK_SIZE * table.diff_blocks.size());

    return out;
}

template<unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::istream& operator>>(std::istream &in, RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table)
{
    // read number of block
    unsigned number_of_blocks;
    in.read((char *) &number_of_blocks, sizeof(unsigned));
    // read total length
    in.read((char *) &table.sum_lengths, sizeof(unsigned));

    table.block_offsets.resize(number_of_blocks);
    table.diff_blocks.resize(number_of_blocks);

    // read block offsets
    in.read((char *) table.block_offsets.data(), sizeof(unsigned) * number_of_blocks);
    // read blocks
    in.read((char *) table.diff_blocks.data(), BLOCK_SIZE * number_of_blocks);
    return in;
}

#endif
