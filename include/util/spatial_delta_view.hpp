#ifndef OSRM_UTIL_SPATIAL_DELTA_VIEW_HPP
#define OSRM_UTIL_SPATIAL_DELTA_VIEW_HPP

#include "osrm/coordinate.hpp"
#include "util/block_delta.hpp"
#include "util/exception.hpp"
#include "util/typedefs.hpp"

#include <bit>
#include <ranges>
#include <span>

namespace osrm::util
{
struct SpatialDeltaView
{
    std::span<const BlockDeltaHeader> headers;
    std::span<const std::uint64_t> words;

    std::uint32_t operator[](std::size_t index) const
    { return static_cast<std::uint32_t>(decodeBlockDeltaValues(headers, words, index)); }

    void validate(std::size_t count) const
    {
        const auto block_count = count / 128 + (count % 128 != 0);
        if (headers.size() != block_count || words.empty())
            throw exception("Invalid spatial delta block count");
        for (std::size_t block = 0; block < block_count; ++block)
        {
            const auto &header = headers[block];
            const auto entries = std::min(count - block * 128, std::size_t{128});
            const auto word_count = (entries * header.width + 63) / 64;
            if (header.width > 32 || header.word_offset >= words.size() ||
                word_count >= words.size() - header.word_offset)
                throw exception("Invalid spatial delta word range");
        }
    }

    explicit operator bool() const { return !headers.empty(); }
};

inline std::uint32_t spatialUnsigned(std::int32_t value)
{ return std::bit_cast<std::uint32_t>(value) ^ 0x80000000u; }

inline std::int32_t spatialSigned(std::uint32_t value)
{ return std::bit_cast<std::int32_t>(value ^ 0x80000000u); }

struct SpatialCoordinateView
{
    std::span<const Coordinate> coordinates;
    SpatialDeltaView lon, lat;
    std::size_t count = 0;

    SpatialCoordinateView() = default;
    SpatialCoordinateView(const Coordinate *data, std::size_t count)
        : coordinates(data, count), count(count)
    {
    }
    SpatialCoordinateView(vector_view<Coordinate> view)
        : SpatialCoordinateView(view.data(), view.size())
    {
    }

    SpatialCoordinateView(SpatialDeltaView lon, SpatialDeltaView lat, std::size_t count)
        : lon(lon), lat(lat), count(count)
    {
    }

    std::size_t size() const { return count; }

    Coordinate operator[](std::size_t index) const
    {
        if (lon)
            return {FixedLongitude{spatialSigned(lon[index])},
                    FixedLatitude{spatialSigned(lat[index])}};
        return coordinates[index];
    }
};

struct SpatialNodeLookup
{
    std::span<const NodeID> plain;
    SpatialDeltaView packed;

    NodeID operator()(std::size_t index) const { return packed ? packed[index] : plain[index]; }
};

using SpatialNodeRange =
    std::ranges::transform_view<std::ranges::iota_view<std::size_t, std::size_t>,
                                SpatialNodeLookup>;

inline SpatialNodeRange
makeSpatialNodeRange(SpatialNodeLookup lookup, std::size_t begin, std::size_t end)
{ return SpatialNodeRange{std::views::iota(begin, end), lookup}; }
} // namespace osrm::util

#endif
