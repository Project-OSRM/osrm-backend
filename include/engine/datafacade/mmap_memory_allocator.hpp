#ifndef OSRM_ENGINE_DATAFACADE_MMAP_MEMORY_ALLOCATOR_HPP_
#define OSRM_ENGINE_DATAFACADE_MMAP_MEMORY_ALLOCATOR_HPP_

#include "engine/datafacade/contiguous_block_allocator.hpp"

#include "storage/storage_config.hpp"

#include "util/vector_view.hpp"

#include <boost/iostreams/device/mapped_file.hpp>

#include <memory>

namespace osrm
{
namespace engine
{
namespace datafacade
{

/**
 * This allocator uses file backed mmap memory block as the data location.
 */
class MMapMemoryAllocator : public ContiguousBlockAllocator
{
  public:
    explicit MMapMemoryAllocator(const storage::StorageConfig &config,
                                 const boost::filesystem::path &memory_file);
    ~MMapMemoryAllocator() override final;

    // interface to give access to the datafacades
    storage::DataLayout &GetLayout() override final;
    char *GetMemory() override final;

  private:
    storage::DataLayout *data_layout;
    util::vector_view<char> mapped_memory;
    boost::iostreams::mapped_file mapped_memory_file;
};

} // namespace datafacade
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_
