#ifndef OSRM_ENGINE_DATAFACADE_PROVIDER_HPP
#define OSRM_ENGINE_DATAFACADE_PROVIDER_HPP

#include "engine/data_watchdog.hpp"
#include "engine/datafacade.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/datafacade/process_memory_allocator.hpp"

namespace osrm
{
namespace engine
{
namespace detail
{

template <typename AlgorithmT, template <typename A> class FacadeT> class DataFacadeProvider
{
  public:
    using Facade = FacadeT<AlgorithmT>;

    virtual ~DataFacadeProvider() = default;

    virtual std::shared_ptr<const Facade> Get() const = 0;
};

template <typename AlgorithmT, template <typename A> class FacadeT>
class ImmutableProvider final : public DataFacadeProvider<AlgorithmT, FacadeT>
{
  public:
    using Facade = typename DataFacadeProvider<AlgorithmT, FacadeT>::Facade;

    ImmutableProvider(const storage::StorageConfig &config)
        : immutable_data_facade(std::make_shared<Facade>(
              std::make_shared<datafacade::ProcessMemoryAllocator>(config)))
    {
    }

    std::shared_ptr<const Facade> Get() const override final { return immutable_data_facade; }

  private:
    std::shared_ptr<const Facade> immutable_data_facade;
};

template <typename AlgorithmT, template <typename A> class FacadeT>
class WatchingProvider : public DataFacadeProvider<AlgorithmT, FacadeT>
{
    DataWatchdog<AlgorithmT> watchdog;

  public:
    using Facade = typename DataFacadeProvider<AlgorithmT, FacadeT>::Facade;

    std::shared_ptr<const Facade> Get() const override final { return watchdog.Get(); }
};
}

template <typename AlgorithmT>
using DataFacadeProvider = detail::DataFacadeProvider<AlgorithmT, DataFacade>;
template <typename AlgorithmT>
using WatchingProvider = detail::WatchingProvider<AlgorithmT, DataFacade>;
template <typename AlgorithmT>
using ImmutableProvider = detail::ImmutableProvider<AlgorithmT, DataFacade>;
}
}

#endif
