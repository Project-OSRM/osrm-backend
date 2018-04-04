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
    storage::DataLayout layout;
    storage.PopulateStaticLayout(layout);
    storage.PopulateUpdatableLayout(layout);

    // Allocate the memory block, then load data from files into it
    internal_memory = std::make_unique<char[]>(layout.GetSizeOfLayout());

    index = storage::SharedDataIndex({{internal_memory.get(), std::move(layout)}});

    storage.PopulateStaticData(index);
    storage.PopulateUpdatableData(index);
}

ProcessMemoryAllocator::~ProcessMemoryAllocator() {}

const storage::SharedDataIndex &ProcessMemoryAllocator::GetIndex() { return index; }

} // namespace datafacade
} // namespace engine
} // namespace osrm
