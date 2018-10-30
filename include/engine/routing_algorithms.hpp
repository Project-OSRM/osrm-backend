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
    virtual InternalManyRoutesResult
    AlternativePathSearch(const PhantomNodes &phantom_node_pair,
                          unsigned number_of_alternatives) const = 0;

    virtual InternalRouteResult
    ShortestPathSearch(const std::vector<PhantomNodes> &phantom_node_pair,
                       const boost::optional<bool> continue_straight_at_waypoint) const = 0;

    virtual InternalRouteResult
    DirectShortestPathSearch(const PhantomNodes &phantom_node_pair) const = 0;

    virtual std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
    ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                     const std::vector<std::size_t> &source_indices,
                     const std::vector<std::size_t> &target_indices,
                     const bool calculate_distance) const = 0;

    virtual routing_algorithms::SubMatchingList
    MapMatching(const routing_algorithms::CandidateLists &candidates_list,
                const std::vector<util::Coordinate> &trace_coordinates,
                const std::vector<unsigned> &trace_timestamps,
                const std::vector<boost::optional<double>> &trace_gps_precision,
                const bool allow_splitting) const = 0;

    virtual std::vector<routing_algorithms::TurnData>
    GetTileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
                 const std::vector<std::size_t> &sorted_edge_indexes) const = 0;

    virtual const DataFacadeBase &GetFacade() const = 0;

    virtual bool HasAlternativePathSearch() const = 0;
    virtual bool HasShortestPathSearch() const = 0;
    virtual bool HasDirectShortestPathSearch() const = 0;
    virtual bool HasMapMatching() const = 0;
    virtual bool HasManyToManySearch() const = 0;
    virtual bool SupportsDistanceAnnotationType() const = 0;
    virtual bool HasGetTileTurns() const = 0;
    virtual bool HasExcludeFlags() const = 0;
    virtual bool IsValid() const = 0;
};

// Short-lived object passed to each plugin in request to wrap routing algorithms
template <typename Algorithm> class RoutingAlgorithms final : public RoutingAlgorithmsInterface
{
  public:
    RoutingAlgorithms(SearchEngineData<Algorithm> &heaps,
                      std::shared_ptr<const DataFacade<Algorithm>> facade)
        : heaps(heaps), facade(facade)
    {
    }

    virtual ~RoutingAlgorithms() = default;

    InternalManyRoutesResult
    AlternativePathSearch(const PhantomNodes &phantom_node_pair,
                          unsigned number_of_alternatives) const final override;

    InternalRouteResult ShortestPathSearch(
        const std::vector<PhantomNodes> &phantom_node_pair,
        const boost::optional<bool> continue_straight_at_waypoint) const final override;

    InternalRouteResult
    DirectShortestPathSearch(const PhantomNodes &phantom_nodes) const final override;

    virtual std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
    ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                     const std::vector<std::size_t> &source_indices,
                     const std::vector<std::size_t> &target_indices,
                     const bool calculate_distance) const final override;

    routing_algorithms::SubMatchingList
    MapMatching(const routing_algorithms::CandidateLists &candidates_list,
                const std::vector<util::Coordinate> &trace_coordinates,
                const std::vector<unsigned> &trace_timestamps,
                const std::vector<boost::optional<double>> &trace_gps_precision,
                const bool allow_splitting) const final override;

    std::vector<routing_algorithms::TurnData>
    GetTileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
                 const std::vector<std::size_t> &sorted_edge_indexes) const final override;

    const DataFacadeBase &GetFacade() const final override { return *facade; }

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

    bool SupportsDistanceAnnotationType() const final override
    {
        return routing_algorithms::SupportsDistanceAnnotationType<Algorithm>::value;
    }

    bool HasGetTileTurns() const final override
    {
        return routing_algorithms::HasGetTileTurns<Algorithm>::value;
    }

    bool HasExcludeFlags() const final override
    {
        return routing_algorithms::HasExcludeFlags<Algorithm>::value;
    }

    bool IsValid() const final override { return static_cast<bool>(facade); }

  private:
    SearchEngineData<Algorithm> &heaps;
    std::shared_ptr<const DataFacade<Algorithm>> facade;
};

template <typename Algorithm>
InternalManyRoutesResult
RoutingAlgorithms<Algorithm>::AlternativePathSearch(const PhantomNodes &phantom_node_pair,
                                                    unsigned number_of_alternatives) const
{
    return routing_algorithms::alternativePathSearch(
        heaps, *facade, phantom_node_pair, number_of_alternatives);
}

template <typename Algorithm>
InternalRouteResult RoutingAlgorithms<Algorithm>::ShortestPathSearch(
    const std::vector<PhantomNodes> &phantom_node_pair,
    const boost::optional<bool> continue_straight_at_waypoint) const
{
    return routing_algorithms::shortestPathSearch(
        heaps, *facade, phantom_node_pair, continue_straight_at_waypoint);
}

template <typename Algorithm>
InternalRouteResult
RoutingAlgorithms<Algorithm>::DirectShortestPathSearch(const PhantomNodes &phantom_nodes) const
{
    return routing_algorithms::directShortestPathSearch(heaps, *facade, phantom_nodes);
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
                                           *facade,
                                           candidates_list,
                                           trace_coordinates,
                                           trace_timestamps,
                                           trace_gps_precision,
                                           allow_splitting);
}

template <typename Algorithm>
std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
RoutingAlgorithms<Algorithm>::ManyToManySearch(const std::vector<PhantomNode> &phantom_nodes,
                                               const std::vector<std::size_t> &_source_indices,
                                               const std::vector<std::size_t> &_target_indices,
                                               const bool calculate_distance) const
{
    BOOST_ASSERT(!phantom_nodes.empty());

    auto source_indices = _source_indices;
    auto target_indices = _target_indices;

    if (source_indices.empty())
    {
        source_indices.resize(phantom_nodes.size());
        std::iota(source_indices.begin(), source_indices.end(), 0);
    }
    if (target_indices.empty())
    {
        target_indices.resize(phantom_nodes.size());
        std::iota(target_indices.begin(), target_indices.end(), 0);
    }

    return routing_algorithms::manyToManySearch(heaps,
                                                *facade,
                                                phantom_nodes,
                                                std::move(source_indices),
                                                std::move(target_indices),
                                                calculate_distance);
}

template <typename Algorithm>
inline std::vector<routing_algorithms::TurnData> RoutingAlgorithms<Algorithm>::GetTileTurns(
    const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
    const std::vector<std::size_t> &sorted_edge_indexes) const
{
    return routing_algorithms::getTileTurns(*facade, edges, sorted_edge_indexes);
}

} // ns engine
} // ns osrm

#endif
