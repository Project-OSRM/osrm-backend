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
    AlternativePathSearch(const PhantomNodes &phantom_node_pair) const final override
    {
        return routing_algorithms::alternativePathSearch(heaps, facade, phantom_node_pair);
    }

    InternalRouteResult ShortestPathSearch(
        const std::vector<PhantomNodes> &phantom_node_pair,
        const boost::optional<bool> continue_straight_at_waypoint) const final override
    {
        return routing_algorithms::shortestPathSearch(
            heaps, facade, phantom_node_pair, continue_straight_at_waypoint);
    }

    InternalRouteResult
    DirectShortestPathSearch(const PhantomNodes &phantom_nodes) const final override
    {
        return routing_algorithms::directShortestPathSearch(heaps, facade, phantom_nodes);
    }

    std::vector<EdgeWeight>
    ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                     const std::vector<std::size_t> &source_indices,
                     const std::vector<std::size_t> &target_indices) const final override
    {
        return routing_algorithms::manyToManySearch(
            heaps, facade, phantom_nodes, source_indices, target_indices);
    }

    routing_algorithms::SubMatchingList MapMatching(
        const routing_algorithms::CandidateLists &candidates_list,
        const std::vector<util::Coordinate> &trace_coordinates,
        const std::vector<unsigned> &trace_timestamps,
        const std::vector<boost::optional<double>> &trace_gps_precision) const final override
    {
        return routing_algorithms::mapMatching(heaps,
                                               facade,
                                               candidates_list,
                                               trace_coordinates,
                                               trace_timestamps,
                                               trace_gps_precision);
    }

    std::vector<routing_algorithms::TurnData>
    GetTileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
                 const std::vector<std::size_t> &sorted_edge_indexes) const final override
    {
        return routing_algorithms::getTileTurns(facade, edges, sorted_edge_indexes);
    }

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
}
}

#endif
