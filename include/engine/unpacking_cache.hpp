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
    std::pair<NodeID, NodeID> edge;
    std::unordered_map<std::pair<NodeID, NodeID>, int> cache;
    int number_of_lookups;
    int number_of_finds;
    int number_of_misses;

  public:
    UnpackingCache() : number_of_lookups(0), number_of_finds(0), number_of_misses(0) {}

    void Clear()
    {
        cache.clear();
        number_of_lookups = 0;
        number_of_finds = 0;
        number_of_misses = 0;
    }

    bool IsEdgeInCache(std::pair<NodeID, NodeID> edge)
    {
        ++number_of_lookups;
        bool edge_is_in_cache = cache.find(edge) != cache.end();
        if (edge_is_in_cache)
        {
            std::cout << edge.first << ", " << edge.second << " true" << std::endl;
        }
        else
        {
            std::cout << edge.first << ", " << edge.second << " false" << std::endl;
        }
        return edge_is_in_cache;
    }

    // void CollectStats(std::pair<NodeID, NodeID> edge)
    // {

    //     // check if edge is in the map
    //     // if edge is in the map :
    //     // 		- increment number_of_lookups, number_of_finds
    //     // 		- update cache with edge:number_of_times to edge:number_of_times++
    //     // if edge is not in map:
    //     //		- increment number_of_lookups, number_of_misses
    //     //		- insert edge into map with value 1

    //     std::cout << "Collected Stats" << std::endl;
    //     if (cache.find(edge) == cache.end())
    //     {
    //         ++number_of_misses;
    //     }
    //     else
    //     {
    //         ++number_of_finds;
    //     }
    //     ++cache[edge];
    // }

    void PrintStats()
    {
        std::cout << "Total Misses :" << number_of_misses << " Total Finds: " << number_of_finds
                  << " Total Lookups: " << number_of_lookups << " Cache size: " << cache.size()
                  << std::endl;
        // std::cout << number_of_misses << "," << number_of_finds << "," << number_of_lookups <<
        // ","
        //           << cache.size() << std::endl;
    }

    void AddEdge(std::pair<NodeID, NodeID> edge)
    {
        ++cache[edge];
        std::cout << "Added edge: " << edge.first << ", " << edge.second << std::endl;
    }

    EdgeDuration GetDuration(std::pair<NodeID, NodeID> edge)
    {
        EdgeDuration duration = cache[edge];
        std::cout << "Duration is: " << duration << std::endl;
        return duration;
    }

    // void PrintEdgeLookups(std::pair<NodeID, NodeID> edge)
    // {
    // }
};
} // engine
} // osrm

#endif // UNPACKING_CACHE_HPP