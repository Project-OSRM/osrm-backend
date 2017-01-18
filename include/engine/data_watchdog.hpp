#ifndef OSRM_ENGINE_DATA_WATCHDOG_HPP
#define OSRM_ENGINE_DATA_WATCHDOG_HPP

#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/datafacade/shared_memory_allocator.hpp"

#include "storage/shared_barriers.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"

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
class DataWatchdog
{
  public:
    DataWatchdog() : active(true), timestamp(0)
    {
        // create the initial facade before launching the watchdog thread
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> current_region_lock(
                barrier.region_mutex);

            auto shared_memory = makeSharedMemory(storage::CURRENT_REGION);
            auto current = static_cast<storage::SharedDataTimestamp *>(shared_memory->Ptr());

            facade = std::make_shared<datafacade::ContiguousInternalMemoryDataFacade>(
                std::make_unique<datafacade::SharedMemoryAllocator>(current->region));
            timestamp = current->timestamp;
        }

        watcher = std::thread(&DataWatchdog::Run, this);
    }

    ~DataWatchdog()
    {
        active = false;
        barrier.region_condition.notify_all();
        watcher.join();
    }

    // Tries to connect to the shared memory containing the regions table
    static bool TryConnect()
    {
        return storage::SharedMemory::RegionExists(storage::CURRENT_REGION);
    }

    auto GetDataFacade() const { return facade; }

  private:
    void Run()
    {
        auto shared_memory = makeSharedMemory(storage::CURRENT_REGION);
        auto current = static_cast<storage::SharedDataTimestamp *>(shared_memory->Ptr());

        while (active)
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> current_region_lock(
                barrier.region_mutex);

            while (active && timestamp == current->timestamp)
            {
                barrier.region_condition.wait(current_region_lock);
            }

            if (timestamp != current->timestamp)
            {
                facade = std::make_shared<datafacade::ContiguousInternalMemoryDataFacade>(
                    std::make_unique<datafacade::SharedMemoryAllocator>(current->region));
                timestamp = current->timestamp;
                util::Log() << "updated facade to region "
                            << storage::regionToString(current->region) << " with timestamp "
                            << current->timestamp;
            }
        }

        util::Log() << "DataWatchdog thread stopped";
    }

    storage::SharedBarriers barrier;
    std::thread watcher;
    bool active;
    unsigned timestamp;
    std::shared_ptr<datafacade::ContiguousInternalMemoryDataFacade> facade;
};
}
}

#endif
