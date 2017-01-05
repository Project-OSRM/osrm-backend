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
    storage::SharedDataType data_region;

    SharedMemoryDataFacade() {}

  public:
    SharedMemoryDataFacade(storage::SharedDataType data_region) : data_region(data_region)
    {
        util::Log(logDEBUG) << "Loading new data for region " << regionToString(data_region);

        BOOST_ASSERT(storage::SharedMemory::RegionExists(data_region));
        m_large_memory = storage::makeSharedMemory(data_region);

        InitializeInternalPointers(*reinterpret_cast<storage::DataLayout *>(m_large_memory->Ptr()),
                                   reinterpret_cast<char *>(m_large_memory->Ptr()) +
                                       sizeof(storage::DataLayout));
    }
};
}
}
}

#endif // SHARED_MEMORY_DATAFACADE_HPP
