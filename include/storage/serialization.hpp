#ifndef OSRM_STORAGE_SERIALIZATION_HPP
#define OSRM_STORAGE_SERIALIZATION_HPP

#include "util/deallocating_vector.hpp"
#include "util/integer_range.hpp"
#include "util/vector_view.hpp"

#include "storage/io.hpp"

#include <cmath>
#include <cstdint>

#if USE_STXXL_LIBRARY
#include <stxxl/vector>
#endif

namespace osrm
{
namespace storage
{
namespace serialization
{

/* All vector formats here use the same on-disk format.
 * This is important because we want to be able to write from a vector
 * of one kind, but read it into a vector of another kind.
 *
 * All vector types with this guarantee should be placed in this file.
 */

template <typename T>
inline void read(storage::io::FileReader &reader, util::DeallocatingVector<T> &vec)
{
    vec.current_size = reader.ReadElementCount64(vec.current_size);
    std::size_t num_blocks =
        std::ceil(vec.current_size / util::DeallocatingVector<T>::ELEMENTS_PER_BLOCK);
    vec.bucket_list.resize(num_blocks);
    // Read all but the last block which can be partital
    for (auto bucket_index : util::irange<std::size_t>(0, num_blocks - 1))
    {
        vec.bucket_list[bucket_index] = new T[util::DeallocatingVector<T>::ELEMENTS_PER_BLOCK];
        reader.ReadInto(vec.bucket_list[bucket_index],
                        util::DeallocatingVector<T>::ELEMENTS_PER_BLOCK);
    }
    std::size_t last_block_size =
        vec.current_size % util::DeallocatingVector<T>::ELEMENTS_PER_BLOCK;
    vec.bucket_list.back() = new T[util::DeallocatingVector<T>::ELEMENTS_PER_BLOCK];
    reader.ReadInto(vec.bucket_list.back(), last_block_size);
}

template <typename T>
inline void write(storage::io::FileWriter &writer, const util::DeallocatingVector<T> &vec)
{
    writer.WriteElementCount64(vec.current_size);
    // Write all but the last block which can be partially filled
    for (auto bucket_index : util::irange<std::size_t>(0, vec.bucket_list.size() - 1))
    {
        writer.WriteFrom(vec.bucket_list[bucket_index],
                         util::DeallocatingVector<T>::ELEMENTS_PER_BLOCK);
    }
    std::size_t last_block_size =
        vec.current_size % util::DeallocatingVector<T>::ELEMENTS_PER_BLOCK;
    writer.WriteFrom(vec.bucket_list.back(), last_block_size);
}

#if USE_STXXL_LIBRARY
template <typename T> inline void read(storage::io::FileReader &reader, stxxl::vector<T> &vec)
{
    auto size = reader.ReadOne<std::uint64_t>();
    vec.reserve(size);
    for (auto idx : util::irange<std::size_t>(0, size))
    {
        (void)idx;
        vec.push_back(reader.ReadOne<T>());
    }
}

template <typename T>
inline void write(storage::io::FileWriter &writer, const stxxl::vector<T> &vec)
{
    writer.WriteOne(vec.size());
    for (auto idx : util::irange<std::size_t>(0, vec.size()))
    {
        writer.WriteOne<T>(vec[idx]);
    }
}
#endif

template <typename T> void read(io::FileReader &reader, std::vector<T> &data)
{
    const auto count = reader.ReadElementCount64();
    data.resize(count);
    reader.ReadInto(data.data(), count);
}

template <typename T> void write(io::FileWriter &writer, const std::vector<T> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    writer.WriteFrom(data.data(), count);
}

template <typename T> void read(io::FileReader &reader, util::vector_view<T> &data)
{
    const auto count = reader.ReadElementCount64();
    BOOST_ASSERT(data.size() == count);
    reader.ReadInto(data.data(), count);
}

template <typename T> void write(io::FileWriter &writer, const util::vector_view<T> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    writer.WriteFrom(data.data(), count);
}

template <typename T>
inline unsigned char packBits(const T &data, std::size_t index, std::size_t count)
{
    static_assert(std::is_same<typename T::value_type, bool>::value, "value_type is not bool");
    unsigned char value = 0;
    for (std::size_t bit = 0; bit < count; ++bit, ++index)
        value = (value << 1) | data[index];
    return value;
}

template <typename T>
inline void unpackBits(T &data, std::size_t index, std::size_t count, unsigned char value)
{
    static_assert(std::is_same<typename T::value_type, bool>::value, "value_type is not bool");
    const unsigned char mask = 1 << (count - 1);
    for (std::size_t bit = 0; bit < count; value <<= 1, ++bit, ++index)
        data[index] = value & mask;
}

template <> inline void read<bool>(io::FileReader &reader, util::vector_view<bool> &data)
{
    const auto count = reader.ReadElementCount64();
    BOOST_ASSERT(data.size() == count);
    std::uint64_t index = 0;
    for (std::uint64_t next = CHAR_BIT; next < count; index = next, next += CHAR_BIT)
    {
        unpackBits(data, index, CHAR_BIT, reader.ReadOne<unsigned char>());
    }
    if (count > index)
        unpackBits(data, index, count - index, reader.ReadOne<unsigned char>());
}

template <> inline void write<bool>(io::FileWriter &writer, const util::vector_view<bool> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    std::uint64_t index = 0;
    for (std::uint64_t next = CHAR_BIT; next < count; index = next, next += CHAR_BIT)
    {
        writer.WriteOne<unsigned char>(packBits(data, CHAR_BIT * index, CHAR_BIT));
    }
    if (count > index)
        writer.WriteOne<unsigned char>(packBits(data, index, count - index));
}

template <> inline void read<bool>(io::FileReader &reader, std::vector<bool> &data)
{
    const auto count = reader.ReadElementCount64();
    data.resize(count);
    std::uint64_t index = 0;
    for (std::uint64_t next = CHAR_BIT; next < count; index = next, next += CHAR_BIT)
    {
        unpackBits(data, index, CHAR_BIT, reader.ReadOne<unsigned char>());
    }
    if (count > index)
        unpackBits(data, index, count - index, reader.ReadOne<unsigned char>());
}

template <> inline void write<bool>(io::FileWriter &writer, const std::vector<bool> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    std::uint64_t index = 0;
    for (std::uint64_t next = CHAR_BIT; next < count; index = next, next += CHAR_BIT)
    {
        writer.WriteOne<unsigned char>(packBits(data, index, CHAR_BIT));
    }
    if (count > index)
        writer.WriteOne<unsigned char>(packBits(data, index, count - index));
}
}
}
}

#endif
