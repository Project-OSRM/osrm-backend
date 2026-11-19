#ifndef OSRM_EXTRACTOR_SEGMENT_COST_ENCODING_HPP
#define OSRM_EXTRACTOR_SEGMENT_COST_ENCODING_HPP

#include "util/exception.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <vector>

namespace osrm::storage
{
struct GeometryCosts
{
    std::vector<std::uint64_t> Descriptors;
    std::vector<std::uint64_t> Words{0};
    std::uint64_t Entries = 0;
};

template <typename Segment>
inline std::optional<GeometryCosts> encodeGeometryCosts(const Segment &segment)
{
    GeometryCosts result;
    const auto count = segment.GetNumberOfGeometries();
    result.Descriptors.reserve(count + 1);
    std::vector<std::array<std::uint32_t, 4>> values;
    std::uint64_t bit = 0;
    for (std::size_t geometry = 0; geometry < count; ++geometry)
    {
        const auto fw = segment.GetForwardWeights(geometry);
        const auto rw = segment.GetReverseWeights(geometry);
        const auto fd = segment.GetForwardDurations(geometry);
        const auto rd = segment.GetReverseDurations(geometry);
        const auto length = fw.size();
        if (length > 65535 || bit > 0xffffffffULL)
            return std::nullopt;
        if (fd.size() != length || rw.size() != length || rd.size() != length)
            throw util::exception("Geometry cost dimensions do not agree");
        values.resize(length);
        std::array<std::array<bool, 4>, 4> equal;
        for (auto &row : equal)
            row.fill(true);
        std::uint32_t max_finite = 0;
        bool sentinel = false;
        for (std::size_t k = 0; k < length; ++k)
        {
            values[k] = {static_cast<std::uint32_t>(fw[k]),
                         static_cast<std::uint32_t>(fd[k]),
                         static_cast<std::uint32_t>(rw[length - 1 - k]),
                         static_cast<std::uint32_t>(rd[length - 1 - k])};
            for (auto value : values[k])
                if (value == 4194303)
                    sentinel = true;
                else
                    max_finite = std::max(max_finite, value);
            for (unsigned a = 0; a < 4; ++a)
                for (unsigned b = 0; b < a; ++b)
                    equal[a][b] &= values[k][a] == values[k][b];
        }
        std::array<unsigned, 4> representatives{}, ordinals{};
        unsigned unique = 0;
        for (unsigned a = 0; a < 4; ++a)
        {
            bool found = false;
            for (unsigned b = 0; b < unique; ++b)
                if (equal[a][representatives[b]])
                {
                    ordinals[a] = b;
                    found = true;
                    break;
                }
            if (!found)
            {
                ordinals[a] = unique;
                representatives[unique++] = a;
            }
        }
        const auto width = std::bit_width(std::uint64_t(max_finite) + unsigned(sentinel));
        if (width > 22)
            return std::nullopt;
        const auto final_bit = bit + std::uint64_t(unique) * length * width;
        if (final_bit > 0xffffffffULL)
            return std::nullopt;
        const auto base = bit;
        const std::uint64_t mask = (1ULL << width) - 1;
        result.Descriptors.push_back(
            base | (std::uint64_t(length) << 32) | (std::uint64_t(ordinals[1]) << 48) |
            (std::uint64_t(ordinals[2]) << 50) | (std::uint64_t(ordinals[3]) << 52) |
            (std::uint64_t(width) << 54) | (std::uint64_t(sentinel) << 59) | (1ULL << 63));
        result.Words.resize((final_bit + 63) / 64 + 1, 0);
        for (unsigned a = 0; a < unique; ++a)
            for (std::size_t k = 0; k < length; ++k)
            {
                const std::uint64_t value =
                    values[k][representatives[a]] == 4194303 ? mask : values[k][representatives[a]];
                result.Words[bit / 64] |= value << (bit % 64);
                if (bit % 64 + width > 64)
                    result.Words[bit / 64 + 1] |= value >> (64 - bit % 64);
                bit += width;
            }
        for (unsigned a = 0; a < 4; ++a)
            for (std::size_t k = 0; k < length; ++k)
            {
                const auto position = base + (ordinals[a] * length + k) * width;
                auto value = result.Words[position / 64] >> (position % 64);
                if (position % 64 + width > 64)
                    value |= result.Words[position / 64 + 1] << (64 - position % 64);
                value &= mask;
                if (sentinel && value == mask)
                    value = 4194303;
                if (value != values[k][a])
                    throw util::exception("Geometry cost reconstruction failed");
            }
        result.Entries += std::uint64_t(unique) * length;
    }
    result.Descriptors.push_back(result.Entries);
    return result;
}

} // namespace osrm::storage
#endif
