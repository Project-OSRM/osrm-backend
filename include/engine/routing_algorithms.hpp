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
    virtual void AlternativeRouting(const PhantomNodes &phantom_node_pair,
                                    InternalRouteResult &raw_route_data) const = 0;

    virtual void ShortestRouting(const std::vector<PhantomNodes> &phantom_node_pair,
                                 const boost::optional<bool> continue_straight_at_waypoint,
                                 InternalRouteResult &raw_route_data) const = 0;

    virtual void DirectShortestPathRouting(const std::vector<PhantomNodes> &phantom_node_pair,
                                           InternalRouteResult &raw_route_data) const = 0;

    virtual std::vector<EdgeWeight>
    ManyToManyRouting(const std::vector<PhantomNode> &phantom_nodes,
                      const std::vector<std::size_t> &source_indices,
                      const std::vector<std::size_t> &target_indices) const = 0;

    virtual routing_algorithms::SubMatchingList
    MapMatching(const routing_algorithms::CandidateLists &candidates_list,
                const std::vector<util::Coordinate> &trace_coordinates,
                const std::vector<unsigned> &trace_timestamps,
                const std::vector<boost::optional<double>> &trace_gps_precision) const = 0;

    virtual std::vector<routing_algorithms::TurnData>
    TileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
              const std::vector<std::size_t> &sorted_edge_indexes) const = 0;

    virtual bool HasAlternativeRouting() const = 0;
};

// Short-lived object passed to each plugin in request to wrap routing algorithms
template <typename AlgorithmT> class RoutingAlgorithms final : public RoutingAlgorithmsInterface
{
  public:
    RoutingAlgorithms(SearchEngineData &heaps,
                      const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &facade)
        : facade(facade), alternative_routing(heaps), shortest_path_routing(heaps),
          direct_shortest_path_routing(heaps), many_to_many_routing(heaps), map_matching(heaps)
    {
    }

    void AlternativeRouting(const PhantomNodes &phantom_node_pair,
                            InternalRouteResult &raw_route_data) const final override
    {
        alternative_routing(facade, phantom_node_pair, raw_route_data);
    }

    void ShortestRouting(const std::vector<PhantomNodes> &phantom_node_pair,
                         const boost::optional<bool> continue_straight_at_waypoint,
                         InternalRouteResult &raw_route_data) const final override
    {
        shortest_path_routing(
            facade, phantom_node_pair, continue_straight_at_waypoint, raw_route_data);
    }

    void DirectShortestPathRouting(const std::vector<PhantomNodes> &phantom_node_pair,
                                   InternalRouteResult &raw_route_data) const final override
    {
        direct_shortest_path_routing(facade, phantom_node_pair, raw_route_data);
    }

    std::vector<EdgeWeight>
    ManyToManyRouting(const std::vector<PhantomNode> &phantom_nodes,
                      const std::vector<std::size_t> &source_indices,
                      const std::vector<std::size_t> &target_indices) const final override
    {
        return many_to_many_routing(facade, phantom_nodes, source_indices, target_indices);
    }

    routing_algorithms::SubMatchingList MapMatching(
        const routing_algorithms::CandidateLists &candidates_list,
        const std::vector<util::Coordinate> &trace_coordinates,
        const std::vector<unsigned> &trace_timestamps,
        const std::vector<boost::optional<double>> &trace_gps_precision) const final override
    {
        return map_matching(
            facade, candidates_list, trace_coordinates, trace_timestamps, trace_gps_precision);
    }

    std::vector<routing_algorithms::TurnData>
    TileTurns(const std::vector<datafacade::BaseDataFacade::RTreeLeaf> &edges,
              const std::vector<std::size_t> &sorted_edge_indexes) const final override
    {
        return tile_turns(facade, edges, sorted_edge_indexes);
    }

    bool HasAlternativeRouting() const final override
    {
        return algorithm_trais::HasAlternativeRouting<AlgorithmT>()(facade);
    };

  private:
    // Owned by shared-ptr passed to the query
    const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &facade;

    mutable routing_algorithms::AlternativeRouting<AlgorithmT> alternative_routing;
    mutable routing_algorithms::ShortestPathRouting<AlgorithmT> shortest_path_routing;
    mutable routing_algorithms::DirectShortestPathRouting<AlgorithmT> direct_shortest_path_routing;
    mutable routing_algorithms::ManyToManyRouting<AlgorithmT> many_to_many_routing;
    mutable routing_algorithms::MapMatching<AlgorithmT> map_matching;
    routing_algorithms::TileTurns<AlgorithmT> tile_turns;
};
}
}

#endif
