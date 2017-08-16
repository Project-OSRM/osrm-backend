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

template <> inline void read<bool>(io::FileReader &reader, util::vector_view<bool> &data)
{
    const auto count = reader.ReadElementCount64();
    BOOST_ASSERT(data.size() == count);
    for (const auto index : util::irange<std::uint64_t>(0, count))
    {
        data[index] = reader.ReadOne<bool>();
    }
}

template <> inline void write<bool>(io::FileWriter &writer, const util::vector_view<bool> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    for (const auto index : util::irange<std::uint64_t>(0, count))
    {
        writer.WriteOne<bool>(data[index]);
    }
}

template <> inline void read<bool>(io::FileReader &reader, std::vector<bool> &data)
{
    const auto count = reader.ReadElementCount64();
    data.resize(count);
    for (const auto index : util::irange<std::uint64_t>(0, count))
    {
        data[index] = reader.ReadOne<bool>();
    }
}

template <> inline void write<bool>(io::FileWriter &writer, const std::vector<bool> &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(count);
    for (const auto index : util::irange<std::uint64_t>(0, count))
    {
        writer.WriteOne<bool>(data[index]);
    }
}
}
}
}

#endif
