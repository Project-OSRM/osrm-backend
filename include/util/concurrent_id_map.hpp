#ifndef CONCURRENT_ID_MAP_HPP
#define CONCURRENT_ID_MAP_HPP

#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace osrm::util
{

/**
 * This is a special purpose map for caching incrementing IDs
 */
template <typename KeyType, typename ValueType, typename HashType = std::hash<KeyType>>
struct ConcurrentIDMap
{
    static_assert(std::is_unsigned<ValueType>::value, "Only unsigned integer types are supported.");

    using UpgradableMutex = boost::interprocess::interprocess_upgradable_mutex;
    using ScopedReaderLock = boost::interprocess::sharable_lock<UpgradableMutex>;
    using ScopedWriterLock = boost::interprocess::scoped_lock<UpgradableMutex>;

    std::unordered_map<KeyType, ValueType, HashType> data;
    mutable UpgradableMutex mutex;

    ConcurrentIDMap() = default;
    ConcurrentIDMap(ConcurrentIDMap &&other) noexcept
    {
        if (this != &other)
        {
            ScopedWriterLock other_lock{other.mutex};
            ScopedWriterLock lock{mutex};

            data = std::move(other.data);
        }
    }
    ConcurrentIDMap &operator=(ConcurrentIDMap &&other) noexcept
    {
        if (this != &other)
        {
            ScopedWriterLock other_lock{other.mutex};
            ScopedWriterLock lock{mutex};

            data = std::move(other.data);
        }
        return *this;
    }

    const ValueType ConcurrentFindOrAdd(const KeyType &key)
    {
        {
            ScopedReaderLock sentry{mutex};
            const auto result = data.find(key);
            if (result != data.end())
            {
                return result->second;
            }
        }
        {
            ScopedWriterLock sentry{mutex};
            const auto result = data.find(key);
            if (result != data.end())
            {
                return result->second;
            }
            const auto id = static_cast<ValueType>(data.size());
            data[key] = id;
            return id;
        }
    }

    // Reassigns IDs in key order, IDs below first_id stay. Returns old to new ID.
    // Not thread safe, call after all concurrent insertion is done.
    template <typename KeyMap, typename Less>
    std::vector<ValueType> Renumber(const ValueType first_id, KeyMap key_map, Less less)
    {
        std::vector<std::pair<KeyType, ValueType>> items;
        items.reserve(data.size());
        for (const auto &pair : data)
        {
            items.emplace_back(key_map(pair.first), pair.second);
        }
        std::sort(items.begin(),
                  items.end(),
                  [&](const auto &a, const auto &b)
                  {
                      const bool a_fixed = a.second < first_id, b_fixed = b.second < first_id;
                      if (a_fixed || b_fixed)
                          return a_fixed && (!b_fixed || a.second < b.second);
                      return less(a.first, b.first);
                  });
        std::vector<ValueType> old_to_new(items.size());
        data.clear();
        for (std::size_t i = 0; i < items.size(); ++i)
        {
            old_to_new[items[i].second] = static_cast<ValueType>(i);
            data.emplace(std::move(items[i].first), static_cast<ValueType>(i));
        }
        return old_to_new;
    }
};

} // namespace osrm::util

#endif // CONCURRENT_ID_MAP_HPP
