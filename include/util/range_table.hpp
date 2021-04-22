#ifndef RANGE_TABLE_HPP
#define RANGE_TABLE_HPP

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"
#include "util/integer_range.hpp"
#include "util/vector_view.hpp"

#include <array>
#include <fstream>
#include <utility>

namespace osrm
{
namespace util
{
/*
 * These pre-declarations are needed because parsing C++ is hard
 * and otherwise the compiler gets confused.
 */

template <unsigned BLOCK_SIZE = 16, storage::Ownership Ownership = storage::Ownership::Container>
class RangeTable;

namespace serialization
{
template <unsigned BlockSize, storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const util::RangeTable<BlockSize, Ownership> &table);

template <unsigned BlockSize, storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          util::RangeTable<BlockSize, Ownership> &table);
} // namespace serialization

/**
 * Stores adjacent ranges in a compressed format.
 *
 * Maximum supported length of a range is 255.
 *
 * Note: BLOCK_SIZE is the number of differential encodoed values.
 * But each block consists of an absolute value and BLOCK_SIZE differential values.
 * So the effective block size is sizeof(unsigned) + BLOCK_SIZE.
 */
template <unsigned BLOCK_SIZE, storage::Ownership Ownership> class RangeTable
{
  public:
    using BlockT = std::array<unsigned char, BLOCK_SIZE>;
    using BlockContainerT = util::ViewOrVector<BlockT, Ownership>;
    using OffsetContainerT = util::ViewOrVector<unsigned, Ownership>;
    using RangeT = range<unsigned>;

    RangeTable() : sum_lengths(0) {}

    // for loading from shared memory
    explicit RangeTable(OffsetContainerT offsets_,
                        BlockContainerT blocks_,
                        const unsigned sum_lengths)
        : block_offsets(std::move(offsets_)), diff_blocks(std::move(blocks_)),
          sum_lengths(sum_lengths)
    {
    }

    // construct table from length vector
    template <typename VectorT> explicit RangeTable(const VectorT &lengths)
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
                block[block_idx - 1] = last_length;
                block_sum += last_length;
            }

            BOOST_ASSERT((block_idx == 0 && block_offsets[block_counter] == lengths_prefix_sum) ||
                         lengths_prefix_sum == (block_offsets[block_counter] + block_sum));

            // block is full
            if (BLOCK_SIZE == block_idx)
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
        BOOST_ASSERT(block_counter == (number_of_blocks - 1));

        // one block missing: starts with guard value
        if (0 == block_idx)
        {
            // the last value is used as sentinel
            block_offsets.push_back(lengths_prefix_sum);
            block_idx = 1;
            last_length = 0;
        }

        while (0 != block_idx)
        {
            block[block_idx - 1] = last_length;
            last_length = 0;
            block_idx = (block_idx + 1) % (BLOCK_SIZE + 1);
        }
        diff_blocks.push_back(block);

        BOOST_ASSERT(diff_blocks.size() == number_of_blocks &&
                     block_offsets.size() == number_of_blocks);

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
        const BlockT &block = diff_blocks[block_idx];
        if (internal_idx > 0)
        {
            begin_idx += PrefixSumAtIndex(internal_idx - 1, block);
        }

        // next index inside current block
        if (internal_idx < BLOCK_SIZE)
        {
            // note internal_idx - 1 is the *current* index for uint8_blocks
            end_idx = begin_idx + block[internal_idx];
        }
        else
        {
            BOOST_ASSERT(block_idx < block_offsets.size() - 1);
            end_idx = block_offsets[block_idx + 1];
        }

        BOOST_ASSERT(end_idx <= sum_lengths);
        BOOST_ASSERT(begin_idx <= end_idx);

        return irange(begin_idx, end_idx);
    }

    friend void serialization::write<BLOCK_SIZE, Ownership>(storage::tar::FileWriter &writer,
                                                            const std::string &name,
                                                            const RangeTable &table);
    friend void serialization::read<BLOCK_SIZE, Ownership>(storage::tar::FileReader &reader,
                                                           const std::string &name,
                                                           RangeTable &table);

  private:
    inline unsigned PrefixSumAtIndex(int index, const BlockT &block) const;

    // contains offset for each differential block
    OffsetContainerT block_offsets;
    // blocks of differential encoded offsets, should be aligned
    BlockContainerT diff_blocks;
    unsigned sum_lengths;
};

template <unsigned BLOCK_SIZE, storage::Ownership Ownership>
unsigned RangeTable<BLOCK_SIZE, Ownership>::PrefixSumAtIndex(int index, const BlockT &block) const
{
    // this loop looks inefficent, but a modern compiler
    // will emit nice SIMD here, at least for sensible block sizes. (I checked.)
    unsigned sum = 0;
    for (int i = 0; i <= index; ++i)
    {
        sum += block[i];
    }

    return sum;
}
} // namespace util
} // namespace osrm

#endif // RANGE_TABLE_HPP
