#ifndef OSRM_STORAGE_SERIALIZATION_HPP
#define OSRM_STORAGE_SERIALIZATION_HPP

#include "util/integer_range.hpp"
#include "util/vector_view.hpp"

#include "storage/io.hpp"

#include <cstdint>

namespace osrm
{
namespace storage
{
namespace serialization
{
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
    return writer.WriteFrom(data.data(), count);
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
    return writer.WriteFrom(data.data(), count);
}
}
}
}

#endif
