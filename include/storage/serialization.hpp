#ifndef OSRM_STORAGE_SERIALIZATION_HPP
#define OSRM_STORAGE_SERIALIZATION_HPP

#include "util/deallocating_vector.hpp"
#include "util/integer_range.hpp"
#include "util/vector_view.hpp"

#include "storage/io.hpp"
#include "storage/tar.hpp"

#include <boost/function_output_iterator.hpp>
#include <boost/iterator/function_input_iterator.hpp>

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

namespace detail
{
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

template <typename VectorT>
void readBoolVector(tar::FileReader &reader, const std::string &name, VectorT &data)
{
    const auto count = reader.ReadElementCount64(name);
    data.resize(count);
    std::uint64_t index = 0;

    const auto decode = [&](const unsigned char block) {
        auto read_size = std::min<std::size_t>(count - index, CHAR_BIT);
        unpackBits(data, index, read_size, block);
        index += CHAR_BIT;
    };

    reader.ReadStreaming<unsigned char>(name, boost::make_function_output_iterator(decode));
}

template <typename VectorT>
void writeBoolVector(tar::FileWriter &writer, const std::string &name, const VectorT &data)
{
    const auto count = data.size();
    writer.WriteElementCount64(name, count);
    std::uint64_t index = 0;

    // FIXME on old boost version the function_input_iterator does not work with lambdas
    // so we need to wrap it in a function here.
    const std::function<char()> encode_function = [&]() -> char {
        auto write_size = std::min<std::size_t>(count - index, CHAR_BIT);
        auto packed = packBits(data, index, write_size);
        index += CHAR_BIT;
        return packed;
    };

    std::uint64_t number_of_blocks = std::ceil((double)count / CHAR_BIT);
    writer.WriteStreaming<unsigned char>(
        name,
        boost::make_function_input_iterator(encode_function, boost::infinite()),
        number_of_blocks);
}
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
}
}
}

#endif
