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
                const std::vector<boost::optional<double>> &trace_gps_precision,
                const bool allow_splitting) const = 0;

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
template <typename Algorithm> class RoutingAlgorithms final : public RoutingAlgorithmsInterface
{
  public:
    RoutingAlgorithms(SearchEngineData<Algorithm> &heaps,
                      const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade)
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

    routing_algorithms::SubMatchingList
    MapMatching(const routing_algorithms::CandidateLists &candidates_list,
                const std::vector<util::Coordinate> &trace_coordinates,
                const std::vector<unsigned> &trace_timestamps,
                const std::vector<boost::optional<double>> &trace_gps_precision,
                const bool allow_splitting) const final override;

    std::vector<routing_algorithms::TurnData>
    GetTileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
                 const std::vector<std::size_t> &sorted_edge_indexes) const final override;

    bool HasAlternativePathSearch() const final override
    {
        return routing_algorithms::HasAlternativePathSearch<Algorithm>::value;
    }

    bool HasShortestPathSearch() const final override
    {
        return routing_algorithms::HasShortestPathSearch<Algorithm>::value;
    }

    bool HasDirectShortestPathSearch() const final override
    {
        return routing_algorithms::HasDirectShortestPathSearch<Algorithm>::value;
    }

    bool HasMapMatching() const final override
    {
        return routing_algorithms::HasMapMatching<Algorithm>::value;
    }

    bool HasManyToManySearch() const final override
    {
        return routing_algorithms::HasManyToManySearch<Algorithm>::value;
    }

    bool HasGetTileTurns() const final override
    {
        return routing_algorithms::HasGetTileTurns<Algorithm>::value;
    }

  private:
    SearchEngineData<Algorithm> &heaps;

    // Owned by shared-ptr passed to the query
    const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade;
};

template <typename Algorithm>
InternalRouteResult
RoutingAlgorithms<Algorithm>::AlternativePathSearch(const PhantomNodes &phantom_node_pair) const
{
    return routing_algorithms::ch::alternativePathSearch(heaps, facade, phantom_node_pair);
}

template <typename Algorithm>
InternalRouteResult RoutingAlgorithms<Algorithm>::ShortestPathSearch(
    const std::vector<PhantomNodes> &phantom_node_pair,
    const boost::optional<bool> continue_straight_at_waypoint) const
{
    return routing_algorithms::shortestPathSearch(
        heaps, facade, phantom_node_pair, continue_straight_at_waypoint);
}

template <typename Algorithm>
InternalRouteResult
RoutingAlgorithms<Algorithm>::DirectShortestPathSearch(const PhantomNodes &phantom_nodes) const
{
    return routing_algorithms::directShortestPathSearch(heaps, facade, phantom_nodes);
}

template <typename Algorithm>
std::vector<EdgeWeight>
RoutingAlgorithms<Algorithm>::ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                                               const std::vector<std::size_t> &source_indices,
                                               const std::vector<std::size_t> &target_indices) const
{
    return routing_algorithms::ch::manyToManySearch(
        heaps, facade, phantom_nodes, source_indices, target_indices);
}

template <typename Algorithm>
inline routing_algorithms::SubMatchingList RoutingAlgorithms<Algorithm>::MapMatching(
    const routing_algorithms::CandidateLists &candidates_list,
    const std::vector<util::Coordinate> &trace_coordinates,
    const std::vector<unsigned> &trace_timestamps,
    const std::vector<boost::optional<double>> &trace_gps_precision,
    const bool allow_splitting) const
{
    return routing_algorithms::mapMatching(heaps,
                                           facade,
                                           candidates_list,
                                           trace_coordinates,
                                           trace_timestamps,
                                           trace_gps_precision,
                                           allow_splitting);
}

template <typename Algorithm>
inline std::vector<routing_algorithms::TurnData> RoutingAlgorithms<Algorithm>::GetTileTurns(
    const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
    const std::vector<std::size_t> &sorted_edge_indexes) const
{
    return routing_algorithms::getTileTurns(facade, edges, sorted_edge_indexes);
}

// CoreCH overrides
template <>
InternalRouteResult inline RoutingAlgorithms<
    routing_algorithms::corech::Algorithm>::AlternativePathSearch(const PhantomNodes &) const
{
    throw util::exception("AlternativePathSearch is disabled due to performance reasons");
}

template <>
inline std::vector<EdgeWeight>
RoutingAlgorithms<routing_algorithms::corech::Algorithm>::ManyToManySearch(
    const std::vector<PhantomNode> &,
    const std::vector<std::size_t> &,
    const std::vector<std::size_t> &) const
{
    throw util::exception("ManyToManySearch is disabled due to performance reasons");
}

// MLD overrides for not implemented
template <>
InternalRouteResult inline RoutingAlgorithms<
    routing_algorithms::mld::Algorithm>::AlternativePathSearch(const PhantomNodes &) const
{
    throw util::exception("AlternativePathSearch is not implemented");
}

template <>
inline std::vector<EdgeWeight>
RoutingAlgorithms<routing_algorithms::mld::Algorithm>::ManyToManySearch(
    const std::vector<PhantomNode> &,
    const std::vector<std::size_t> &,
    const std::vector<std::size_t> &) const
{
    throw util::exception("ManyToManySearch is not implemented");
}

template <>
inline std::vector<routing_algorithms::TurnData>
RoutingAlgorithms<routing_algorithms::mld::Algorithm>::GetTileTurns(
    const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &,
    const std::vector<std::size_t> &) const
{
    throw util::exception("GetTileTurns is not implemented");
}
}
}

#endif
