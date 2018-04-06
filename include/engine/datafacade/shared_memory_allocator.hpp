#ifndef OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_
#define OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_

#include "engine/datafacade/contiguous_block_allocator.hpp"

#include "storage/shared_data_index.hpp"
#include "storage/shared_memory.hpp"

#include <memory>

namespace osrm
{
namespace engine
{
namespace datafacade
{

/**
 * This allocator uses an IPC shared memory block as the data location.
 * Many SharedMemoryDataFacade objects can be created that point to the same shared
 * memory block.
 */
class SharedMemoryAllocator : public ContiguousBlockAllocator
{
  public:
    explicit SharedMemoryAllocator(
        const std::vector<storage::SharedRegionRegister::ShmKey> &shm_keys);
    ~SharedMemoryAllocator() override final;

    // interface to give access to the datafacades
    const storage::SharedDataIndex &GetIndex() override final;

  private:
    storage::SharedDataIndex index;
    std::vector<std::unique_ptr<storage::SharedMemory>> memory_regions;
};

} // namespace datafacade
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_
