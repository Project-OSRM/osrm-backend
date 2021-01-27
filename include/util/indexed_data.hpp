#ifndef OSRM_INDEXED_DATA_HPP
#define OSRM_INDEXED_DATA_HPP

#include "storage/tar_fwd.hpp"

#include "util/exception.hpp"
#include "util/string_view.hpp"
#include "util/vector_view.hpp"

#include <boost/assert.hpp>
#include <boost/function_output_iterator.hpp>

#include <array>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>

namespace osrm
{
namespace util
{
namespace detail
{
template <typename GroupBlockPolicy, storage::Ownership Ownership> struct IndexedDataImpl;
}

namespace serialization
{
template <typename BlockPolicy, storage::Ownership Ownership>
inline void read(storage::tar::FileReader &reader,
                 const std::string &name,
                 detail::IndexedDataImpl<BlockPolicy, Ownership> &index_data);

template <typename BlockPolicy, storage::Ownership Ownership>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const detail::IndexedDataImpl<BlockPolicy, Ownership> &index_data);
} // namespace serialization

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
        if (byte_length == 0) {}
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
    template <typename Offset, typename OffsetIterator, typename OutIter>
    OutIter WriteBlockReference(OffsetIterator first,
                                OffsetIterator last,
                                Offset &data_offset,
                                OutIter out) const
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

        data_offset += prefix_length;
        *out++ = refernce;
        return out;
    }

    /// Write a block prefix that is an array of variable encoded data lengths:
    ///   0 is omitted;
    ///   1..255 is 1 byte;
    ///   256..65535 is 2 bytes;
    ///   65536..16777215 is 3 bytes.
    /// [first..last] is an inclusive range of block data.
    /// The length of the last item in the block is not stored.
    template <typename OffsetIterator, typename OutByteIter>
    OutByteIter WriteBlockPrefix(OffsetIterator first, OffsetIterator last, OutByteIter out) const
    {
        for (OffsetIterator curr = first, next = std::next(first); curr != last; ++curr, ++next)
        {
            const std::uint32_t data_length = *next - *curr;
            const std::uint32_t byte_length = log256(data_length);
            if (byte_length == 0)
                continue;

            // Here, we're only writing a few bytes from the 4-byte std::uint32_t,
            // so we need to cast to (char *)
            out = std::copy_n((const char *)&data_length, byte_length, out);
        }
        return out;
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
    template <typename Offset, typename OffsetIterator, typename OutIterator>
    OutIterator
    WriteBlockReference(OffsetIterator, OffsetIterator, Offset &data_offset, OutIterator out) const
    {
        BOOST_ASSERT(data_offset <= std::numeric_limits<decltype(BlockReference::offset)>::max());

        BlockReference refernce{static_cast<decltype(BlockReference::offset)>(data_offset)};
        data_offset += BLOCK_SIZE;
        *out++ = refernce;

        return out;
    }

    /// Write a fixed length block prefix.
    template <typename OffsetIterator, typename OutByteIter>
    OutByteIter WriteBlockPrefix(OffsetIterator first, OffsetIterator last, OutByteIter out) const
    {
        constexpr std::size_t MAX_LENGTH =
            std::numeric_limits<std::make_unsigned_t<ValueType>>::max();

        auto index = 0;
        std::array<ValueType, BLOCK_SIZE> prefix;

        for (OffsetIterator curr = first, next = std::next(first); curr != last; ++curr, ++next)
        {
            const std::uint32_t data_length = *next - *curr;
            if (data_length > MAX_LENGTH)
                throw util::exception(boost::format("too large data length %1% > %2%") %
                                      data_length % MAX_LENGTH);

            prefix[index++] = data_length;
        }

        out = std::copy_n((const char *)prefix.data(), sizeof(ValueType) * BLOCK_SIZE, out);
        return out;
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

namespace detail
{
template <typename GroupBlockPolicy, storage::Ownership Ownership> struct IndexedDataImpl
{
    static constexpr std::uint32_t BLOCK_SIZE = GroupBlockPolicy::BLOCK_SIZE;

    using BlocksNumberType = std::uint32_t;
    using DataSizeType = std::uint64_t;

    using BlockReference = typename GroupBlockPolicy::BlockReference;
    using ResultType = typename GroupBlockPolicy::ResultType;
    using ValueType = typename GroupBlockPolicy::ValueType;

    static_assert(sizeof(ValueType) == 1, "data basic type must char");

    IndexedDataImpl() = default;
    IndexedDataImpl(util::vector_view<BlockReference> blocks_, util::vector_view<ValueType> values_)
        : blocks(std::move(blocks_)), values(std::move(values_))
    {
    }

    bool empty() const { return blocks.empty(); }

    template <typename OffsetIterator, typename DataIterator>
    IndexedDataImpl(OffsetIterator first, OffsetIterator last, DataIterator data)
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
        blocks.resize(number_of_blocks);

        // Write block references and compute the total data size that includes prefix and data
        const GroupBlockPolicy block;

        auto block_iter = blocks.begin();
        DataSizeType data_size = 0;
        for (OffsetIterator curr = first, next = first; next != sentinel; curr = next)
        {
            std::advance(next, std::min<diff_type>(BLOCK_SIZE, std::distance(next, sentinel)));
            block_iter = block.WriteBlockReference(curr, next, data_size, block_iter);
            std::advance(next, std::min<diff_type>(1, std::distance(next, sentinel)));
            data_size += *next - *curr;
        }

        values.resize(data_size);
        auto values_byte_iter = reinterpret_cast<char *>(values.data());
        // Write data blocks that are (prefix, data)
        for (OffsetIterator curr = first, next = first; next != sentinel; curr = next)
        {
            std::advance(next, std::min<diff_type>(BLOCK_SIZE, std::distance(next, sentinel)));
            values_byte_iter = block.WriteBlockPrefix(curr, next, values_byte_iter);
            std::advance(next, std::min<diff_type>(1, std::distance(next, sentinel)));

            auto to_bytes = [&](const auto &data) {
                values_byte_iter = std::copy_n(&data, sizeof(ValueType), values_byte_iter);
            };
            std::copy(data + *curr,
                      data + *next,
                      boost::make_function_output_iterator(std::cref(to_bytes)));
        }
    }

    // Return value at the given index
    ResultType at(std::uint32_t index) const
    {
        if (values.empty())
            return ResultType();

        // Get block external ad internal indices
        const BlocksNumberType block_idx = index / (BLOCK_SIZE + 1);
        const std::uint32_t internal_idx = index % (BLOCK_SIZE + 1);

        if (block_idx >= blocks.size())
            return ResultType();

        // Get block first and last iterators
        auto first = values.begin() + blocks[block_idx].offset;
        auto last = block_idx + 1 == blocks.size() ? values.end()
                                                   : values.begin() + blocks[block_idx + 1].offset;

        const GroupBlockPolicy block;
        block.ReadRefrencedBlock(blocks[block_idx], internal_idx, first, last);

        return adapt(first, last);
    }

    friend void serialization::read<GroupBlockPolicy, Ownership>(storage::tar::FileReader &reader,
                                                                 const std::string &name,
                                                                 IndexedDataImpl &index_data);

    friend void
    serialization::write<GroupBlockPolicy, Ownership>(storage::tar::FileWriter &writer,
                                                      const std::string &name,
                                                      const IndexedDataImpl &index_data);

  private:
    template <typename Iter, typename T>
    using IsValueIterator =
        std::enable_if_t<std::is_same<T, typename std::iterator_traits<Iter>::value_type>::value>;

    template <typename T = ResultType, typename Iter, typename = IsValueIterator<Iter, ValueType>>
    typename std::enable_if<!std::is_same<T, StringView>::value, T>::type
    adapt(const Iter first, const Iter last) const
    {
        return ResultType(first, last);
    }

    template <typename T = ResultType, typename Iter, typename = IsValueIterator<Iter, ValueType>>
    typename std::enable_if<std::is_same<T, StringView>::value, T>::type
    adapt(const Iter first, const Iter last) const
    {
        auto diff = std::distance(first, last);
        return diff == 0 ? ResultType() : ResultType(&*first, diff);
    }

    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;
    Vector<BlockReference> blocks;
    Vector<ValueType> values;
};
} // namespace detail

template <typename GroupBlockPolicy>
using IndexedData = detail::IndexedDataImpl<GroupBlockPolicy, storage::Ownership::Container>;
template <typename GroupBlockPolicy>
using IndexedDataView = detail::IndexedDataImpl<GroupBlockPolicy, storage::Ownership::View>;
} // namespace util
} // namespace osrm
#endif // OSRM_INDEXED_DATA_HPP
