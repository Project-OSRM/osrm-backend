#ifndef OSRM_STORAGE_BLOCK_VECTOR_ENCODING_HPP
#define OSRM_STORAGE_BLOCK_VECTOR_ENCODING_HPP
#include "storage/serialization.hpp"
#include "util/block_delta_encoding.hpp"
#include <array>
#include <bit>
#include <limits>
#include <optional>
#include <span>
#include <vector>
namespace osrm::storage
{
template <typename T>
void writeCompactVector(tar::FileWriter &writer, const std::string &name, std::span<const T> values)
{
    static_assert(std::is_unsigned_v<T>);
    auto blocks = util::encodeBlockDelta<T>(values);
    if (blocks && blocks->Headers.size() * sizeof(util::BlockDeltaHeader) +
                          blocks->Words.size() * sizeof(std::uint64_t) <
                      values.size_bytes())
    {
        const auto prefix = name + "_blocks";
        serialization::write(writer, prefix + "/headers", blocks->Headers);
        serialization::write(writer, prefix + "/words", blocks->Words);
        writer.WriteElementCount64(prefix + "/count", 1);
        writer.WriteFrom(prefix + "/count", std::uint64_t(values.size()));
    }
    else
    {
        writer.WriteElementCount64(name, values.size());
        writer.WriteFrom(name, values.data(), values.size());
    }
}
template <typename T>
void readCompactVector(tar::FileReader &reader, const std::string &name, std::vector<T> &values)
{
    if (!reader.HasEntry(name + "_blocks/headers"))
    {
        serialization::read(reader, name, values);
        return;
    }
    std::vector<util::BlockDeltaHeader> headers;
    std::vector<std::uint64_t> words;
    serialization::read(reader, name + "_blocks/headers", headers);
    serialization::read(reader, name + "_blocks/words", words);
    std::uint64_t count;
    reader.ReadInto(name + "_blocks/count", count);
    if (headers.size() != (count + 127) / 128)
        throw util::exception("Invalid compact vector dimensions");
    values.resize(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto value = util::decodeBlockDelta(
            {headers.data(), headers.size()}, {words.data(), words.size()}, i);
        if (value > std::numeric_limits<T>::max())
            throw util::exception("Compact vector value out of range");
        values[i] = static_cast<T>(value);
    }
}
} // namespace osrm::storage
#endif
