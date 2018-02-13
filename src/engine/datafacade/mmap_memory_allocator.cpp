#include "engine/datafacade/mmap_memory_allocator.hpp"

#include "storage/storage.hpp"

#include "util/log.hpp"
#include "util/mmap_file.hpp"

#include "boost/assert.hpp"

namespace osrm
{
namespace engine
{
namespace datafacade
{

MMapMemoryAllocator::MMapMemoryAllocator(const storage::StorageConfig &config,
                                         const boost::filesystem::path &memory_file)
{
    storage::Storage storage(config);

    if (!boost::filesystem::exists(memory_file))
    {
        storage::DataLayout initial_layout;
        storage.PopulateLayout(initial_layout);

        auto data_size = initial_layout.GetSizeOfLayout();
        auto total_size = data_size + sizeof(storage::DataLayout);

        mapped_memory = util::mmapFile<char>(memory_file, mapped_memory_file, total_size);

        data_layout = reinterpret_cast<storage::DataLayout *>(mapped_memory.data());
        *data_layout = initial_layout;
        storage.PopulateData(*data_layout, GetMemory());
    }
    else
    {
        mapped_memory = util::mmapFile<char>(memory_file, mapped_memory_file);
        data_layout = reinterpret_cast<storage::DataLayout *>(mapped_memory.data());
    }
}

MMapMemoryAllocator::~MMapMemoryAllocator() {}

storage::DataLayout &MMapMemoryAllocator::GetLayout() { return *data_layout; }
char *MMapMemoryAllocator::GetMemory()
{
    return mapped_memory.data() + sizeof(storage::DataLayout);
}

} // namespace datafacade
} // namespace engine
} // namespace osrm
