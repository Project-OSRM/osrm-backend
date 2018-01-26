#ifndef UNPACKING_STATISTICS_HPP
#define UNPACKING_STATISTICS_HPP

#include "util/typedefs.hpp"

#include <utility>
#include <unordered_map>

namespace std
{
	template<> struct hash<std::pair<NodeID, NodeID>>
    {
        typedef std::pair<NodeID, NodeID> argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& pair) const noexcept
        {
            result_type const h1 ( std::hash<unsigned int>{}(pair.first) );
            result_type const h2 ( std::hash<unsigned int>{}(pair.second) );
            return h1 ^ (h2 << 1); // or use boost::hash_combine (see Discussion)
        }
    };

}
namespace osrm
{
namespace engine
{
class UnpackingStatistics
{
	std::pair<NodeID, NodeID> edge;
	unsigned number_of_nodes;


	std::unordered_map<std::pair<NodeID, NodeID>, int> cache;
	int number_of_lookups;
	int number_of_finds;
	int number_of_misses;

  public:
    UnpackingStatistics(unsigned number_of_nodes) :
		number_of_nodes(number_of_nodes), number_of_lookups(0), number_of_finds(0), number_of_misses(0) {}
    // UnpackingStatistics(std::pair<NodeID, NodeID> edge) : edge(edge) {}

    // UnpackingStatistics() : edge(std::make_pair(SPECIAL_NODEID, SPECIAL_NODEID)) {}

    void Clear()
    {
        cache.clear();
        number_of_lookups = 0;
        number_of_finds = 0;
        number_of_misses = 0;
    }

    void CollectStats(std::pair<NodeID, NodeID> edge)
    {

		// check if edge is in the map
		// if edge is in the map :
		// 		- increment number_of_lookups, number_of_finds
		// 		- update cache with edge:number_of_times to edge:number_of_times++
		// if edge is not in map:
		//		- increment number_of_lookups, number_of_misses
		//		- insert edge into map with value 1

        number_of_lookups = number_of_lookups + 1;

        if (cache.find(edge) == cache.end()) {
			++number_of_misses;
        } else {
			++number_of_finds;
        }
        ++cache[edge];

        std::cout << "Misses :" << number_of_misses << " Finds: " << number_of_finds << " Total Lookups So Far: " << number_of_lookups << std::endl;
    }

    void PrintStats()
    {
        std::cout << "Total Misses :" << number_of_misses << " Total Finds: " << number_of_finds << " Total Lookups: " << number_of_lookups << std::endl;
    }

    void PrintEdgeLookups(std::pair<NodeID, NodeID> edge)
    {
    	if (cache.find(edge) == cache.end()) {

    	}
        std::cout << "{I'm heeeear}" << std::endl;
    }
};
} // engine
} // osrm

#endif // UNPACKING_STATISTICS_HPP