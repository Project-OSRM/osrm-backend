#ifndef OSRM_CONTRACTOR_CONTRACTOR_HEAP_HPP_
#define OSRM_CONTRACTOR_CONTRACTOR_HEAP_HPP_

#include "util/linear_hash_storage.hpp"
#include "util/query_heap.hpp"
#include "util/typedefs.hpp"

namespace osrm::contractor
{
// Data: if true signals that node is a target
using ContractorHeap =
    util::QueryHeap<NodeID, NodeID, EdgeWeight, bool, util::LinearHashStorage<NodeID, NodeID>>;

} // namespace osrm::contractor

#endif // OSRM_CONTRACTOR_CONTRACTOR_HEAP_HPP_
