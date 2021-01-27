#ifndef OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP
#define OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP

#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"

namespace osrm
{
namespace engine
{

using DataFacadeBase = datafacade::ContiguousInternalMemoryDataFacadeBase;
template <typename AlgorithmT>
using DataFacade = datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT>;
} // namespace engine
} // namespace osrm

#endif
