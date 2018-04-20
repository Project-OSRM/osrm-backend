#ifndef UNPACKING_CACHE_HPP
#define UNPACKING_CACHE_HPP

#include <boost/optional/optional_io.hpp>
#include <boost/thread.hpp>

#include "../../third_party/compute_detail/lru_cache.hpp"
#include "util/typedefs.hpp"

// sizeof size_t: 8
// sizeof unsigned: 4
// sizeof unchar: 1
// sizeof uint32: 4
namespace osrm
{
namespace engine
{
typedef unsigned char ExcludeIndex;
typedef unsigned Timestamp;
typedef std::tuple<NodeID, NodeID, ExcludeIndex, Timestamp> Key;

class UnpackingCache
{
  private:
    boost::compute::detail::lru_cache<Key, EdgeDuration> m_cache;
    boost::shared_mutex m_shared_access;

  public:
    // TO FIGURE OUT HOW MANY LINES TO INITIALIZE CACHE TO:
    // Assume max cache size is 500mb (see bottom of OP here:
    // https://github.com/Project-OSRM/osrm-backend/issues/4798#issue-288608332)

    // LRU CACHE IMPLEMENTATION HAS THESE TWO STORAGE CONTAINERS
    // Key is of size: std::uint32_t * 2 + (unsigned char) * 1
    //                = 4 * 2 + 1 * 1 = 9
    // map: n * Key + n * EdgeDuration
    //    = n * 9 bytes + n * std::int32_t
    //    = n * 9 bytes + n * 4 bytes
    //    = n * 13 bytes
    // list: n * Key
    //     = n * 9 bytes
    // Total = n * (13 + 9) = n * 22 bytes
    // Total cache size: 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes
    // Total cache size: 1024 mb = 1024 * 1024 *1024 bytes = 1073741824 bytes
    // Total cache size: 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes

    // THREAD LOCAL STORAGE (500 mb)
    // Number of lines we need  = 524288000 / 22 / number of threads = 23831272 / number of threads
    // 16 threads: 23831272 / 16 = 1489454
    // 8 threads: 23831272 / 8 = 2978909
    // 4 threads: 23831272 / 4 = 5957818
    // 2 threads: 23831272 / 2 = 11915636

    // THREAD LOCAL STORAGE (1024 mb)
    // Number of lines we need  = 1073741824 / 22 / number of threads = 48806446 / number of threads
    // 16 threads: 48806446 / 16 = 3050402
    // 8 threads: 48806446 / 8 = 6100805
    // 4 threads: 48806446 / 4 = 12201611
    // 2 threads: 48806446 / 2 = 24403223

    // LRU CACHE IMPLEMENTATION HAS THESE TWO STORAGE CONTAINERS
    // Key is of size: std::uint32_t * 2 + (unsigned char) * 1 + unsigned * 1
    //                = 4 * 2 + 1 * 1 + 4 * 1 =  13
    // map: n * Key + n * EdgeDuration
    //    = n * 13 bytes + n * std::int32_t
    //    = n * 13 bytes + n * 4 bytes
    //    = n * 17 bytes
    // list: n * Key
    //     = n * 13 bytes
    // Total = n * (17 + 13) = n * 30 bytes
    // Total cache size: 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes
    // Total cache size: 1024 mb = 1024 * 1024 *1024 bytes = 1073741824 bytes
    // Total cache size: 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes

    // SHARED STORAGE CACHE
    // Number of lines for shared storage cache 1024 mb = 524288000 / 30 = 17476266
    // Number of lines for shared storage cache 500 mb = 1073741824 / 30 = 35791394
    // Number of lines for shared storage cache 250 mb = 11397565 / 30 = 379918

    UnpackingCache() : m_cache(379918){};

    bool IsEdgeInCache(Key edge)
    {
        boost::shared_lock<boost::shared_mutex> lock(m_shared_access);
        return m_cache.contains(edge);
    }

    void AddEdge(Key edge, EdgeDuration duration)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_shared_access);
        m_cache.insert(edge, duration);
    }

    EdgeDuration GetDuration(Key edge)
    {
        boost::shared_lock<boost::shared_mutex> lock(m_shared_access);
        boost::optional<EdgeDuration> duration = m_cache.get(edge);
        return duration ? *duration : MAXIMAL_EDGE_DURATION;
    }
};
} // engine
} // osrm

#endif // UNPACKING_CACHE_HPP