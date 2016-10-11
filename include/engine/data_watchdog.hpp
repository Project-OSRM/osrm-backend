#ifndef OSRM_ENGINE_DATA_WATCHDOG_HPP
#define OSRM_ENGINE_DATA_WATCHDOG_HPP

#include "engine/datafacade/shared_datafacade.hpp"

#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_barriers.hpp"

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
//
// TODO: This also needs a shared memory reader lock with other clients and
// possibly osrm-datastore since updating the CURRENT_REGIONS data is not atomic.
// Currently we enfore this by waiting that all queries have finished before
// osrm-datastore writes to this section.
class DataWatchdog
{
  public:
    DataWatchdog()
        : shared_barriers{std::make_shared<storage::SharedBarriers>()},
          shared_regions(storage::makeSharedMemory(storage::CURRENT_REGIONS)),
          current_timestamp{storage::LAYOUT_NONE, storage::DATA_NONE, 0}
    {
    }

    // Tries to connect to the shared memory containing the regions table
    static bool TryConnect()
    {
        return storage::SharedMemory::RegionExists(storage::CURRENT_REGIONS);
    }

    using RegionsLock =
        boost::interprocess::sharable_lock<boost::interprocess::named_sharable_mutex>;
    using LockAndFacade = std::pair<RegionsLock, std::shared_ptr<datafacade::BaseDataFacade>>;

    // This will either update the contens of facade or just leave it as is
    // if the update was already done by another thread
    LockAndFacade GetDataFacade()
    {
        const boost::interprocess::sharable_lock<boost::interprocess::named_upgradable_mutex> lock(
            shared_barriers->current_regions_mutex);

        const auto shared_timestamp =
            static_cast<const storage::SharedDataTimestamp *>(shared_regions->Ptr());

        const auto get_locked_facade = [this, shared_timestamp]() {
            if (current_timestamp.data == storage::DATA_1)
            {
                BOOST_ASSERT(current_timestamp.layout == storage::LAYOUT_1);
                return std::make_pair(RegionsLock(shared_barriers->regions_1_mutex), facade);
            }
            else
            {
                BOOST_ASSERT(current_timestamp.layout == storage::LAYOUT_2);
                BOOST_ASSERT(current_timestamp.data == storage::DATA_2);
                return std::make_pair(RegionsLock(shared_barriers->regions_2_mutex), facade);
            }
        };

        // this blocks handle the common case when there is no data update -> we will only need a
        // shared lock
        {
            boost::shared_lock<boost::shared_mutex> facade_lock(facade_mutex);

            if (shared_timestamp->timestamp == current_timestamp.timestamp)
            {
                BOOST_ASSERT(shared_timestamp->layout == current_timestamp.layout);
                BOOST_ASSERT(shared_timestamp->data == current_timestamp.data);
                return get_locked_facade();
            }
        }

        // if we reach this code there is a data update to be made. multiple
        // requests can reach this, but only ever one goes through at a time.
        boost::upgrade_lock<boost::shared_mutex> facade_lock(facade_mutex);

        // we might get overtaken before we actually do the writing
        // in that case we don't modify anthing
        if (shared_timestamp->timestamp == current_timestamp.timestamp)
        {
            BOOST_ASSERT(shared_timestamp->layout == current_timestamp.layout);
            BOOST_ASSERT(shared_timestamp->data == current_timestamp.data);

            return get_locked_facade();
        }

        // this thread has won and can update the data
        boost::upgrade_to_unique_lock<boost::upgrade_mutex> unique_facade_lock(facade_lock);

        current_timestamp = *shared_timestamp;
        facade = std::make_shared<datafacade::SharedDataFacade>(
            current_timestamp.layout, current_timestamp.data, current_timestamp.timestamp);

        return get_locked_facade();
    }

  private:
    // mutexes should be mutable even on const objects: This enables
    // marking functions as logical const and thread-safe.
    std::shared_ptr<storage::SharedBarriers> shared_barriers;

    // shared memory table containing pointers to all shared regions
    std::unique_ptr<storage::SharedMemory> shared_regions;

    mutable boost::shared_mutex facade_mutex;
    std::shared_ptr<datafacade::SharedDataFacade> facade;
    storage::SharedDataTimestamp current_timestamp;
};
}
}

#endif
