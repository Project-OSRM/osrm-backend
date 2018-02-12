#ifndef UNPACKING_CACHE_HPP
#define UNPACKING_CACHE_HPP

#include <boost/optional/optional_io.hpp>

#include "../../third_party/compute_detail/lru_cache.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace engine
{
typedef unsigned char ExcludeIndex;
class UnpackingCache
{
  private:
    boost::compute::detail::lru_cache<std::tuple<NodeID, NodeID, ExcludeIndex>, EdgeDuration>
        m_cache;
    unsigned m_current_data_timestamp = 0;

  public:
    // TO FIGURE OUT HOW MANY LINES TO INITIALIZE CACHE TO:
    // Assume max cache size is 500mb (see bottom of OP here:
    // https://github.com/Project-OSRM/osrm-backend/issues/4798#issue-288608332)
    // Total cache size: 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes
    // Assume unsigned char is 1 byte (my local machine this is the case):
    // Current cache line = NodeID * 2 + unsigned char * 1 + EdgeDuration * 1
    //                    = std::uint32_t * 2 + unsigned char * 1 + std::int32_t * 1
    //                    = 4 bytes * 3 + 1 byte = 13 bytes
    // Number of cache lines is 500 mb = 500 * 1024 *1024 bytes = 524288000 bytes / 13 = 40329846
    // For threadlocal cache, Number of cache lines = max cache size / number of threads
    //                                              (Assume that the number of threads is 16)
    //                                              = 40329846 / 16 = 2520615

    UnpackingCache(unsigned timestamp) : m_cache(2520615), m_current_data_timestamp(timestamp){};

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

    bool IsEdgeInCache(std::tuple<NodeID, NodeID, ExcludeIndex> edge)
    {
        return m_cache.contains(edge);
    }

    void AddEdge(std::tuple<NodeID, NodeID, ExcludeIndex> edge, EdgeDuration duration)
    {
        m_cache.insert(edge, duration);
    }

    EdgeDuration GetDuration(std::tuple<NodeID, NodeID, ExcludeIndex> edge)
    {
        boost::optional<EdgeDuration> duration = m_cache.get(edge);
        return duration ? *duration : MAXIMAL_EDGE_DURATION;
    }
};
} // engine
} // osrm

#endif // UNPACKING_CACHE_HPP
