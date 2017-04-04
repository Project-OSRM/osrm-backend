#ifndef OSRM_STORAGE_SERIALIZATION_HPP
#define OSRM_STORAGE_SERIALIZATION_HPP

#include "util/vector_view.hpp"

#include "storage/io.hpp"

namespace osrm
{
namespace storage
{
namespace serialization
{

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
    data.resize(count);
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
