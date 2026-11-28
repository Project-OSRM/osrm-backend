#ifndef OSRM_EXTRACTOR_NODE_LOCATION_INDEX_HPP
#define OSRM_EXTRACTOR_NODE_LOCATION_INDEX_HPP

#include "extractor/query_node.hpp"
#include "util/block_delta_encoding.hpp"

#include <osmium/index/map/flex_mem.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

#include <algorithm>
#include <bit>
#include <memory>
#include <unordered_map>
#include <vector>

namespace osrm::extractor
{
class NodeLocationIndex final
    : public osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>
{
    using ID = osmium::unsigned_object_id_type;
    using LegacyIndex = osmium::index::map::FlexMem<ID, osmium::Location>;
    static constexpr unsigned SuffixBits = 16;

    struct Block
    {
        ID prefix;
        std::vector<std::uint16_t> suffixes;
        std::vector<osmium::Location> locations;
        util::EncodedBlocks longitudes;
        util::EncodedBlocks latitudes;

        std::size_t usedMemory() const
        {
            return suffixes.capacity() * sizeof(std::uint16_t) +
                   locations.capacity() * sizeof(osmium::Location) +
                   (longitudes.Headers.capacity() + latitudes.Headers.capacity()) *
                       sizeof(util::BlockDeltaHeader) +
                   (longitudes.Words.capacity() + latitudes.Words.capacity()) *
                       sizeof(std::uint64_t);
        }

        void finish()
        {
            suffixes.shrink_to_fit();
            locations.shrink_to_fit();
            if (locations.size() < 128)
                return;

            std::vector<std::uint32_t> column(locations.size());
            const auto encode = [&](auto coordinate)
            {
                std::transform(locations.begin(),
                               locations.end(),
                               column.begin(),
                               [&](const auto &location)
                               {
                                   return std::bit_cast<std::uint32_t>(coordinate(location)) ^
                                          (std::uint32_t{1} << 31);
                               });
                return util::encodeBlockDelta<std::uint32_t>(column);
            };
            auto lon = encode([](const auto &location) { return location.x(); });
            auto lat = encode([](const auto &location) { return location.y(); });
            if (!lon || !lat)
                return;
            const auto bytes =
                (lon->Headers.size() + lat->Headers.size()) * sizeof(util::BlockDeltaHeader) +
                (lon->Words.size() + lat->Words.size()) * sizeof(std::uint64_t);
            if (bytes >= locations.size() * sizeof(osmium::Location))
                return;

            lon->Headers.shrink_to_fit();
            lon->Words.shrink_to_fit();
            lat->Headers.shrink_to_fit();
            lat->Words.shrink_to_fit();
            longitudes = std::move(*lon);
            latitudes = std::move(*lat);
            std::vector<osmium::Location>().swap(locations);
        }

        osmium::Location at(std::size_t index) const noexcept
        {
            if (!locations.empty())
                return locations[index];
            const auto coordinate = [&](const util::EncodedBlocks &column)
            {
                const auto value =
                    util::decodeBlockDeltaValues(column.Headers, column.Words, index);
                return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value) ^
                                                   (std::uint32_t{1} << 31));
            };
            return osmium::Location(coordinate(longitudes), coordinate(latitudes));
        }
    };

    std::vector<Block> m_blocks;
    std::unordered_map<ID, std::size_t> m_block_by_prefix;
    std::unique_ptr<LegacyIndex> m_legacy_index;
    ID m_last_id = 0;
    std::size_t m_node_count = 0;

    void releaseBlocks()
    {
        decltype(m_blocks)().swap(m_blocks);
        decltype(m_block_by_prefix)().swap(m_block_by_prefix);
        m_node_count = 0;
    }

  public:
    void set(ID id, osmium::Location location) final
    {
        if (!m_legacy_index && !m_blocks.empty() && id <= m_last_id)
        {
            // Reinsert in input order to preserve libosmium's duplicate handling.
            m_legacy_index = std::make_unique<LegacyIndex>();
            for (const auto &block : m_blocks)
            {
                for (std::size_t i = 0; i < block.suffixes.size(); ++i)
                    m_legacy_index->set((block.prefix << SuffixBits) | block.suffixes[i],
                                        block.at(i));
            }
            releaseBlocks();
        }
        if (m_legacy_index)
        {
            m_legacy_index->set(id, location);
            return;
        }

        const auto prefix = id >> SuffixBits;
        if (m_blocks.empty() || m_blocks.back().prefix != prefix)
        {
            if (!m_blocks.empty())
                m_blocks.back().finish();
            m_block_by_prefix.emplace(prefix, m_blocks.size());
            m_blocks.push_back(Block{prefix, {}, {}, {}, {}});
        }
        m_blocks.back().suffixes.push_back(static_cast<std::uint16_t>(id));
        m_blocks.back().locations.push_back(location);
        m_last_id = id;
        ++m_node_count;
    }

    osmium::Location get_noexcept(ID id) const noexcept final
    {
        if (m_legacy_index)
            return m_legacy_index->get_noexcept(id);
        const auto found = m_block_by_prefix.find(id >> SuffixBits);
        if (found == m_block_by_prefix.end())
            return osmium::Location{};
        const auto &block = m_blocks[found->second];
        const auto suffix = static_cast<std::uint16_t>(id);
        const auto position =
            std::lower_bound(block.suffixes.begin(), block.suffixes.end(), suffix);
        if (position == block.suffixes.end() || *position != suffix)
            return osmium::Location{};
        return block.at(position - block.suffixes.begin());
    }

    osmium::Location get(ID id) const final
    {
        const auto location = get_noexcept(id);
        if (location.is_undefined())
            throw osmium::not_found{id};
        return location;
    }

    std::size_t size() const final
    { return m_legacy_index ? m_legacy_index->size() : m_node_count; }

    std::size_t used_memory() const final
    {
        if (m_legacy_index)
            return m_legacy_index->used_memory();
        auto bytes = sizeof(*this) + m_blocks.capacity() * sizeof(Block);
        for (const auto &block : m_blocks)
            bytes += block.usedMemory();
        return bytes;
    }

    void clear() final
    {
        m_legacy_index.reset();
        releaseBlocks();
        m_last_id = 0;
    }

    void sort() final
    {
        if (m_legacy_index)
            m_legacy_index->sort();
    }
};

inline QueryNode makeQueryNode(OSMNodeID id, osmium::Location location)
{
    return QueryNode{util::toFixed(util::UnsafeFloatLongitude{location.lon()}),
                     util::toFixed(util::UnsafeFloatLatitude{location.lat()}),
                     id};
}
} // namespace osrm::extractor

#endif
