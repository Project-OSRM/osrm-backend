#ifndef OSRM_ENGINE_DATA_WATCHDOG_HPP
#define OSRM_ENGINE_DATA_WATCHDOG_HPP

#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/datafacade/shared_memory_allocator.hpp"
#include "engine/datafacade_factory.hpp"

#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_monitor.hpp"

#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <memory>
#include <thread>

namespace osrm
{
namespace engine
{

namespace detail
{
// We need this wrapper type since template-template specilization of FacadeT is broken on clang
// when it is combined with an templated alias (DataFacade in this case).
// See https://godbolt.org/g/ZS6Xmt for an example.
template <typename AlgorithmT, typename FacadeT> class DataWatchdogImpl;

template <typename AlgorithmT>
class DataWatchdogImpl<AlgorithmT, datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT>> final
{
    using mutex_type = typename storage::SharedMonitor<storage::SharedRegionRegister>::mutex_type;
    using Facade = datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT>;

  public:
    DataWatchdogImpl(const std::string &dataset_name) : dataset_name(dataset_name), active(true)
    {
        // create the initial facade before launching the watchdog thread
        {
            boost::interprocess::scoped_lock<mutex_type> current_region_lock(barrier.get_mutex());

            auto &shared_register = barrier.data();
            auto static_region_id = shared_register.Find(dataset_name + "/static");
            auto updatable_region_id = shared_register.Find(dataset_name + "/updatable");
            if (static_region_id == storage::SharedRegionRegister::INVALID_REGION_ID ||
                updatable_region_id == storage::SharedRegionRegister::INVALID_REGION_ID)
            {
                throw util::exception("Could not find shared memory region for \"" + dataset_name +
                                      "/data\". Did you run osrm-datastore?");
            }
            static_shared_region = &shared_register.GetRegion(static_region_id);
            updatable_shared_region = &shared_register.GetRegion(updatable_region_id);
            static_region = *static_shared_region;
            updatable_region = *updatable_shared_region;

            {
                boost::unique_lock<boost::shared_mutex> swap_lock(factory_mutex);
                facade_factory =
                    DataFacadeFactory<datafacade::ContiguousInternalMemoryDataFacade, AlgorithmT>(
                        std::make_shared<datafacade::SharedMemoryAllocator>(
                            std::vector<storage::SharedRegionRegister::ShmKey>{
                                static_region.shm_key, updatable_region.shm_key}));
            }
        }

        watcher = std::thread(&DataWatchdogImpl::Run, this);
    }

    ~DataWatchdogImpl()
    {
        active = false;
        barrier.notify_all();
        watcher.join();
    }

    std::shared_ptr<const Facade> Get(const api::BaseParameters &params) const
    {
        // make sure facade_factory stays stable while we call Get()
        boost::shared_lock<boost::shared_mutex> swap_lock(factory_mutex);
        return facade_factory.Get(params);
    }
    std::shared_ptr<const Facade> Get(const api::TileParameters &params) const
    {
        // make sure facade_factory stays stable while we call Get()
        boost::shared_lock<boost::shared_mutex> swap_lock(factory_mutex);
        return facade_factory.Get(params);
    }

  private:
    void Run()
    {
        while (active)
        {
            boost::interprocess::scoped_lock<mutex_type> current_region_lock(barrier.get_mutex());

            while (active && static_region.timestamp == static_shared_region->timestamp &&
                   updatable_region.timestamp == updatable_shared_region->timestamp)
            {
                barrier.wait(current_region_lock);
            }

            if (!active)
                break;

            if (static_region.timestamp != static_shared_region->timestamp)
            {
                static_region = *static_shared_region;
            }
            if (updatable_region.timestamp != updatable_shared_region->timestamp)
            {
                updatable_region = *updatable_shared_region;
            }

            util::Log() << "updated facade to regions " << (int)static_region.shm_key << " and "
                        << (int)updatable_region.shm_key << " with timestamps "
                        << static_region.timestamp << " and " << updatable_region.timestamp;

            {
                boost::unique_lock<boost::shared_mutex> swap_lock(factory_mutex);
                facade_factory =
                    DataFacadeFactory<datafacade::ContiguousInternalMemoryDataFacade, AlgorithmT>(
                        std::make_shared<datafacade::SharedMemoryAllocator>(
                            std::vector<storage::SharedRegionRegister::ShmKey>{
                                static_region.shm_key, updatable_region.shm_key}));
            }
        }

        util::Log() << "DataWatchdog thread stopped";
    }

    mutable boost::shared_mutex factory_mutex;
    const std::string dataset_name;
    storage::SharedMonitor<storage::SharedRegionRegister> barrier;
    std::thread watcher;
    bool active;
    storage::SharedRegion static_region;
    storage::SharedRegion updatable_region;
    storage::SharedRegion *static_shared_region;
    storage::SharedRegion *updatable_shared_region;
    DataFacadeFactory<datafacade::ContiguousInternalMemoryDataFacade, AlgorithmT> facade_factory;
};
} // namespace detail

// This class monitors the shared memory region that contains the pointers to
// the data and layout regions that should be used. This region is updated
// once a new dataset arrives.
template <typename AlgorithmT, template <typename A> class FacadeT>
using DataWatchdog = detail::DataWatchdogImpl<AlgorithmT, FacadeT<AlgorithmT>>;
} // namespace engine
} // namespace osrm

#endif
