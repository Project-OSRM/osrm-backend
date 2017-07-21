#ifndef OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP
#define OSRM_ENGINE_DATAFACADE_DATAFACADE_HPP

#include "engine/search_engine_data_fwd.hpp"

#ifdef OSRM_EXTERNAL_MEMORY

#include "engine/datafacade/datafacade_base.hpp"

#include "util/integer_range.hpp"

namespace osrm
{
namespace engine
{

using EdgeRange = util::range<EdgeID>;
using StringView = datafacade::StringView;

using DataFacadeBase = datafacade::BaseDataFacade;
template <typename AlgorithmT> class DataFacade;

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
