#ifndef OSRM_ENGINE_DATAFACADE_MMAP_MEMORY_ALLOCATOR_HPP_
#define OSRM_ENGINE_DATAFACADE_MMAP_MEMORY_ALLOCATOR_HPP_

#include "engine/datafacade/contiguous_block_allocator.hpp"

#include "storage/storage_config.hpp"

#include "util/vector_view.hpp"

#include <boost/iostreams/device/mapped_file.hpp>

#include <memory>
#include <string>

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
    explicit MMapMemoryAllocator(const storage::StorageConfig &config);
    ~MMapMemoryAllocator() override final;

    // interface to give access to the datafacades
    const storage::SharedDataIndex &GetIndex() override final;

  private:
    storage::SharedDataIndex index;
    std::vector<boost::iostreams::mapped_file> mapped_memory_files;
    std::string rtree_filename;
};

} // namespace datafacade
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_
