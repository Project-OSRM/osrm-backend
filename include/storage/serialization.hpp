#ifndef OSRM_STORAGE_SERIALIZATION_HPP
#define OSRM_STORAGE_SERIALIZATION_HPP

#include "util/deallocating_vector.hpp"
#include "util/integer_range.hpp"
#include "util/vector_view.hpp"

#include "storage/io.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/tar.hpp"

#include <boost/assert.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/iterator/function_input_iterator.hpp>

#include <cmath>
#include <cstdint>
#include <tuple>

#if USE_STXXL_LIBRARY
#include <stxxl/vector>
#endif

namespace osrm
{
namespace storage
{
namespace serialization
{

namespace detail
{
template <typename T, typename BlockT = unsigned char>
inline BlockT packBits(const T &data, std::size_t base_index, const std::size_t count)
{
    static_assert(std::is_same<typename T::value_type, bool>::value, "value_type is not bool");
    static_assert(std::is_unsigned<BlockT>::value, "BlockT must be unsigned type");
    static_assert(std::is_integral<BlockT>::value, "BlockT must be an integral type");
    static_assert(CHAR_BIT == 8, "Non-8-bit bytes not supported, sorry!");
    BOOST_ASSERT(sizeof(BlockT) * CHAR_BIT >= count);

    // Note: if this packing is changed, be sure to update vector_view<bool>
    //       as well, so that on-disk and in-memory layouts match.
    BlockT value = 0;
    for (std::size_t bit = 0; bit < count; ++bit)
    {
        value |= (data[base_index + bit] ? BlockT{1} : BlockT{0}) << bit;
    }
    return value;
}

template <typename T, typename BlockT = unsigned char>
inline void
unpackBits(T &data, const std::size_t base_index, const std::size_t count, const BlockT value)
{
    static_assert(std::is_same<typename T::value_type, bool>::value, "value_type is not bool");
    static_assert(std::is_unsigned<BlockT>::value, "BlockT must be unsigned type");
    static_assert(std::is_integral<BlockT>::value, "BlockT must be an integral type");
    static_assert(CHAR_BIT == 8, "Non-8-bit bytes not supported, sorry!");
    BOOST_ASSERT(sizeof(BlockT) * CHAR_BIT >= count);
    for (std::size_t bit = 0; bit < count; ++bit)
    {
        data[base_index + bit] = value & (BlockT{1} << bit);
    }
}

template <typename VectorT>
void readBoolVector(tar::FileReader &reader, const std::string &name, VectorT &data)
{
    const auto count = reader.ReadElementCount64(name);
    data.resize(count);
    std::uint64_t index = 0;

    using BlockType = std::uint64_t;
    constexpr std::uint64_t BLOCK_BITS = CHAR_BIT * sizeof(BlockType);

    const auto decode = [&](const BlockType block) {
        auto read_size = std::min<std::size_t>(count - index, BLOCK_BITS);
        unpackBits<VectorT, BlockType>(data, index, read_size, block);
        index += BLOCK_BITS;
    };

    reader.ReadStreaming<BlockType>(name, boost::make_function_output_iterator(decode));
}

template <typename VectorT>
void writeBoolVector(tar::FileWriter &writer, const std::string &name, const VectorT &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(name, count);
    std::uint64_t index = 0;

    using BlockType = std::uint64_t;
    constexpr std::uint64_t BLOCK_BITS = CHAR_BIT * sizeof(BlockType);

    // FIXME on old boost version the function_input_iterator does not work with lambdas
    // so we need to wrap it in a function here.
    const std::function<BlockType()> encode_function = [&]() -> BlockType {
        auto write_size = std::min<std::size_t>(count - index, BLOCK_BITS);
        auto packed = packBits<VectorT, BlockType>(data, index, write_size);
        index += BLOCK_BITS;
        return packed;
    };

    std::uint64_t number_of_blocks = (count + BLOCK_BITS - 1) / BLOCK_BITS;
    writer.WriteStreaming<BlockType>(
        name,
        boost::make_function_input_iterator(encode_function, boost::infinite()),
        number_of_blocks);
}
}

/* All vector formats here use the same on-disk format.
 * This is important because we want to be able to write from a vector
 * of one kind, but read it into a vector of another kind.
 *
 * All vector types with this guarantee should be placed in this file.
 */

template <typename T>
inline void
read(storage::tar::FileReader &reader, const std::string &name, util::DeallocatingVector<T> &vec)
{
    vec.resize(reader.ReadElementCount64(name));
    reader.ReadStreaming<T>(name, vec.begin(), vec.size());
}

template <typename T>
inline void write(storage::tar::FileWriter &writer,
                  const std::string &name,
                  const util::DeallocatingVector<T> &vec)
{
    writer.WriteElementCount64(name, vec.size());
    writer.WriteStreaming<T>(name, vec.begin(), vec.size());
}

#if USE_STXXL_LIBRARY
template <typename T>
inline void read(storage::tar::FileReader &reader, const std::string &name, stxxl::vector<T> &vec)
{
    auto size = reader.ReadElementCount64(name);
    vec.reserve(size);
    reader.ReadStreaming<T>(name, std::back_inserter(vec), size);
}

template <typename T>
inline void
write(storage::tar::FileWriter &writer, const std::string &name, const stxxl::vector<T> &vec)
{
    writer.WriteElementCount64(name, vec.size());
    writer.WriteStreaming<T>(name, vec.begin(), vec.size());
}
#endif

template <typename T> void read(io::BufferReader &reader, std::vector<T> &data)
{
    const auto count = reader.ReadElementCount64();
    data.resize(count);
    reader.ReadInto(data.data(), count);
}

template <typename T> void write(io::BufferWriter &writer, const std::vector<T> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    writer.WriteFrom(data.data(), count);
}

template <typename T> inline void write(io::BufferWriter &writer, const T &data)
{
    writer.WriteFrom(data);
}

template <typename T> inline void read(io::BufferReader &reader, T &data) { reader.ReadInto(data); }

inline void write(io::BufferWriter &writer, const std::string &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    writer.WriteFrom(data.data(), count);
}

inline void read(io::BufferReader &reader, std::string &data)
{
    const auto count = reader.ReadElementCount64();
    data.resize(count);
    reader.ReadInto(const_cast<char *>(data.data()), count);
}

inline void write(tar::FileWriter &writer, const std::string &name, const std::string &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(name, count);
    writer.WriteFrom(name, data.data(), count);
}

inline void read(tar::FileReader &reader, const std::string &name, std::string &data)
{
    const auto count = reader.ReadElementCount64(name);
    data.resize(count);
    reader.ReadInto(name, const_cast<char *>(data.data()), count);
}

template <typename T>
inline void read(tar::FileReader &reader, const std::string &name, std::vector<T> &data)
{
    const auto count = reader.ReadElementCount64(name);
    data.resize(count);
    reader.ReadInto(name, data.data(), count);
}

template <typename T>
void write(tar::FileWriter &writer, const std::string &name, const std::vector<T> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(name, count);
    writer.WriteFrom(name, data.data(), count);
}

template <typename T>
void read(tar::FileReader &reader, const std::string &name, util::vector_view<T> &data)
{
    const auto count = reader.ReadElementCount64(name);
    data.resize(count);
    reader.ReadInto(name, data.data(), count);
}

template <typename T>
void write(tar::FileWriter &writer, const std::string &name, const util::vector_view<T> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(name, count);
    writer.WriteFrom(name, data.data(), count);
}

template <>
inline void
read<bool>(tar::FileReader &reader, const std::string &name, util::vector_view<bool> &data)
{
    detail::readBoolVector(reader, name, data);
}

template <>
inline void
write<bool>(tar::FileWriter &writer, const std::string &name, const util::vector_view<bool> &data)
{
    detail::writeBoolVector(writer, name, data);
}

template <>
inline void read<bool>(tar::FileReader &reader, const std::string &name, std::vector<bool> &data)
{
    detail::readBoolVector(reader, name, data);
}

template <>
inline void
write<bool>(tar::FileWriter &writer, const std::string &name, const std::vector<bool> &data)
{
    detail::writeBoolVector(writer, name, data);
}

template <typename K, typename V> void read(io::BufferReader &reader, std::map<K, V> &data)
{
    const auto count = reader.ReadElementCount64();
    for (auto index : util::irange<std::size_t>(0, count))
    {
        (void)index;
        std::pair<K, V> pair;
        read(reader, pair.first);
        read(reader, pair.second);
        data.insert(pair);
    }
}

template <typename K, typename V> void write(io::BufferWriter &writer, const std::map<K, V> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    for (const auto &pair : data)
    {
        write(writer, pair.first);
        write(writer, pair.second);
    }
}

inline void read(io::BufferReader &reader, BaseDataLayout &layout) { read(reader, layout.blocks); }

inline void write(io::BufferWriter &writer, const BaseDataLayout &layout)
{
    write(writer, layout.blocks);
}
}
}
}

#endif
