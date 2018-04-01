#ifndef UNPACKING_CACHE_HPP
#define UNPACKING_CACHE_HPP

#include <boost/optional/optional_io.hpp>
#include <boost/thread.hpp>

#include "../../third_party/compute_detail/lru_cache.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace engine
{
class UnpackingCache
{
  private:
    boost::compute::detail::lru_cache<std::tuple<NodeID, NodeID, std::size_t>, EdgeDuration>
        m_cache;
    unsigned m_current_data_timestamp = 0;
    std::mutex m_mutex;
    boost::shared_mutex m_shared_access;

  public:
    UnpackingCache(unsigned timestamp) : m_cache(6000000), m_current_data_timestamp(timestamp){};

    void Clear(unsigned new_data_timestamp)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_shared_access);
        if (m_current_data_timestamp != new_data_timestamp)
        {
            m_cache.clear();
            m_current_data_timestamp = new_data_timestamp;
        }
    }

    bool IsEdgeInCache(std::tuple<NodeID, NodeID, std::size_t> edge)
    {
        boost::shared_lock<boost::shared_mutex> lock(m_shared_access);
        return m_cache.contains(edge);
    }

    void AddEdge(std::tuple<NodeID, NodeID, std::size_t> edge, EdgeDuration duration)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_shared_access);
        std::lock_guard<std::mutex> guard(m_mutex);
        m_cache.insert(edge, duration);
    }

    EdgeDuration GetDuration(std::tuple<NodeID, NodeID, std::size_t> edge)
    {
        boost::shared_lock<boost::shared_mutex> lock(m_shared_access);
        boost::optional<EdgeDuration> duration = m_cache.get(edge);
        return *duration ? *duration : MAXIMAL_EDGE_DURATION;
    }
};
} // engine
} // osrm

#endif // UNPACKING_CACHE_HPP
