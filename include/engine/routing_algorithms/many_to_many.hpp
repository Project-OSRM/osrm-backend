#ifndef MANY_TO_MANY_ROUTING_HPP
#define MANY_TO_MANY_ROUTING_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace ch
{
std::vector<EdgeWeight>
manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                 const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices);
} // namespace ch

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
