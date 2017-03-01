#ifndef OSRM_ENGINE_DATA_WATCHDOG_HPP
#define OSRM_ENGINE_DATA_WATCHDOG_HPP

#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/datafacade/shared_memory_allocator.hpp"

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

// This class monitors the shared memory region that contains the pointers to
// the data and layout regions that should be used. This region is updated
// once a new dataset arrives.
template <typename AlgorithmT> class DataWatchdog final
{
    using mutex_type = typename storage::SharedMonitor<storage::SharedDataTimestamp>::mutex_type;
    using FacadeT = datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT>;

  public:
    DataWatchdog() : active(true), timestamp(0)
    {
        // create the initial facade before launching the watchdog thread
        {
            boost::interprocess::scoped_lock<mutex_type> current_region_lock(barrier.get_mutex());

            facade = std::make_shared<const FacadeT>(
                std::make_unique<datafacade::SharedMemoryAllocator>(barrier.data().region));
            timestamp = barrier.data().timestamp;
        }

        watcher = std::thread(&DataWatchdog::Run, this);
    }

    ~DataWatchdog()
    {
        active = false;
        barrier.notify_all();
        watcher.join();
    }

    std::shared_ptr<const FacadeT> Get() const { return facade; }

  private:
    void Run()
    {
        while (active)
        {
            boost::interprocess::scoped_lock<mutex_type> current_region_lock(barrier.get_mutex());

            while (active && timestamp == barrier.data().timestamp)
            {
                barrier.wait(current_region_lock);
            }

            if (timestamp != barrier.data().timestamp)
            {
                auto region = barrier.data().region;
                facade = std::make_shared<const FacadeT>(
                    std::make_unique<datafacade::SharedMemoryAllocator>(region));
                timestamp = barrier.data().timestamp;
                util::Log() << "updated facade to region " << region << " with timestamp "
                            << timestamp;
            }
        }

        util::Log() << "DataWatchdog thread stopped";
    }

    storage::SharedMonitor<storage::SharedDataTimestamp> barrier;
    std::thread watcher;
    bool active;
    unsigned timestamp;
    std::shared_ptr<const FacadeT> facade;
};
}
}

#endif
