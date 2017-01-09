#ifndef DIRECT_SHORTEST_PATH_HPP
#define DIRECT_SHORTEST_PATH_HPP

#include "engine/routing_algorithms/routing_base.hpp"

#include "engine/algorithm.hpp"
#include "engine/search_engine_data.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

template <typename AlgorithmT> class DirectShortestPathRouting;

/// This is a striped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrainted
/// by the previous route.
/// This variation is only an optimazation for graphs with slow queries, for example
/// not fully contracted graphs.
template <>
class DirectShortestPathRouting<algorithm::CH> final : public BasicRouting<algorithm::CH>
{
    using super = BasicRouting<algorithm::CH>;
    using FacadeT = datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

  public:
    DirectShortestPathRouting(SearchEngineData &engine_working_data)
        : engine_working_data(engine_working_data)
    {
    }

    ~DirectShortestPathRouting() {}

    void operator()(const FacadeT &facade,
                    const std::vector<PhantomNodes> &phantom_nodes_vector,
                    InternalRouteResult &raw_route_data) const;
};

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif /* DIRECT_SHORTEST_PATH_HPP */
