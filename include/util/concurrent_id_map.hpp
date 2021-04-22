#ifndef CONCURRENT_ID_MAP_HPP
#define CONCURRENT_ID_MAP_HPP

#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include <unordered_map>

namespace osrm
{
namespace util
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
    ConcurrentIDMap(ConcurrentIDMap &&other)
    {
        if (this != &other)
        {
            ScopedWriterLock other_lock{other.mutex};
            ScopedWriterLock lock{mutex};

            data = std::move(other.data);
        }
    }
    ConcurrentIDMap &operator=(ConcurrentIDMap &&other)
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
};

} // namespace util
} // namespace osrm

#endif // CONCURRENT_ID_MAP_HPP
