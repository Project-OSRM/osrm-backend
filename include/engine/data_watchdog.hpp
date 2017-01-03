#ifndef OSRM_ENGINE_DATA_WATCHDOG_HPP
#define OSRM_ENGINE_DATA_WATCHDOG_HPP

#include "engine/datafacade/shared_memory_datafacade.hpp"

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
// the dataset regions that should be used. This region is updated
// once a new dataset arrives.
class DataWatchdog
{
  static const constexpr unsigned MAX_UPDATE_WAIT_MSEC = 500;

  public:
    DataWatchdog()
        : shared_barriers{std::make_shared<storage::SharedBarriers>()},
          shared_regions(storage::makeSharedMemory(storage::CURRENT_REGION)),
          current_timestamp{-1}, check_for_updates{true}, watch_thread{&DataWatchdog::Watch, this}
    {
    }

    ~DataWatchdog()
    {
        check_for_updates = false;
        // will terminate on next wakeup
        watch_thread.join();
    }

    void Watch()
    {
        while (check_for_updates)
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex>
                update_lock{shared_barriers->current_region_mutex};
            // we only wait for a limited amount of time to have the chance to exit this loop cleanly
            auto new_update = shared_barriers->new_dataset_condition.timed_wait(
                update_lock,
                boost::posix_time::microsec_clock::universal_time() +
                    boost::posix_time::millisec(MAX_UPDATE_WAIT_MSEC));

            if (new_update)
            {
                BOOST_ASSERT(update_lock.owns());
                const auto shared_timestamp =
                    static_cast<const storage::SharedDataTimestamp *>(shared_regions->Ptr());

                BOOST_ASSERT(current_timestamp != shared_timestamp->timestamp);
                auto facade = std::make_shared<datafacade::SharedMemoryDataFacade>(
                    shared_barriers, shared_timestamp->region, shared_timestamp->timestamp);
                RegionsMutex &mutex = [this, shared_timestamp]() -> RegionsMutex& {
                    if (shared_timestamp->region == storage::REGION_1)
                    {
                        return shared_barriers->region_1_mutex;
                    }
                    else
                    {
                        BOOST_ASSERT(shared_timestamp->region == storage::REGION_2);
                        return shared_barriers->region_2_mutex;
                    }
                }();

                util::Log() << "Updating " << current_timestamp << " to " << shared_timestamp->timestamp;
                current_timestamp = shared_timestamp->timestamp;

                auto new_mutex_and_facade = std::make_shared<MutexAndFacade>(mutex, facade);
                std::atomic_store(&mutex_and_facade, new_mutex_and_facade);
            }
        }
    }

    // Tries to connect to the shared memory containing the regions table
    static bool TryConnect()
    {
        return storage::SharedMemory::RegionExists(storage::CURRENT_REGION);
    }

    using RegionsMutex = boost::interprocess::named_sharable_mutex;
    using RegionsLock = boost::interprocess::sharable_lock<RegionsMutex>;
    using MutexAndFacade = std::pair<RegionsMutex &, std::shared_ptr<datafacade::BaseDataFacade>>;
    using LockAndFacade = std::pair<RegionsLock, std::shared_ptr<datafacade::BaseDataFacade>>;

    // This will return the newest data facade and an appropriate read lock on the shared memory
    // region
    LockAndFacade GetDataFacade()
    {
        // copy locally first in case a data update happens concurrently
        auto mutex_and_facade_local = mutex_and_facade;

        return std::make_pair(RegionsLock(mutex_and_facade->first), mutex_and_facade->second);
    }

  private:
    // mutexes should be mutable even on const objects: This enables
    // marking functions as logical const and thread-safe.
    std::shared_ptr<storage::SharedBarriers> shared_barriers;

    // shared memory table containing pointers to all shared regions
    std::unique_ptr<storage::SharedMemory> shared_regions;

    int current_timestamp;
    std::shared_ptr<MutexAndFacade> mutex_and_facade;

    bool check_for_updates;
    std::mutex update_mutex;
    std::thread watch_thread;
};
}
}

#endif
