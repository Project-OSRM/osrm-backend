#ifndef OSRM_ENGINE_DATAFACADE_CONTIGUOUS_BLOCK_ALLOCATOR_HPP_
#define OSRM_ENGINE_DATAFACADE_CONTIGUOUS_BLOCK_ALLOCATOR_HPP_

#include "storage/shared_data_index.hpp"

namespace osrm::engine::datafacade
{

class ContiguousBlockAllocator
{
  public:
    virtual ~ContiguousBlockAllocator() = default;

    // interface to give access to the datafacades
    virtual const storage::SharedDataIndex &GetIndex() = 0;
};

} // namespace osrm::engine::datafacade

#endif // OSRM_ENGINE_DATAFACADE_CONTIGUOUS_BLOCK_ALLOCATOR_HPP_
