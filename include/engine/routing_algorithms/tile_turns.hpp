#ifndef OSRM_ENGINE_ROUTING_ALGORITHMS_TILE_TURNS_HPP
#define OSRM_ENGINE_ROUTING_ALGORITHMS_TILE_TURNS_HPP

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

template <typename AlgorithmT> class TileTurns;

// Used to accumulate all the information we want in the tile about a turn.
struct TurnData final
{
    const util::Coordinate coordinate;
    const int in_angle;
    const int turn_angle;
    const int weight;
};

/// This class is used to extract turn information for the tile plugin from a CH graph
template <> class TileTurns<algorithm::CH> final : public BasicRouting<algorithm::CH>
{
    using super = BasicRouting<algorithm::CH>;
    using FacadeT = datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH>;

    using RTreeLeaf = datafacade::BaseDataFacade::RTreeLeaf;

  public:
    std::vector<TurnData> operator()(const FacadeT &facade,
                                     const std::vector<RTreeLeaf> &edges,
                                     const std::vector<std::size_t> &sorted_edge_indexes) const;
};

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
