#ifndef SHARED_MEMORY_DATAFACADE_HPP
#define SHARED_MEMORY_DATAFACADE_HPP

// implements all data storage when shared memory _IS_ used

#include "storage/shared_barriers.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade_base.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

/**
 * This datafacade uses an IPC shared memory block as the data location.
 * Many SharedMemoryDataFacade objects can be created that point to the same shared
 * memory block.
 */
class SharedMemoryDataFacade : public ContiguousInternalMemoryDataFacadeBase
{

  protected:
    std::unique_ptr<storage::SharedMemory> m_large_memory;
    std::shared_ptr<storage::SharedBarriers> shared_barriers;
    storage::SharedDataType data_region;
    unsigned shared_timestamp;

    SharedMemoryDataFacade() {}

  public:
    // this function handle the deallocation of the shared memory it we can prove it will not be
    // used anymore.  We crash hard here if something goes wrong (noexcept).
    virtual ~SharedMemoryDataFacade() noexcept
    {
        // Now check if this is still the newest dataset
        boost::interprocess::sharable_lock<boost::interprocess::named_upgradable_mutex>
            current_regions_lock(shared_barriers->current_region_mutex,
                                 boost::interprocess::defer_lock);

        boost::interprocess::scoped_lock<boost::interprocess::named_sharable_mutex> exclusive_lock(
            data_region == storage::REGION_1 ? shared_barriers->region_1_mutex
                                             : shared_barriers->region_2_mutex,
            boost::interprocess::defer_lock);

        // if this returns false this is still in use
        if (current_regions_lock.try_lock() && exclusive_lock.try_lock())
        {
            if (storage::SharedMemory::RegionExists(data_region))
            {
                auto shared_region = storage::makeSharedMemory(storage::CURRENT_REGION);
                const auto current_timestamp =
                    static_cast<const storage::SharedDataTimestamp *>(shared_region->Ptr());

                // check if the memory region referenced by this facade needs cleanup
                if (current_timestamp->region == data_region)
                {
                    util::Log(logDEBUG) << "Retaining data with shared timestamp "
                                        << shared_timestamp;
                }
                else
                {
                    storage::SharedMemory::Remove(data_region);
                }
            }
        }
    }

    SharedMemoryDataFacade(const std::shared_ptr<storage::SharedBarriers> &shared_barriers_,
                           storage::SharedDataType data_region_,
                           unsigned shared_timestamp_)
        : shared_barriers(shared_barriers_), data_region(data_region_), shared_timestamp(shared_timestamp_)
    {
        util::Log(logDEBUG) << "Loading new data with shared timestamp " << shared_timestamp;

        BOOST_ASSERT(storage::SharedMemory::RegionExists(data_region));
        m_large_memory = storage::makeSharedMemory(data_region);

        InitializeInternalPointers(*reinterpret_cast<storage::DataLayout *>(m_large_memory->Ptr()),
                                   reinterpret_cast<char *>(m_large_memory->Ptr()) + sizeof(storage::DataLayout));
    }
};
}
}
}

#endif // SHARED_MEMORY_DATAFACADE_HPP
