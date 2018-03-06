#ifndef UNPACKING_CACHE_HPP
#define UNPACKING_CACHE_HPP

#include "util/typedefs.hpp"

#include <unordered_map>
#include <utility>

namespace std
{
template <> struct hash<std::pair<NodeID, NodeID>>
{
    typedef std::pair<NodeID, NodeID> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const &pair) const noexcept
    {
        result_type const h1(std::hash<unsigned int>{}(pair.first));
        result_type const h2(std::hash<unsigned int>{}(pair.second));
        return h1 ^ (h2 << 1); // or use boost::hash_combine (see Discussion)
    }
};
}
namespace osrm
{
namespace engine
{
class UnpackingCache
{
    std::unordered_map<std::pair<NodeID, NodeID>, EdgeDuration> cache;

  public:
    UnpackingCache(){};

    void Clear() { cache.clear(); }

    bool IsEdgeInCache(std::pair<NodeID, NodeID> edge)
    {
        bool edge_is_in_cache = cache.find(edge) != cache.end();
        return edge_is_in_cache;
    }

    void AddEdge(std::pair<NodeID, NodeID> edge, EdgeDuration duration)
    {
        cache.insert({edge, duration});
        GetDuration(edge);
    }

    EdgeDuration GetDuration(std::pair<NodeID, NodeID> edge)
    {
        EdgeDuration duration = cache[edge];
        return duration;
    }
};
} // engine
} // osrm

#endif // UNPACKING_CACHE_HPP