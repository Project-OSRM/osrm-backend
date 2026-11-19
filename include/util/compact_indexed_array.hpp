#ifndef OSRM_UTIL_COMPACT_INDEXED_ARRAY_HPP
#define OSRM_UTIL_COMPACT_INDEXED_ARRAY_HPP

#include <boost/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace osrm::util
{
template <typename Storage, typename NodeID, typename Key> class IndexStorageReference
{
  public:
    IndexStorageReference(Storage &storage, NodeID node) : m_storage(storage), m_node(node) {}
    IndexStorageReference(const IndexStorageReference &) = default;

    IndexStorageReference &operator=(const IndexStorageReference &other)
    { return *this = static_cast<Key>(other); }

    IndexStorageReference &operator=(Key value)
    {
        m_storage.setIndex(m_node, value);
        return *this;
    }

    operator Key() const { return m_storage.peek_index(m_node); }

  private:
    Storage &m_storage;
    NodeID m_node;
};

template <typename NodeID, typename Key> class CompactArrayStorage
{
    static_assert(std::is_integral_v<Key> && sizeof(Key) <= sizeof(std::uint64_t));
    static_assert(std::numeric_limits<Key>::max() > 65535);
    static constexpr std::uint16_t m_overflow_flag = 65535;
    static constexpr std::size_t m_block_size = 512;

  public:
    explicit CompactArrayStorage(std::size_t size)
        : m_positions(size ? std::make_unique_for_overwrite<std::uint16_t[]>(size) : nullptr),
          m_size(size), m_initialized(size / m_block_size + (size % m_block_size != 0), 0)
    {
    }

    IndexStorageReference<CompactArrayStorage, NodeID, Key> operator[](NodeID node)
    { return {*this, node}; }

    Key operator[](NodeID node) const { return peek_index(node); }

    Key peek_index(NodeID node) const
    {
        if (!m_positions)
        {
            BOOST_ASSERT(node < m_wide.size());
            return m_wide[node];
        }
        BOOST_ASSERT(node < m_size);
        if (!m_initialized[node / m_block_size])
            return 0;
        const auto position = m_positions[node];
        if (position != m_overflow_flag)
            return position;
        const auto found = m_overflow.find(node);
        return found == m_overflow.end() ? std::numeric_limits<Key>::max() : found->second;
    }

    void setIndex(NodeID node, Key value)
    {
        if (!m_positions)
        {
            BOOST_ASSERT(node < m_wide.size());
            m_wide[node] = value;
            return;
        }
        BOOST_ASSERT(node < m_size);
        const auto block = node / m_block_size;
        if (!m_initialized[block])
        {
            const auto begin = block * m_block_size;
            std::fill_n(m_positions.get() + begin, std::min(m_block_size, m_size - begin), 0);
            m_initialized[block] = 1;
        }
        if (static_cast<std::uint64_t>(value) < m_overflow_flag)
        {
            m_positions[node] = static_cast<std::uint16_t>(value);
            return;
        }
        // Estimate node and bucket overhead when choosing the dense fallback.
        constexpr std::size_t overflow_entry_bytes = 64;
        if (m_overflow.size() >= m_size * sizeof(std::uint16_t) / overflow_entry_bytes)
        {
            promote();
            m_wide[node] = value;
            return;
        }
        m_overflow[node] = value;
        m_positions[node] = m_overflow_flag;
    }

    void Clear() { m_overflow.clear(); }

  private:
    void promote()
    {
        std::vector<Key> wide;
        wide.reserve(m_size);
        for (std::size_t node = 0; node < m_size; ++node)
            wide.push_back(peek_index(static_cast<NodeID>(node)));
        m_wide.swap(wide);
        m_positions.reset();
        std::vector<std::uint8_t>().swap(m_initialized);
        std::unordered_map<NodeID, Key>().swap(m_overflow);
    }

    std::unique_ptr<std::uint16_t[]> m_positions;
    std::size_t m_size;
    std::vector<std::uint8_t> m_initialized;
    std::unordered_map<NodeID, Key> m_overflow;
    std::vector<Key> m_wide;
};
} // namespace osrm::util

#endif
