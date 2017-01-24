#ifndef OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_
#define OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_

#include "engine/datafacade/contiguous_block_allocator.hpp"

#include "storage/shared_datatype.hpp"
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
    explicit SharedMemoryAllocator(storage::SharedDataType data_region);
    ~SharedMemoryAllocator() override final;

    // interface to give access to the datafacades
    storage::DataLayout &GetLayout() override final;
    char *GetMemory() override final;

  private:
    std::unique_ptr<storage::SharedMemory> m_large_memory;
};

} // namespace datafacade
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_DATAFACADE_SHARED_MEMORY_ALLOCATOR_HPP_
