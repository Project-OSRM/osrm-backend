#ifndef OSRM_ENGINE_ROUTING_ALGORITHM_HPP
#define OSRM_ENGINE_ROUTING_ALGORITHM_HPP

#include "engine/algorithm.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/routing_algorithms/alternative_path.hpp"
#include "engine/routing_algorithms/direct_shortest_path.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/map_matching.hpp"
#include "engine/routing_algorithms/shortest_path.hpp"
#include "engine/routing_algorithms/tile_turns.hpp"

namespace osrm
{
namespace engine
{

class RoutingAlgorithmsInterface
{
  public:
    virtual InternalRouteResult
    AlternativePathSearch(const PhantomNodes &phantom_node_pair) const = 0;

    virtual InternalRouteResult
    ShortestPathSearch(const std::vector<PhantomNodes> &phantom_node_pair,
                       const boost::optional<bool> continue_straight_at_waypoint) const = 0;

    virtual InternalRouteResult
    DirectShortestPathSearch(const PhantomNodes &phantom_node_pair) const = 0;

    virtual std::vector<EdgeWeight>
    ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                     const std::vector<std::size_t> &source_indices,
                     const std::vector<std::size_t> &target_indices) const = 0;

    virtual routing_algorithms::SubMatchingList
    MapMatching(const routing_algorithms::CandidateLists &candidates_list,
                const std::vector<util::Coordinate> &trace_coordinates,
                const std::vector<unsigned> &trace_timestamps,
                const std::vector<boost::optional<double>> &trace_gps_precision) const = 0;

    virtual std::vector<routing_algorithms::TurnData>
    GetTileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
                 const std::vector<std::size_t> &sorted_edge_indexes) const = 0;

    virtual bool HasAlternativePathSearch() const = 0;
    virtual bool HasShortestPathSearch() const = 0;
    virtual bool HasDirectShortestPathSearch() const = 0;
    virtual bool HasMapMatching() const = 0;
    virtual bool HasManyToManySearch() const = 0;
    virtual bool HasGetTileTurns() const = 0;
};

// Short-lived object passed to each plugin in request to wrap routing algorithms
template <typename AlgorithmT> class RoutingAlgorithms final : public RoutingAlgorithmsInterface
{
  public:
    RoutingAlgorithms(SearchEngineData &heaps,
                      const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &facade)
        : heaps(heaps), facade(facade)
    {
    }

    virtual ~RoutingAlgorithms() = default;

    InternalRouteResult
    AlternativePathSearch(const PhantomNodes &phantom_node_pair) const final override;

    InternalRouteResult ShortestPathSearch(
        const std::vector<PhantomNodes> &phantom_node_pair,
        const boost::optional<bool> continue_straight_at_waypoint) const final override;

    InternalRouteResult
    DirectShortestPathSearch(const PhantomNodes &phantom_nodes) const final override;

    std::vector<EdgeWeight>
    ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                     const std::vector<std::size_t> &source_indices,
                     const std::vector<std::size_t> &target_indices) const final override;

    routing_algorithms::SubMatchingList MapMatching(
        const routing_algorithms::CandidateLists &candidates_list,
        const std::vector<util::Coordinate> &trace_coordinates,
        const std::vector<unsigned> &trace_timestamps,
        const std::vector<boost::optional<double>> &trace_gps_precision) const final override;

    std::vector<routing_algorithms::TurnData>
    GetTileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
                 const std::vector<std::size_t> &sorted_edge_indexes) const final override;

    bool HasAlternativePathSearch() const final override
    {
        return algorithm_trais::HasAlternativePathSearch<AlgorithmT>::value;
    }

    bool HasShortestPathSearch() const final override
    {
        return algorithm_trais::HasShortestPathSearch<AlgorithmT>::value;
    }

    bool HasDirectShortestPathSearch() const final override
    {
        return algorithm_trais::HasDirectShortestPathSearch<AlgorithmT>::value;
    }

    bool HasMapMatching() const final override
    {
        return algorithm_trais::HasMapMatching<AlgorithmT>::value;
    }

    bool HasManyToManySearch() const final override
    {
        return algorithm_trais::HasManyToManySearch<AlgorithmT>::value;
    }

    bool HasGetTileTurns() const final override
    {
        return algorithm_trais::HasGetTileTurns<AlgorithmT>::value;
    }

  private:
    SearchEngineData &heaps;
    // Owned by shared-ptr passed to the query
    const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &facade;
};

template <typename AlgorithmT>
InternalRouteResult
RoutingAlgorithms<AlgorithmT>::AlternativePathSearch(const PhantomNodes &phantom_node_pair) const
{
    return routing_algorithms::alternativePathSearch(heaps, facade, phantom_node_pair);
}

template <typename AlgorithmT>
InternalRouteResult RoutingAlgorithms<AlgorithmT>::ShortestPathSearch(
    const std::vector<PhantomNodes> &phantom_node_pair,
    const boost::optional<bool> continue_straight_at_waypoint) const
{
    return routing_algorithms::shortestPathSearch(
        heaps, facade, phantom_node_pair, continue_straight_at_waypoint);
}

template <typename AlgorithmT>
InternalRouteResult
RoutingAlgorithms<AlgorithmT>::DirectShortestPathSearch(const PhantomNodes &phantom_nodes) const
{
    return routing_algorithms::directShortestPathSearch(heaps, facade, phantom_nodes);
}

template <typename AlgorithmT>
std::vector<EdgeWeight> RoutingAlgorithms<AlgorithmT>::ManyToManySearch(
    const std::vector<PhantomNode> &phantom_nodes,
    const std::vector<std::size_t> &source_indices,
    const std::vector<std::size_t> &target_indices) const
{
    return routing_algorithms::manyToManySearch(
        heaps, facade, phantom_nodes, source_indices, target_indices);
}

template <typename AlgorithmT>
inline routing_algorithms::SubMatchingList RoutingAlgorithms<AlgorithmT>::MapMatching(
    const routing_algorithms::CandidateLists &candidates_list,
    const std::vector<util::Coordinate> &trace_coordinates,
    const std::vector<unsigned> &trace_timestamps,
    const std::vector<boost::optional<double>> &trace_gps_precision) const
{
    return routing_algorithms::mapMatching(
        heaps, facade, candidates_list, trace_coordinates, trace_timestamps, trace_gps_precision);
}

template <typename AlgorithmT>
inline std::vector<routing_algorithms::TurnData> RoutingAlgorithms<AlgorithmT>::GetTileTurns(
    const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
    const std::vector<std::size_t> &sorted_edge_indexes) const
{
    return routing_algorithms::getTileTurns(facade, edges, sorted_edge_indexes);
}

// MLD overrides for not implemented
template <>
InternalRouteResult inline RoutingAlgorithms<algorithm::MLD>::AlternativePathSearch(
    const PhantomNodes &) const
{
    throw util::exception("AlternativePathSearch is not implemented");
}

template <>
inline InternalRouteResult
RoutingAlgorithms<algorithm::MLD>::ShortestPathSearch(const std::vector<PhantomNodes> &,
                                                      const boost::optional<bool>) const
{
    throw util::exception("ShortestPathSearch is not implemented");
}

template <>
InternalRouteResult inline RoutingAlgorithms<algorithm::MLD>::DirectShortestPathSearch(
    const PhantomNodes &) const
{
    throw util::exception("DirectShortestPathSearch is not implemented");
}

template <>
inline std::vector<EdgeWeight>
RoutingAlgorithms<algorithm::MLD>::ManyToManySearch(const std::vector<PhantomNode> &,
                                                    const std::vector<std::size_t> &,
                                                    const std::vector<std::size_t> &) const
{
    throw util::exception("ManyToManySearch is not implemented");
}

template <>
inline routing_algorithms::SubMatchingList
RoutingAlgorithms<algorithm::MLD>::MapMatching(const routing_algorithms::CandidateLists &,
                                               const std::vector<util::Coordinate> &,
                                               const std::vector<unsigned> &,
                                               const std::vector<boost::optional<double>> &) const
{
    throw util::exception("MapMatching is not implemented");
}

template <>
inline std::vector<routing_algorithms::TurnData> RoutingAlgorithms<algorithm::MLD>::GetTileTurns(
    const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &,
    const std::vector<std::size_t> &) const
{
    throw util::exception("GetTileTurns is not implemented");
}
}
}

#endif
