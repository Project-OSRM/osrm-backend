#ifndef OSRM_ENGINE_DATA_WATCHDOG_HPP
#define OSRM_ENGINE_DATA_WATCHDOG_HPP

#include "engine/datafacade/shared_datafacade.hpp"

#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"

#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_types.hpp>
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
          shared_regions(storage::makeSharedMemoryView(storage::CURRENT_REGIONS)),
          current_timestamp{storage::LAYOUT_NONE, storage::DATA_NONE, 0}
    {
    }

    // Tries to connect to the shared memory containing the regions table
    static bool TryConnect()
    {
        return storage::SharedMemory::RegionExists(storage::CURRENT_REGIONS);
    }

    // Check if it might be worth to try to aquire a exclusive lock
    bool HasNewRegion() const
    {
        const boost::interprocess::sharable_lock<boost::interprocess::named_upgradable_mutex> lock(
            shared_barriers->current_regions_mutex);

        const auto shared_timestamp =
            static_cast<const storage::SharedDataTimestamp *>(shared_regions->Ptr());

        // sanity check: if the timestamp is the same all other data needs to be the same as well
        BOOST_ASSERT(shared_timestamp->timestamp != current_timestamp.timestamp ||
                     (shared_timestamp->layout == current_timestamp.layout &&
                      shared_timestamp->data == current_timestamp.data));

        return shared_timestamp->timestamp != current_timestamp.timestamp;
    }

    // This will either update the contens of facade or just leave it as is
    // if the update was already done by another thread
    void MaybeLoadNewRegion(std::shared_ptr<datafacade::BaseDataFacade> &facade)
    {
        const boost::interprocess::sharable_lock<boost::interprocess::named_upgradable_mutex> lock(
            shared_barriers->current_regions_mutex);

        const auto shared_timestamp =
            static_cast<const storage::SharedDataTimestamp *>(shared_regions->Ptr());

        boost::upgrade_lock<boost::shared_mutex> facade_lock(facade_mutex);

        // if more then one request tried to aquire the write lock
        // we might get overtaken before we actually do the writing
        // in that case we don't modify anthing
        if (shared_timestamp->timestamp == current_timestamp.timestamp)
        {
            BOOST_ASSERT(shared_timestamp->layout == current_timestamp.layout);
            BOOST_ASSERT(shared_timestamp->data == current_timestamp.data);
        }
        // this thread has won and can update the data
        else
        {
            boost::upgrade_to_unique_lock<boost::upgrade_mutex> unique_facade_lock(facade_lock);

            current_timestamp = *shared_timestamp;
            // TODO remove once we allow for more then one SharedMemoryFacade at the same time
            // at this point no other query is allowed to reference this facade!
            // the old facade will die exactly here
            BOOST_ASSERT(!facade || facade.use_count() == 1);
            facade = std::make_shared<datafacade::SharedDataFacade>(shared_barriers,
                                                                    current_timestamp.layout,
                                                                    current_timestamp.data,
                                                                    current_timestamp.timestamp);
        }
    }

  private:
    // mutexes should be mutable even on const objects: This enables
    // marking functions as logical const and thread-safe.
    std::shared_ptr<storage::SharedBarriers> shared_barriers;

    // shared memory table containing pointers to all shared regions
    std::unique_ptr<storage::SharedMemory> shared_regions;

    mutable boost::shared_mutex facade_mutex;
    storage::SharedDataTimestamp current_timestamp;
};
}
}

#endif
