#ifndef UNPACKING_CACHE_HPP
#define UNPACKING_CACHE_HPP

#include "util/typedefs.hpp"

#include <boost/functional/hash_fwd.hpp>
#include <unordered_map>
#include <utility>

namespace std
{
template <> struct hash<std::tuple<NodeID, NodeID, std::size_t>>
{
    typedef std::tuple<NodeID, NodeID, std::size_t> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const &tuple) const noexcept
    {
        result_type seed = 0;
        result_type const h1(std::hash<unsigned int>{}(std::get<0>(tuple)));
        result_type const h2(std::hash<unsigned int>{}(std::get<1>(tuple)));
        result_type const h3(std::hash<unsigned int>{}(std::get<2>(tuple)));

        boost::hash_combine(seed, h1);
        boost::hash_combine(seed, h2);
        boost::hash_combine(seed, h3);

        return seed;
    }
};
}
namespace osrm
{
namespace engine
{
class UnpackingCache
{
    std::unordered_map<std::tuple<NodeID, NodeID, std::size_t>, EdgeDuration> cache;

  private:
    unsigned current_data_timestamp = 0;

  public:
    UnpackingCache(unsigned timestamp) : current_data_timestamp(timestamp){};
    // UnpackingCache(){};

    void Clear(unsigned new_data_timestamp)
    {
        if (current_data_timestamp != new_data_timestamp)
        {
            cache.clear();
        }
    }

    bool IsEdgeInCache(std::tuple<NodeID, NodeID, std::size_t> edge)
    {
        bool edge_is_in_cache = cache.find(edge) != cache.end();
        return edge_is_in_cache;
    }

    void AddEdge(std::tuple<NodeID, NodeID, std::size_t> edge, EdgeDuration duration)
    {
        cache.insert({edge, duration});
        GetDuration(edge);
    }

    EdgeDuration GetDuration(std::tuple<NodeID, NodeID, std::size_t> edge)
    {
        EdgeDuration duration = cache[edge];
        return duration;
    }
};
} // engine
} // osrm

#endif // UNPACKING_CACHE_HPP