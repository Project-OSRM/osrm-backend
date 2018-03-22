#include "engine/datafacade/process_memory_allocator.hpp"
#include "storage/storage.hpp"

#include "boost/assert.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

ProcessMemoryAllocator::ProcessMemoryAllocator(const storage::StorageConfig &config)
{
    storage::Storage storage(config);

    // Calculate the layout/size of the memory block
    storage.PopulateLayout(internal_layout);

    // Allocate the memory block, then load data from files into it
    internal_memory = std::make_unique<char[]>(internal_layout.GetSizeOfLayout());
    storage.PopulateData(internal_layout, internal_memory.get());
}

ProcessMemoryAllocator::~ProcessMemoryAllocator() {}

const storage::DataLayout &ProcessMemoryAllocator::GetLayout() { return internal_layout; }
char *ProcessMemoryAllocator::GetMemory() { return internal_memory.get(); }

} // namespace datafacade
} // namespace engine
} // namespace osrm
