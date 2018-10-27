#ifndef OSRM_ENGINE_DATAFACADE_PROVIDER_HPP
#define OSRM_ENGINE_DATAFACADE_PROVIDER_HPP

#include "engine/data_watchdog.hpp"
#include "engine/datafacade.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/datafacade/mmap_memory_allocator.hpp"
#include "engine/datafacade/process_memory_allocator.hpp"
#include "engine/datafacade_factory.hpp"

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

    virtual std::shared_ptr<const Facade> Get(const api::BaseParameters &) const = 0;
    virtual std::shared_ptr<const Facade> Get(const api::TileParameters &) const = 0;
};

template <typename AlgorithmT, template <typename A> class FacadeT>
class ExternalProvider final : public DataFacadeProvider<AlgorithmT, FacadeT>
{
  public:
    using Facade = typename DataFacadeProvider<AlgorithmT, FacadeT>::Facade;

    ExternalProvider(const storage::StorageConfig &config)
        : facade_factory(std::make_shared<datafacade::MMapMemoryAllocator>(config))
    {
    }

    std::shared_ptr<const Facade> Get(const api::TileParameters &params) const override final
    {
        return facade_factory.Get(params);
    }
    std::shared_ptr<const Facade> Get(const api::BaseParameters &params) const override final
    {
        return facade_factory.Get(params);
    }

  private:
    DataFacadeFactory<FacadeT, AlgorithmT> facade_factory;
};

template <typename AlgorithmT, template <typename A> class FacadeT>
class ImmutableProvider final : public DataFacadeProvider<AlgorithmT, FacadeT>
{
  public:
    using Facade = typename DataFacadeProvider<AlgorithmT, FacadeT>::Facade;

    ImmutableProvider(const storage::StorageConfig &config)
        : facade_factory(std::make_shared<datafacade::ProcessMemoryAllocator>(config))
    {
    }

    std::shared_ptr<const Facade> Get(const api::TileParameters &params) const override final
    {
        return facade_factory.Get(params);
    }
    std::shared_ptr<const Facade> Get(const api::BaseParameters &params) const override final
    {
        return facade_factory.Get(params);
    }

  private:
    DataFacadeFactory<FacadeT, AlgorithmT> facade_factory;
};

template <typename AlgorithmT, template <typename A> class FacadeT>
class WatchingProvider : public DataFacadeProvider<AlgorithmT, FacadeT>
{
    DataWatchdog<AlgorithmT, FacadeT> watchdog;

  public:
    using Facade = typename DataFacadeProvider<AlgorithmT, FacadeT>::Facade;

    WatchingProvider(const std::string &dataset_name) : watchdog(dataset_name) {}

    std::shared_ptr<const Facade> Get(const api::TileParameters &params) const override final
    {
        return watchdog.Get(params);
    }
    std::shared_ptr<const Facade> Get(const api::BaseParameters &params) const override final
    {
        return watchdog.Get(params);
    }
};
}

template <typename AlgorithmT>
using DataFacadeProvider = detail::DataFacadeProvider<AlgorithmT, DataFacade>;
template <typename AlgorithmT>
using WatchingProvider = detail::WatchingProvider<AlgorithmT, DataFacade>;
template <typename AlgorithmT>
using ImmutableProvider = detail::ImmutableProvider<AlgorithmT, DataFacade>;
template <typename AlgorithmT>
using ExternalProvider = detail::ExternalProvider<AlgorithmT, DataFacade>;
}
}

#endif
