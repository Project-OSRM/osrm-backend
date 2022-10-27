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
    auto static_files = storage.GetStaticFiles();
    auto updatable_files = storage.GetUpdatableFiles();
    std::unique_ptr<storage::BaseDataLayout> layout =
        std::make_unique<storage::ContiguousDataLayout>();
    storage.PopulateLayoutWithRTree(*layout);
    storage.PopulateLayout(*layout, static_files);
    storage.PopulateLayout(*layout, updatable_files);

    // Allocate the memory block, then load data from files into it
    internal_memory = std::make_unique<char[]>(layout->GetSizeOfLayout());

    std::vector<storage::SharedDataIndex::AllocatedRegion> regions;
    regions.push_back({internal_memory.get(), std::move(layout)});
    index = {std::move(regions)};

    storage.PopulateStaticData(index);
    storage.PopulateUpdatableData(index);
}

ProcessMemoryAllocator::~ProcessMemoryAllocator() {}

const storage::SharedDataIndex &ProcessMemoryAllocator::GetIndex() { return index; }

} // namespace datafacade
} // namespace engine
} // namespace osrm
