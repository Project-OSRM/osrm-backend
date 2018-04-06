#include "engine/datafacade/mmap_memory_allocator.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"
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
        storage.PopulateStaticLayout(initial_layout);
        storage.PopulateUpdatableLayout(initial_layout);

        auto data_size = initial_layout.GetSizeOfLayout();

        storage::io::BufferWriter writer;
        storage::serialization::write(writer, initial_layout);
        auto encoded_layout = writer.GetBuffer();

        auto total_size = data_size + encoded_layout.size();

        mapped_memory = util::mmapFile<char>(memory_file, mapped_memory_file, total_size);

        std::copy(encoded_layout.begin(), encoded_layout.end(), mapped_memory.data());

        index = storage::SharedDataIndex(
            {{mapped_memory.data() + encoded_layout.size(), std::move(initial_layout)}});

        storage.PopulateStaticData(index);
        storage.PopulateUpdatableData(index);
    }
    else
    {
        mapped_memory = util::mmapFile<char>(memory_file, mapped_memory_file);

        storage::DataLayout layout;
        storage::io::BufferReader reader(mapped_memory.data(), mapped_memory.size());
        storage::serialization::read(reader, layout);
        auto layout_size = reader.GetPosition();

        index = storage::SharedDataIndex({{mapped_memory.data() + layout_size, std::move(layout)}});
    }
}

MMapMemoryAllocator::~MMapMemoryAllocator() {}

const storage::SharedDataIndex &MMapMemoryAllocator::GetIndex() { return index; }

} // namespace datafacade
} // namespace engine
} // namespace osrm
