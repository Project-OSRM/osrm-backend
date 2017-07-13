#ifndef OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP
#define OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP

#ifdef OSRM_EXTERNAL_MEMORY

// Register your own data backend here
#error "No external memory implementation found"

#else

#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"

namespace osrm
{
namespace engine
{

using DataFacadeBase = datafacade::ContiguousInternalMemoryDataFacadeBase;
template <typename AlgorithmT>
using DataFacade = datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT>;
}
}

#endif

#endif
