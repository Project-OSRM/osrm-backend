#include "engine/datafacade/shared_memory_allocator.hpp"
#include "util/log.hpp"

#include "boost/assert.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

SharedMemoryAllocator::SharedMemoryAllocator(storage::SharedDataType data_region)
{
    util::Log(logDEBUG) << "Loading new data for region " << regionToString(data_region);

    BOOST_ASSERT(storage::SharedMemory::RegionExists(data_region));
    m_large_memory = storage::makeSharedMemory(data_region);
}

SharedMemoryAllocator::~SharedMemoryAllocator() {}

storage::DataLayout &SharedMemoryAllocator::GetLayout()
{
    return *reinterpret_cast<storage::DataLayout *>(m_large_memory->Ptr());
}
char *SharedMemoryAllocator::GetMemory()
{
    return reinterpret_cast<char *>(m_large_memory->Ptr()) + sizeof(storage::DataLayout);
}

} // namespace datafacade
} // namespace engine
} // namespace osrm
