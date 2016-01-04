#ifndef XOR_FAST_HASH_STORAGE_HPP
#define XOR_FAST_HASH_STORAGE_HPP

#include "util/xor_fast_hash.hpp"

#include <limits>
#include <vector>

template <typename NodeID, typename Key> class XORFastHashStorage
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

    XORFastHashStorage() = delete;

    explicit XORFastHashStorage(size_t) : positions(2 << 16), current_timestamp(0) {}

    HashCell &operator[](const NodeID node)
    {
        unsigned short position = fast_hasher(node);
        while ((positions[position].time == current_timestamp) && (positions[position].id != node))
        {
            ++position %= (2 << 16);
        }

        positions[position].time = current_timestamp;
        positions[position].id = node;
        return positions[position];
    }

    // peek into table, get key for node, think of it as a read-only operator[]
    Key peek_index(const NodeID node) const
    {
        unsigned short position = fast_hasher(node);
        while ((positions[position].time == current_timestamp) && (positions[position].id != node))
        {
            ++position %= (2 << 16);
        }
        return positions[position].key;
    }

    void Clear()
    {
        ++current_timestamp;
        if (std::numeric_limits<unsigned>::max() == current_timestamp)
        {
            positions.clear();
            positions.resize(2 << 16);
        }
    }

  private:
    std::vector<HashCell> positions;
    XORFastHash fast_hasher;
    unsigned current_timestamp;
};

#endif // XOR_FAST_HASH_STORAGE_HPP
