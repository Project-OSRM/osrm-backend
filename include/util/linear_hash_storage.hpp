#ifndef LINEAR_HASH_STORAGE
#define LINEAR_HASH_STORAGE

#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <vector>

#include <boost/assert.hpp>

namespace osrm::util
{

/**
 * @brief A hash table using open addressing with linear probing.
 *
 * Use this table only if the key is a NodeID.
 *
 * This hash table has a fixed maximum size.  The size will always be rounded up to the
 * next power of 2.  The hash function is a simple bitwise AND with a mask (as faster
 * replacement for a modulo function). Its performance degrades with occupancy. With
 * occupancy at 50% it performs faster than std::unordered_map, at 90% it performs
 * slower than std::unordered_map.
 *
 * @tparam KeyType
 * @tparam ValueType
 *
 * For an introduction to linear probing see:
 * https://opendatastructures.org/ods-cpp/5_2_Linear_Probing.html#SECTION00923000000000000000
 */
template <typename KeyType, typename ValueType>
    requires std::unsigned_integral<KeyType>
class LinearHashStorage
{

  private:
    struct HashCell
    {
        unsigned time;
        KeyType key;
        ValueType value;
        HashCell()
            : time(std::numeric_limits<unsigned>::max()), key(std::numeric_limits<KeyType>::max()),
              value(std::numeric_limits<ValueType>::max())
        {
        }
    };

    std::vector<HashCell> cells;
    unsigned current_timestamp{0u};
    std::size_t mask;

  public:
    explicit LinearHashStorage(std::size_t size) : cells(size), mask{size - 1}
    {
        BOOST_ASSERT_MSG((size & mask) == 0, "size must be a power of 2");
    }

    ValueType &operator[](const KeyType key)
    {
        std::size_t position = key & mask;
        while ((cells[position].time == current_timestamp) && (cells[position].key != key))
        {
            ++position &= mask;
        }
        auto p = &cells[position];
        if (p->time != current_timestamp)
        {
            p->time = current_timestamp;
            p->key = key;
            p->value = std::numeric_limits<ValueType>::max();
        }
        return p->value;
    }

    // peek into table, get key for node, think of it as a read-only operator[]
    ValueType peek_index(const KeyType key) const
    {
        std::size_t position = key & mask;
        while ((cells[position].time == current_timestamp) && (cells[position].key != key))
        {
            ++position &= mask;
        }
        return cells[position].time == current_timestamp ? cells[position].value
                                                         : std::numeric_limits<ValueType>::max();
    }

    bool contains(const KeyType key) const
    {
        std::size_t position = key & mask;
        while ((cells[position].time == current_timestamp) && (cells[position].key != key))
        {
            ++position &= mask;
        }
        return cells[position].time == current_timestamp;
    }

    void Clear()
    {
        ++current_timestamp;
        if (std::numeric_limits<unsigned>::max() == current_timestamp)
        {
            cells.clear();
        }
    }
};
} // namespace osrm::util

#endif // LINEAR_HASH_STORAGE
