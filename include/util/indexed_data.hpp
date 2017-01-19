#ifndef OSRM_INDEXED_DATA_HPP
#define OSRM_INDEXED_DATA_HPP

#include "util/exception.hpp"
#include "util/string_view.hpp"

#include <boost/assert.hpp>

#include <array>
#include <iterator>
#include <limits>
#include <ostream>
#include <string>
#include <type_traits>

namespace osrm
{
namespace util
{

template <int N, typename T = std::string> struct VariableGroupBlock
{
    static constexpr std::uint32_t BLOCK_SIZE = N;

    using ResultType = T;
    using ValueType = typename T::value_type;

    static_assert(0 <= BLOCK_SIZE && BLOCK_SIZE <= 16, "incorrect block size");
    static_assert(sizeof(ValueType) == 1, "data basic type must char");

    struct BlockReference
    {
        std::uint32_t offset;
        std::uint32_t descriptor;
    };

    VariableGroupBlock() {}

    /// Returns ceiling(log_256(value + 1))
    inline std::uint32_t log256(std::uint32_t value) const
    {
        BOOST_ASSERT(value < 0x1000000);
        return value == 0 ? 0 : value < 0x100 ? 1 : value < 0x10000 ? 2 : 3;
    }

    /// Advance data iterator by the value of byte_length bytes at length iterator.
    /// Advance length iterator by byte_length.
    template <typename DataIterator>
    inline void
    var_advance(DataIterator &data, DataIterator &length, std::uint32_t byte_length) const
    {
        if (byte_length == 0)
        {
        }
        else if (byte_length == 1)
        {
            data += static_cast<unsigned char>(*length++);
        }
        else if (byte_length == 2)
        {
            data += static_cast<unsigned char>(*length++);
            data += static_cast<unsigned char>(*length++) << 8;
        }
        else
        {
            BOOST_ASSERT(byte_length == 3);
            data += static_cast<unsigned char>(*length++);
            data += static_cast<unsigned char>(*length++) << 8;
            data += static_cast<unsigned char>(*length++) << 16;
        }
    }

    /// Summation of 16 2-bit values using SWAR
    inline std::uint32_t sum2bits(std::uint32_t value) const
    {
        value = (value >> 2 & 0x33333333) + (value & 0x33333333);
        value = (value >> 4 & 0x0f0f0f0f) + (value & 0x0f0f0f0f);
        value = (value >> 8 & 0x00ff00ff) + (value & 0x00ff00ff);
        return (value >> 16 & 0x0000ffff) + (value & 0x0000ffff);
    }

    /// Write a block reference {offset, descriptor}, where offset
    /// is a global block offset and descriptor is a 32-bit value
    /// of prefix length. sum(descriptor) equals to the block
    /// prefix length.
    /// Returns the block prefix length.
    template <typename Offset, typename OffsetIterator>
    Offset WriteBlockReference(std::ostream &out,
                               Offset data_offset,
                               OffsetIterator first,
                               OffsetIterator last) const
    {
        BOOST_ASSERT(data_offset <= std::numeric_limits<decltype(BlockReference::offset)>::max());

        Offset prefix_length = 0;
        BlockReference refernce{static_cast<decltype(BlockReference::offset)>(data_offset), 0};
        for (; first != last; --last)
        {
            const std::uint32_t data_length = *last - *std::prev(last);
            if (data_length >= 0x1000000)
                throw util::exception(boost::format("too large data length %1%") % data_length);

            const std::uint32_t byte_length = log256(data_length);
            refernce.descriptor = (refernce.descriptor << 2) | byte_length;
            prefix_length += byte_length;
        }

        out.write((const char *)&refernce, sizeof(refernce));

        return prefix_length;
    }

    /// Write a block prefix that is an array of variable encoded data lengths:
    ///   0 is omitted;
    ///   1..255 is 1 byte;
    ///   256..65535 is 2 bytes;
    ///   65536..16777215 is 3 bytes.
    /// [first..last] is an inclusive range of block data.
    /// The length of the last item in the block is not stored.
    template <typename OffsetIterator>
    void WriteBlockPrefix(std::ostream &out, OffsetIterator first, OffsetIterator last) const
    {
        for (OffsetIterator curr = first, next = std::next(first); curr != last; ++curr, ++next)
        {
            const std::uint32_t data_length = *next - *curr;
            const std::uint32_t byte_length = log256(data_length);
            if (byte_length == 0)
                continue;

            out.write((const char *)&data_length, byte_length);
        }
    }

    /// Advances the range to an item stored in the referenced block.
    /// Input [first..last) is a range of the complete block data with prefix.
    /// Output [first..last) is a range of the referenced data at local_index.
    template <typename DataIterator>
    void ReadRefrencedBlock(const BlockReference &reference,
                            std::uint32_t local_index,
                            DataIterator &first,
                            DataIterator &last) const
    {
        std::uint32_t descriptor = reference.descriptor;
        DataIterator var_lengths = first;          // iterator to the variable lengths part
        std::advance(first, sum2bits(descriptor)); // advance first to the block data part
        for (std::uint32_t i = 0; i < local_index; ++i, descriptor >>= 2)
        {
            var_advance(first, var_lengths, descriptor & 0x3);
        }

        if (local_index < BLOCK_SIZE)
        {
            last = first;
            var_advance(last, var_lengths, descriptor & 0x3);
        }
    }
};

template <int N, typename T = std::string> struct FixedGroupBlock
{
    static constexpr std::uint32_t BLOCK_SIZE = N;

    using ResultType = T;
    using ValueType = typename T::value_type;

    static_assert(sizeof(ValueType) == 1, "data basic type must char");

    struct BlockReference
    {
        std::uint32_t offset;
    };

    FixedGroupBlock() {}

    /// Write a block reference {offset}, where offset is a global block offset
    /// Returns the fixed block prefix length.
    template <typename Offset, typename OffsetIterator>
    Offset
    WriteBlockReference(std::ostream &out, Offset data_offset, OffsetIterator, OffsetIterator) const
    {
        BOOST_ASSERT(data_offset <= std::numeric_limits<decltype(BlockReference::offset)>::max());

        BlockReference refernce{static_cast<decltype(BlockReference::offset)>(data_offset)};
        out.write((const char *)&refernce, sizeof(refernce));

        return BLOCK_SIZE;
    }

    /// Write a fixed length block prefix.
    template <typename OffsetIterator>
    void WriteBlockPrefix(std::ostream &out, OffsetIterator first, OffsetIterator last) const
    {
        std::uint32_t index = 0;
        std::array<ValueType, BLOCK_SIZE> block_prefix;
        for (OffsetIterator curr = first, next = std::next(first); curr != last; ++curr, ++next)
        {
            const std::uint32_t data_length = *next - *curr;
            if (data_length >= 0x100)
                throw util::exception(boost::format("too large data length %1%") % data_length);

            block_prefix[index++] = static_cast<ValueType>(data_length);
        }
        out.write((const char *)block_prefix.data(), block_prefix.size());
    }

    /// Advances the range to an item stored in the referenced block.
    /// Input [first..last) is a range of the complete block data with prefix.
    /// Output [first..last) is a range of the referenced data at local_index.
    template <typename DataIterator>
    void ReadRefrencedBlock(const BlockReference &,
                            std::uint32_t local_index,
                            DataIterator &first,
                            DataIterator &last) const
    {
        DataIterator fixed_lengths = first; // iterator to the fixed lengths part
        std::advance(first, BLOCK_SIZE);    // advance first to the block data part
        for (std::uint32_t i = 0; i < local_index; ++i)
        {
            first += static_cast<unsigned char>(*fixed_lengths++);
        }

        if (local_index < BLOCK_SIZE)
        {
            last = first + static_cast<unsigned char>(*fixed_lengths);
        }
    }
};

template <typename GroupBlock> struct IndexedData
{
    static constexpr std::uint32_t BLOCK_SIZE = GroupBlock::BLOCK_SIZE;

    using BlocksNumberType = std::uint32_t;
    using DataSizeType = std::uint64_t;

    using BlockReference = typename GroupBlock::BlockReference;
    using ResultType = typename GroupBlock::ResultType;
    using ValueType = typename GroupBlock::ValueType;

    static_assert(sizeof(ValueType) == 1, "data basic type must char");

    IndexedData() : blocks_number{0}, block_references{nullptr}, begin{nullptr}, end{nullptr} {}

    bool empty() const { return blocks_number == 0; }

    template <typename OffsetIterator, typename DataIterator>
    void
    write(std::ostream &out, OffsetIterator first, OffsetIterator last, DataIterator data) const
    {
        static_assert(sizeof(typename DataIterator::value_type) == 1, "data basic type must char");

        using diff_type = typename OffsetIterator::difference_type;

        BOOST_ASSERT(first < last);
        const OffsetIterator sentinel = std::prev(last);

        // Write number of blocks
        const auto number_of_elements = std::distance(first, sentinel);
        const BlocksNumberType number_of_blocks =
            number_of_elements == 0 ? 0
                                    : 1 + (std::distance(first, sentinel) - 1) / (BLOCK_SIZE + 1);
        out.write((const char *)&number_of_blocks, sizeof(number_of_blocks));

        // Write block references and compute the total data size that includes prefix and data
        const GroupBlock block;
        DataSizeType data_size = 0;
        for (OffsetIterator curr = first, next = first; next != sentinel; curr = next)
        {
            std::advance(next, std::min<diff_type>(BLOCK_SIZE, std::distance(next, sentinel)));
            data_size += block.WriteBlockReference(out, data_size, curr, next);
            std::advance(next, std::min<diff_type>(1, std::distance(next, sentinel)));
            data_size += *next - *curr;
        }

        // Write the total data size
        out.write((const char *)&data_size, sizeof(data_size));

        // Write data blocks that are (prefix, data)
        for (OffsetIterator curr = first, next = first; next != sentinel; curr = next)
        {
            std::advance(next, std::min<diff_type>(BLOCK_SIZE, std::distance(next, sentinel)));
            block.WriteBlockPrefix(out, curr, next);
            std::advance(next, std::min<diff_type>(1, std::distance(next, sentinel)));
            std::copy(data + *curr, data + *next, std::ostream_iterator<unsigned char>(out));
        }
    }

    /// Set internal pointers from the buffer [first, last).
    /// Data buffer pointed by ptr must exists during IndexedData life-time.
    /// No ownership is transferred.
    void reset(const ValueType *first, const ValueType *last)
    {
        // Read blocks number
        if (first + sizeof(BlocksNumberType) > last)
            throw util::exception("incorrect memory block");

        blocks_number = *reinterpret_cast<const BlocksNumberType *>(first);
        first += sizeof(BlocksNumberType);

        // Get block references pointer
        if (first + sizeof(BlockReference) * blocks_number > last)
            throw util::exception("incorrect memory block");

        block_references = reinterpret_cast<const BlockReference *>(first);
        first += sizeof(BlockReference) * blocks_number;

        // Read total data size
        if (first + sizeof(DataSizeType) > last)
            throw util::exception("incorrect memory block");

        auto data_size = *reinterpret_cast<const DataSizeType *>(first);
        first += sizeof(DataSizeType);

        // Get data blocks begin and end iterators
        begin = reinterpret_cast<const ValueType *>(first);
        first += sizeof(ValueType) * data_size;

        if (first > last)
            throw util::exception("incorrect memory block");

        end = reinterpret_cast<const ValueType *>(first);
    }

    // Return value at the given index
    ResultType at(std::uint32_t index) const
    {
        // Get block external ad internal indices
        const BlocksNumberType block_idx = index / (BLOCK_SIZE + 1);
        const std::uint32_t internal_idx = index % (BLOCK_SIZE + 1);

        if (block_idx >= blocks_number)
            return ResultType();

        // Get block first and last iterators
        auto first = begin + block_references[block_idx].offset;
        auto last =
            block_idx + 1 == blocks_number ? end : begin + block_references[block_idx + 1].offset;

        const GroupBlock block;
        block.ReadRefrencedBlock(block_references[block_idx], internal_idx, first, last);

        return adapt(first, last);
    }

  private:
    template <class T = ResultType>
    typename std::enable_if<!std::is_same<T, StringView>::value, T>::type
    adapt(const ValueType *first, const ValueType *last) const
    {
        return ResultType(first, last);
    }

    template <class T = ResultType>
    typename std::enable_if<std::is_same<T, StringView>::value, T>::type
    adapt(const ValueType *first, const ValueType *last) const
    {
        return ResultType(first, std::distance(first, last));
    }

    BlocksNumberType blocks_number;
    const BlockReference *block_references;
    const ValueType *begin, *end;
};
}
}
#endif // OSRM_INDEXED_DATA_HPP
