#ifndef OSRM_PARTITION_RECURSIVE_BISECTION_STATE_HPP_
#define OSRM_PARTITION_RECURSIVE_BISECTION_STATE_HPP_

#include <cstddef>
#include <vector>

#include "partition/bisection_graph.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace partition
{

// The recursive state describes the relation between our global graph and the recursive state in a
// recursive bisection.
// 
// We describe the state with the use of two arrays:
// 
// The id-arrays keeps nodes in order, based on their partitioning. Initially, it contains all nodes
// in sorted order:
// 
// ids: [0,1,2,3,4,5,6,7,8,9]
// 
// When partitioned (for this example we use even : odd), the arrays is re-ordered to group all
// elements within a single cell of the partition:
// 
// ids: [0,2,4,6,8,1,3,5,7,9]
//
// This can be performed recursively by interpreting the arrays [0,2,4,6,8] and [1,3,5,7,9] as new
// input.
//
// The partitioning array describes all results of the partitioning in form of `left` or `right`.
// It is a sequence of bits for every id that describes if it is moved to the `front` or `back`
// during the split of the ID array. In the example, we would get the result
// 
// bisection-ids: [0,1,0,1,0,1,0,1,0,1]
// 
// Further partitioning [0,2,4,6,8] into [0,4,8], [2,6] and [1,3,5,7,9] into [1,3,9] and [5,7]
// the result would be:
// 
// bisection-ids: [00,10,01,10,00,11,01,11,00,10]

/* Written out in a recursive tree form:

 ids:     [0,1,2,3,4,5,6,7,8,9]
 mask:    [0,1,0,1,0,1,0,1,0,1]
              /          \
 ids:    [0,2,4,6,8] [1,3,5,7,9]
 mask:   [0,1,0,1,0] [0,0,1,1,0]
           /    \      /    \
 ids:  [0,4,8] [2,6] [1,3,9] [5,7]

 The bisection ids then trace the path (left: 0, right: 1) through the tree:

 ids:     [0,  1,  2,  3,  4,  5,  6,  7,  8,  9 ]
 path:    [00, 10, 01, 10, 00, 11, 01, 11, 00, 10]

*/
class RecursiveBisectionState
{
  public:
    // The ID in the partition array
    using BisectionID = std::uint32_t;
    using IDIterator = std::vector<NodeID>::const_iterator;

    RecursiveBisectionState(const BisectionGraph &bisection_graph);
    ~RecursiveBisectionState();

    BisectionID GetBisectionID(const NodeID nid) const;

    // Bisects the node id array's sub-range based on the partition mask.
    // Returns: partition point of the bisection: iterator to the second group's first element.
    IDIterator ApplyBisection(const IDIterator begin,
                              const IDIterator end,
                              const std::vector<bool> &partition);

    const IDIterator Begin() const;
    const IDIterator End() const;

  private:
    const BisectionGraph &bisection_graph;

    std::vector<NodeID> id_array;
    std::vector<BisectionID> bisection_ids;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_RECURSIVE_BISECTION_STATE_HPP_
