#ifndef OSRM_ENGINE_ROUTING_ALGORITHMS_TILE_TURNS_HPP
#define OSRM_ENGINE_ROUTING_ALGORITHMS_TILE_TURNS_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"

#include "util/coordinate.hpp"
#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

// Used to accumulate all the information we want in the tile about a turn.
struct TurnData final
{
    const util::Coordinate coordinate;
    const int in_angle;
    const int turn_angle;
    const EdgeWeight weight;
    const EdgeWeight duration;
    const guidance::TurnInstruction turn_instruction;
};

using RTreeLeaf = datafacade::BaseDataFacade::RTreeLeaf;

std::vector<TurnData> getTileTurns(const DataFacade<ch::Algorithm> &facade,
                                   const std::vector<RTreeLeaf> &edges,
                                   const std::vector<std::size_t> &sorted_edge_indexes);

std::vector<TurnData> getTileTurns(const DataFacade<mld::Algorithm> &facade,
                                   const std::vector<RTreeLeaf> &edges,
                                   const std::vector<std::size_t> &sorted_edge_indexes);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
