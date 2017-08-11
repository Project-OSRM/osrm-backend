#ifndef OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP
#define OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP

#ifdef OSRM_EXTERNAL_MEMORY

#include "routing/compressed_datafacade.hpp"

namespace osrm
{
namespace engine
{

using DataFacadeBase = datafacade::BaseDataFacade;
template <typename AlgorithmT> using DataFacade = datafacade::CompressedDataFacadeT<AlgorithmT>;
}
}

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
