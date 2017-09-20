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

template <typename Algorithm>
std::vector<EdgeDuration> manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                                           const DataFacade<Algorithm> &facade,
                                           const std::vector<PhantomNode> &phantom_nodes,
                                           std::vector<std::size_t> source_indices,
                                           std::vector<std::size_t> target_indices);

namespace mld
{

template <bool DIRECTION>
std::vector<EdgeDuration> oneToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                                          const DataFacade<Algorithm> &facade,
                                          const std::vector<PhantomNode> &phantom_nodes,
                                          std::size_t phantom_index,
                                          std::vector<std::size_t> phantom_indices);

} // mld

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
