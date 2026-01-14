#include "engine/datafacade/shared_memory_allocator.hpp"

#include "storage/serialization.hpp"

#include "util/log.hpp"

#include "boost/assert.hpp"

namespace osrm::engine::datafacade
{

SharedMemoryAllocator::SharedMemoryAllocator(const std::vector<storage::ProjID> &proj_ids)
{
    std::vector<storage::SharedDataIndex::AllocatedRegion> regions;

    for (const auto proj_id : proj_ids)
    {
        util::Log(logDEBUG) << "Loading new data for region " << (int)proj_id;
        BOOST_ASSERT(storage::RegionExists(proj_id));
        auto mem = storage::makeSharedMemory(proj_id);

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

} // namespace osrm::engine::datafacade
