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
typedef std::tuple<NodeID, NodeID, unsigned char> Key;

class UnpackingCache
{
  private:
    boost::compute::detail::lru_cache<std::tuple<NodeID, NodeID, unsigned char>, EdgeDuration> m_cache;
    unsigned m_current_data_timestamp = 0;

  public:
    // TO FIGURE OUT HOW MANY LINES TO INITIALIZE CACHE TO:
    // Assume max cache size is 500mb (see bottom of OP here:
    // https://github.com/Project-OSRM/osrm-backend/issues/4798#issue-288608332)

    // LRU CACHE IMPLEMENTATION HAS THESE TWO STORAGE CONTAINERS
    // Key is of size: std::uint32_t * 2 + (unsigned char) * 1 + unsigned * 1
    //                = 4 * 2 + 1 * 1 + 4 * 1 =  21
    // map: n * Key + n * EdgeDuration
    //    = n * 21 bytes + n * std::int32_t
    //    = n * 21 bytes + n * 4 bytes
    //    = n * 25 bytes
    // list: n * Key
    //     = n * 21 bytes
    // Total = n * (25 + 21) = n * 46 bytes
    // Total cache size: 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes
    // Total cache size: 1024 mb = 1024 * 1024 *1024 bytes = 1073741824 bytes
    // Total cache size: 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes

    // THREAD LOCAL STORAGE (500 mb)
    // Number of lines we need  = 524288000 / 46 / number of threads = 11397565 / number of threads
    // 16 threads: 11397565 / 16 = 712347
    // 8 threads: 11397565 / 8 = 1424695
    // 4 threads: 11397565 / 4 = 2849391
    // 2 threads: 11397565 / 2 = 5698782

    // THREAD LOCAL STORAGE (1024 mb)
    // Number of lines we need  = 1073741824 / 46 / number of threads = 23342213 / number of threads
    // 16 threads: 23342213 / 16 = 1458888
    // 8 threads: 23342213 / 8 = 2917776
    // 4 threads: 23342213 / 4 = 5835553
    // 2 threads: 23342213 / 2 = 11671106

    // SHARED STORAGE CACHE
    // Number of lines we need for shared storage cache = 524288000 / 20 = 26214400

    UnpackingCache(unsigned timestamp) : m_cache(11671106), m_current_data_timestamp(timestamp){};

    UnpackingCache(std::size_t cache_size, unsigned timestamp)
        : m_cache(cache_size), m_current_data_timestamp(timestamp){};

    void Clear(unsigned new_data_timestamp)
    {
        if (m_current_data_timestamp != new_data_timestamp)
        {
            m_cache.clear();
            m_current_data_timestamp = new_data_timestamp;
        }
    }

    bool IsEdgeInCache(std::tuple<NodeID, NodeID, unsigned char> edge)
    {
        return m_cache.contains(edge);
    }

    void AddEdge(std::tuple<NodeID, NodeID, unsigned char> edge, EdgeDuration duration)
    {
        m_cache.insert(edge, duration);
    }

    EdgeDuration GetDuration(std::tuple<NodeID, NodeID, unsigned char> edge)
    {
        boost::optional<EdgeDuration> duration = m_cache.get(edge);
        return duration ? *duration : MAXIMAL_EDGE_DURATION;
    }
};
} // engine
} // osrm

#endif // UNPACKING_CACHE_HPP
