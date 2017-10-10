#ifndef MANY_TO_MANY_ROUTING_HPP
#define MANY_TO_MANY_ROUTING_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace
{
struct NodeBucket
{
    NodeID middle_node;
    unsigned column_index; // a column in the weight/duration matrix
    EdgeWeight weight;
    EdgeDuration duration;

    NodeBucket(NodeID middle_node, unsigned column_index, EdgeWeight weight, EdgeDuration duration)
        : middle_node(middle_node), column_index(column_index), weight(weight), duration(duration)
    {
    }

    // partial order comparison
    bool operator<(const NodeBucket &rhs) const { return middle_node < rhs.middle_node; }

    // functor for equal_range
    struct Compare
    {
        bool operator()(const NodeBucket &lhs, const NodeID &rhs) const
        {
            return lhs.middle_node < rhs;
        }

        bool operator()(const NodeID &lhs, const NodeBucket &rhs) const
        {
            return lhs < rhs.middle_node;
        }
    };
};
}

template <typename Algorithm>
std::vector<EdgeDuration> manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                                           const DataFacade<Algorithm> &facade,
                                           const std::vector<PhantomNode> &phantom_nodes,
                                           const std::vector<std::size_t> &source_indices,
                                           const std::vector<std::size_t> &target_indices);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
