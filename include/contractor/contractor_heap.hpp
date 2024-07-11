#ifndef OSRM_CONTRACTOR_CONTRACTOR_HEAP_HPP_
#define OSRM_CONTRACTOR_CONTRACTOR_HEAP_HPP_

#include "util/query_heap.hpp"
#include "util/typedefs.hpp"
#include "util/xor_fast_hash_storage.hpp"

namespace osrm::contractor
{
struct ContractorHeapData
{
    ContractorHeapData() {}
    ContractorHeapData(short hop_, bool target_) : hop(hop_), target(target_) {}

    short hop = 0;
    bool target = false;
};

using ContractorHeap = util::QueryHeap<NodeID,
                                       NodeID,
                                       EdgeWeight,
                                       ContractorHeapData,
                                       util::XORFastHashStorage<NodeID, NodeID>>;

} // namespace osrm::contractor

#endif // OSRM_CONTRACTOR_CONTRACTOR_HEAP_HPP_
