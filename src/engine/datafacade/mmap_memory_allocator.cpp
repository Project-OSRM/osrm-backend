#include "engine/datafacade/mmap_memory_allocator.hpp"

#include "storage/block.hpp"
#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "storage/storage.hpp"

#include "util/log.hpp"
#include "util/mmap_file.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
namespace datafacade
{

MMapMemoryAllocator::MMapMemoryAllocator(const storage::StorageConfig &config)
{
    storage::Storage storage(config);
    std::vector<storage::SharedDataIndex::AllocatedRegion> allocated_regions;

    {
        std::unique_ptr<storage::BaseDataLayout> fake_layout =
            std::make_unique<storage::TarDataLayout>();

        // Convert the boost::filesystem::path object into a plain string
        // that's stored as a member of this allocator object
        rtree_filename = storage.PopulateLayoutWithRTree(*fake_layout);

        // Now, we add one more AllocatedRegion, with it's start address as the start
        // of the rtree_filename string we've saved.  In the fake_layout, we've
        // stated that the data is at offset 0, which is where the string starts
        // at it's own memory address.
        // The syntax &(rtree_filename[0]) gets the memory address of the first char.
        // We can't use the convenient `.data()` or `.c_str()` methods, because
        // prior to C++17 (which we're not using), those return a `const char *`,
        // which isn't compatible with the `char *` that AllocatedRegion expects
        // for it's memory_ptr
        allocated_regions.push_back({&(rtree_filename[0]), std::move(fake_layout)});
    }

    auto files = storage.GetStaticFiles();
    auto updatable_files = storage.GetUpdatableFiles();
    files.insert(files.end(), updatable_files.begin(), updatable_files.end());

    for (const auto &file : files)
    {
        if (boost::filesystem::exists(file.second))
        {
            std::unique_ptr<storage::BaseDataLayout> layout =
                std::make_unique<storage::TarDataLayout>();
            boost::iostreams::mapped_file mapped_memory_file;
            util::mmapFile<char>(file.second, mapped_memory_file);
            mapped_memory_files.push_back(std::move(mapped_memory_file));
            storage::populateLayoutFromFile(file.second, *layout);
            allocated_regions.push_back({mapped_memory_file.data(), std::move(layout)});
        }
    }

    index = storage::SharedDataIndex{std::move(allocated_regions)};
}

MMapMemoryAllocator::~MMapMemoryAllocator() {}

const storage::SharedDataIndex &MMapMemoryAllocator::GetIndex() { return index; }

} // namespace datafacade
} // namespace engine
} // namespace osrm
