#ifndef OSRM_UTIL_BLOCK_DELTA_ENCODING_HPP
#define OSRM_UTIL_BLOCK_DELTA_ENCODING_HPP

#include "util/block_delta.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

namespace osrm::util
{
struct EncodedBlocks
{
    std::vector<BlockDeltaHeader> Headers;
    std::vector<std::uint64_t> Words;
};

template <typename T> std::optional<EncodedBlocks> encodeBlockDelta(std::span<const T> values)
{
    static_assert(std::is_unsigned_v<T>);
    EncodedBlocks result;
    for (std::size_t begin = 0; begin < values.size(); begin += 128)
    {
        const auto end = std::min(values.size(), begin + 128);
        const auto [minimum, maximum] =
            std::minmax_element(values.begin() + begin, values.begin() + end);
        const auto base = std::uint64_t(*minimum);
        const auto width = std::bit_width(std::uint64_t(*maximum) - base);
        if (result.Words.size() > std::numeric_limits<std::uint32_t>::max())
            return std::nullopt;
        result.Headers.push_back({base,
                                  static_cast<std::uint32_t>(result.Words.size()),
                                  static_cast<std::uint8_t>(width),
                                  {}});
        const auto word_base = result.Words.size();
        result.Words.resize(word_base + ((end - begin) * width + 63) / 64, 0);
        if (width == 0)
            continue;
        for (auto i = begin; i < end; ++i)
        {
            const auto bit = (i - begin) * width;
            const auto delta = std::uint64_t(values[i]) - base;
            result.Words[word_base + bit / 64] |= delta << (bit % 64);
            if (bit % 64 + width > 64)
                result.Words[word_base + bit / 64 + 1] |= delta >> (64 - bit % 64);
        }
    }
    // The query decoder may make an unaligned eight-byte load at the final value.
    result.Words.push_back(0);
    return result;
}
} // namespace osrm::util

#endif
