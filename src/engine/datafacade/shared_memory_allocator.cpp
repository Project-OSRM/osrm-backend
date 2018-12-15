#include "engine/datafacade/shared_memory_allocator.hpp"

#include "storage/serialization.hpp"

#include "util/log.hpp"

#include "boost/assert.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

SharedMemoryAllocator::SharedMemoryAllocator(
    const std::vector<storage::SharedRegionRegister::ShmKey> &shm_keys)
{
    std::vector<storage::SharedDataIndex::AllocatedRegion> regions;

    for (const auto shm_key : shm_keys)
    {
        util::Log(logDEBUG) << "Loading new data for region " << (int)shm_key;
        BOOST_ASSERT(storage::SharedMemory::RegionExists(shm_key));
        auto mem = storage::makeSharedMemory(shm_key);

        storage::io::BufferReader reader(reinterpret_cast<char *>(mem->Ptr()), mem->Size());
        std::unique_ptr<storage::BaseDataLayout> layout =
            std::make_unique<storage::ContiguousDataLayout>();
        storage::serialization::read(reader, *layout);
        auto layout_size = reader.GetPosition();

        regions.push_back({reinterpret_cast<char *>(mem->Ptr()) + layout_size, std::move(layout)});
        memory_regions.push_back(std::move(mem));
    }

    index = storage::SharedDataIndex{std::move(regions)};
}

SharedMemoryAllocator::~SharedMemoryAllocator() {}

const storage::SharedDataIndex &SharedMemoryAllocator::GetIndex() { return index; }

} // namespace datafacade
} // namespace engine
} // namespace osrm
