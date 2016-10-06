#include "engine/datafacade/shared_datafacade.hpp"

#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_types.hpp>

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
        : shared_regions(storage::makeSharedMemory(
              storage::CURRENT_REGIONS, sizeof(storage::SharedDataTimestamp), false, false)),
        current_timestamp {storage::LAYOUT_NONE, storage::DATA_NONE, 0}
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
        const boost::shared_lock<boost::shared_mutex> lock(current_timestamp_mutex);

        const auto shared_timestamp =
            static_cast<const storage::SharedDataTimestamp *>(shared_regions->Ptr());

        return shared_timestamp->layout != current_timestamp.layout ||
               shared_timestamp->data != current_timestamp.data ||
               shared_timestamp->timestamp != current_timestamp.timestamp;
    }

    // Note this can still return an emptry pointer if this function got overtaken by another thread
    std::shared_ptr<datafacade::SharedDataFacade> MaybeLoadNewRegion()
    {
        const boost::lock_guard<boost::shared_mutex> lock(current_timestamp_mutex);

        const auto shared_timestamp =
            static_cast<const storage::SharedDataTimestamp *>(shared_regions->Ptr());

        if (shared_timestamp->timestamp == current_timestamp.timestamp)
        {
            BOOST_ASSERT(shared_timestamp->layout == current_timestamp.layout);
            BOOST_ASSERT(shared_timestamp->data == current_timestamp.data);
            return std::shared_ptr<datafacade::SharedDataFacade>();
        }

        current_timestamp = *shared_timestamp;

        return std::make_shared<datafacade::SharedDataFacade>(
            current_timestamp.layout, current_timestamp.data, current_timestamp.timestamp);
    }

  private:
    // shared memory table containing pointers to all shared regions
    std::unique_ptr<storage::SharedMemory> shared_regions;

    // mutexes should be mutable even on const objects: This enables
    // marking functions as logical const and thread-safe.
    mutable boost::shared_mutex current_timestamp_mutex;
    storage::SharedDataTimestamp current_timestamp;
};
}
}
