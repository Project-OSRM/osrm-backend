#ifndef OSRM_UTIL_BLOCK_DELTA_HPP
#define OSRM_UTIL_BLOCK_DELTA_HPP

#include "util/vector_view.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <boost/assert.hpp>
#include <cstdint>

namespace osrm::util
{
struct BlockDeltaHeader
{
    std::uint64_t base;
    std::uint32_t word_offset;
    std::uint8_t width;
    std::uint8_t reserved[3]{};
};
static_assert(sizeof(BlockDeltaHeader) == 16);
inline std::uint64_t decodeBlockDelta(vector_view<BlockDeltaHeader> headers,
                                      vector_view<std::uint64_t> words,
                                      std::size_t index)
{
    const auto &header = headers[index / 128];
    const auto width = header.width;
    if (width == 0)
        return header.base;
    BOOST_ASSERT(width <= 64);
    const auto bit = (index % 128) * width;
    if (std::endian::native == std::endian::little && width <= 57)
    {
        std::array<std::uint8_t, sizeof(std::uint64_t)> value_bytes;
        const auto bytes =
            reinterpret_cast<const std::uint8_t *>(words.data() + header.word_offset);
        std::copy_n(bytes + bit / 8, value_bytes.size(), value_bytes.begin());
        const auto value = std::bit_cast<std::uint64_t>(value_bytes);
        return header.base + ((value >> (bit % 8)) & ((std::uint64_t{1} << width) - 1));
    }
    const auto offset = bit % 64;
    auto delta = words[header.word_offset + bit / 64] >> offset;
    if (offset + width > 64)
        delta |= words[header.word_offset + bit / 64 + 1] << (64 - offset);
    if (width < 64)
        delta &= (std::uint64_t{1} << width) - 1;
    return header.base + delta;
}
} // namespace osrm::util
#endif
