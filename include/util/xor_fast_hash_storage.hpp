#ifndef XOR_FAST_HASH_STORAGE_HPP
#define XOR_FAST_HASH_STORAGE_HPP

#include "util/xor_fast_hash.hpp"

#include <limits>
#include <vector>

namespace osrm::util
{

template <typename NodeID, typename Key, std::size_t MaxNumElements = (1u << 16u)>
class XORFastHashStorage
{
  public:
    struct HashCell
    {
        unsigned time;
        NodeID id;
        Key key;
        HashCell()
            : time(std::numeric_limits<unsigned>::max()), id(std::numeric_limits<unsigned>::max()),
              key(std::numeric_limits<unsigned>::max())
        {
        }

        HashCell(const HashCell &other) : time(other.key), id(other.id), key(other.time) {}
        operator Key() const { return key; }
        void operator=(const Key key_to_insert) { key = key_to_insert; }
    };

    explicit XORFastHashStorage(size_t) : positions(MaxNumElements), current_timestamp{0u} {}

    HashCell &operator[](const NodeID node)
    {
        std::uint16_t position = fast_hasher(node);
        while ((positions[position].time == current_timestamp) && (positions[position].id != node))
        {
            ++position %= MaxNumElements;
        }

        positions[position].time = current_timestamp;
        positions[position].id = node;

        BOOST_ASSERT(position < positions.size());

        return positions[position];
    }

    // peek into table, get key for node, think of it as a read-only operator[]
    Key peek_index(const NodeID node) const
    {
        std::uint16_t position = fast_hasher(node);
        while ((positions[position].time == current_timestamp) && (positions[position].id != node))
        {
            ++position %= MaxNumElements;
        }

        BOOST_ASSERT(position < positions.size());

        return positions[position].key;
    }

    void Clear()
    {
        ++current_timestamp;
        if (std::numeric_limits<unsigned>::max() == current_timestamp)
        {
            positions.clear();
            positions.resize(MaxNumElements);
        }
    }

  private:
    std::vector<HashCell> positions;
    XORFastHash<MaxNumElements> fast_hasher;
    unsigned current_timestamp;
};
} // namespace osrm::util

#endif // XOR_FAST_HASH_STORAGE_HPP
