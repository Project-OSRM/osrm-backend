#ifndef OSRM_ENGINE_DATAFACADE_PROCESS_MEMORY_ALLOCATOR_HPP_
#define OSRM_ENGINE_DATAFACADE_PROCESS_MEMORY_ALLOCATOR_HPP_

#include "storage/storage_config.hpp"
#include "engine/datafacade/contiguous_block_allocator.hpp"

#include <memory>

namespace osrm
{
namespace engine
{
namespace datafacade
{

/**
 * This allocator uses a process-local memory block to load
 * data into.  The structure and layout is the same as when using
 * shared memory.
 * This class holds a unique_ptr to the memory block, so it
 * is auto-freed upon destruction.
 */
class ProcessMemoryAllocator : public ContiguousBlockAllocator
{
  public:
    explicit ProcessMemoryAllocator(const storage::StorageConfig &config);
    ~ProcessMemoryAllocator() override final;

    // interface to give access to the datafacades
    const storage::SharedDataIndex &GetIndex() override final;

  private:
    storage::SharedDataIndex index;
    std::unique_ptr<char[]> internal_memory;
};

} // namespace datafacade
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_DATAFACADE_PROCESS_MEMORY_ALLOCATOR_HPP_
