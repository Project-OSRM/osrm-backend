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
    DataWatchdog()
        : shared_barriers{std::make_shared<storage::SharedBarriers>()},
          shared_regions(storage::makeSharedMemory(storage::CURRENT_REGION)),
          current_timestamp{storage::REGION_NONE, 0}
    {
    }

    // Tries to connect to the shared memory containing the regions table
    static bool TryConnect()
    {
        return storage::SharedMemory::RegionExists(storage::CURRENT_REGION);
    }

    using RegionsLock =
        boost::interprocess::sharable_lock<boost::interprocess::named_sharable_mutex>;
    using LockAndFacade = std::pair<RegionsLock, std::shared_ptr<datafacade::BaseDataFacade>>;

    // This will either update the contens of facade or just leave it as is
    // if the update was already done by another thread
    LockAndFacade GetDataFacade()
    {
        const boost::interprocess::sharable_lock<boost::interprocess::named_upgradable_mutex> lock(
            shared_barriers->current_region_mutex);

        const auto shared_timestamp =
            static_cast<const storage::SharedDataTimestamp *>(shared_regions->Ptr());

        const auto get_locked_facade = [this, shared_timestamp](
            const std::shared_ptr<datafacade::SharedMemoryDataFacade> &facade) {
            if (current_timestamp.region == storage::REGION_1)
            {
                return std::make_pair(RegionsLock(shared_barriers->region_1_mutex), facade);
            }
            else
            {
                BOOST_ASSERT(current_timestamp.region == storage::REGION_2);
                return std::make_pair(RegionsLock(shared_barriers->region_2_mutex), facade);
            }
        };

        // this blocks handle the common case when there is no data update -> we will only need a
        // shared lock
        {
            boost::shared_lock<boost::shared_mutex> facade_lock(facade_mutex);

            if (shared_timestamp->timestamp == current_timestamp.timestamp)
            {
                if (auto facade = cached_facade.lock())
                {
                    BOOST_ASSERT(shared_timestamp->region == current_timestamp.region);
                    return get_locked_facade(facade);
                }
            }
        }

        // if we reach this code there is a data update to be made. multiple
        // requests can reach this, but only ever one goes through at a time.
        boost::upgrade_lock<boost::shared_mutex> facade_lock(facade_mutex);

        // we might get overtaken before we actually do the writing
        // in that case we don't modify anything
        if (shared_timestamp->timestamp == current_timestamp.timestamp)
        {
            if (auto facade = cached_facade.lock())
            {
                BOOST_ASSERT(shared_timestamp->region == current_timestamp.region);
                return get_locked_facade(facade);
            }
        }

        // this thread has won and can update the data
        boost::upgrade_to_unique_lock<boost::upgrade_mutex> unique_facade_lock(facade_lock);

        // if two threads try to enter this critical section one will loose
        // and will find an up-to-date instance of the shared data facade
        if (shared_timestamp->timestamp == current_timestamp.timestamp)
        {
            // if the thread that updated the facade finishes the query before
            // we can aquire our handle here, we need to regenerate
            if (auto facade = cached_facade.lock())
            {
                BOOST_ASSERT(shared_timestamp->region == current_timestamp.region);

                return get_locked_facade(facade);
            }
        }
        else
        {
            current_timestamp = *shared_timestamp;
        }

        auto new_facade = std::make_shared<datafacade::SharedMemoryDataFacade>(
            shared_barriers, current_timestamp.region, current_timestamp.timestamp);
        cached_facade = new_facade;

        return get_locked_facade(new_facade);
    }

  private:
    // mutexes should be mutable even on const objects: This enables
    // marking functions as logical const and thread-safe.
    std::shared_ptr<storage::SharedBarriers> shared_barriers;

    // shared memory table containing pointers to all shared regions
    std::unique_ptr<storage::SharedMemory> shared_regions;

    mutable boost::shared_mutex facade_mutex;
    std::weak_ptr<datafacade::SharedMemoryDataFacade> cached_facade;
    storage::SharedDataTimestamp current_timestamp;
};
}
}

#endif
