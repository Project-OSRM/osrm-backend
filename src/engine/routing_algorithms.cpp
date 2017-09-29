#include "engine/routing_algorithms.hpp"

namespace osrm
{
namespace engine
{

template <typename Algorithm>
std::vector<EdgeDuration>
RoutingAlgorithms<Algorithm>::ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                                               const std::vector<std::size_t> &source_indices,
                                               const std::vector<std::size_t> &target_indices) const
{
    return routing_algorithms::manyToManySearch(
        heaps, *facade, phantom_nodes, source_indices, target_indices);
}

template std::vector<EdgeDuration>
RoutingAlgorithms<routing_algorithms::ch::Algorithm>::ManyToManySearch(
    const std::vector<PhantomNode> &phantom_nodes,
    const std::vector<std::size_t> &source_indices,
    const std::vector<std::size_t> &target_indices) const;

template std::vector<EdgeDuration>
RoutingAlgorithms<routing_algorithms::corech::Algorithm>::ManyToManySearch(
    const std::vector<PhantomNode> &phantom_nodes,
    const std::vector<std::size_t> &source_indices,
    const std::vector<std::size_t> &target_indices) const;

// One-to-many and many-to-one can be handled with MLD separately from many-to-many search.
// One-to-many (many-to-one) search is a unidirectional forward (backward) Dijkstra search
// with the candidate node level min(GetQueryLevel(phantom_node, phantom_nodes, node)
template <>
std::vector<EdgeDuration> RoutingAlgorithms<routing_algorithms::mld::Algorithm>::ManyToManySearch(
    const std::vector<PhantomNode> &phantom_nodes,
    const std::vector<std::size_t> &source_indices,
    const std::vector<std::size_t> &target_indices) const
{
    if (source_indices.size() == 1)
    { // TODO: check if target_indices.size() == 1 and do a bi-directional search
        return routing_algorithms::mld::oneToManySearch<routing_algorithms::FORWARD_DIRECTION>(
            heaps, *facade, phantom_nodes, source_indices.front(), target_indices);
    }

    if (target_indices.size() == 1)
    {
        return routing_algorithms::mld::oneToManySearch<routing_algorithms::REVERSE_DIRECTION>(
            heaps, *facade, phantom_nodes, target_indices.front(), source_indices);
    }

    return routing_algorithms::manyToManySearch(
        heaps, *facade, phantom_nodes, source_indices, target_indices);
}

} // namespace engine
} // namespace osrm
