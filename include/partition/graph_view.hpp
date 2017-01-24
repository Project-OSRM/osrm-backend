#ifndef OSRM_PARTITION_GRAPHVIEW_HPP_
#define OSRM_PARTITION_GRAPHVIEW_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <cstddef>

namespace osrm
{
namespace partition
{

// Predicate for EdgeIDs checking their partition ids for equality.
// Used in filter iterator below to discard edges in different partitions.
struct HasSamePartitionID
{
    HasSamePartitionID(const RecursiveBisectionState::BisectionID bisection_id,
                       const BisectionGraph &bisection_graph,
                       const RecursiveBisectionState &recursive_bisection_state);

    bool operator()(const EdgeID eid) const;

  private:
    const RecursiveBisectionState::BisectionID bisection_id;
    const BisectionGraph &bisection_graph;
    const RecursiveBisectionState &recursive_bisection_state;
};

// Random Access Iterator on top of contiguous integral EdgeIDs
class EdgeIDIterator : public boost::iterator_facade<EdgeIDIterator,
                                                     EdgeID const,
                                                     boost::random_access_traversal_tag>
{
  public:
    EdgeIDIterator() : position(SPECIAL_EDGEID) {}
    explicit EdgeIDIterator(EdgeID position_) : position(position_) {}

  private:
    friend class boost::iterator_core_access;

    // Implements the facade's core operations required for random access iterators:
    // http://www.boost.org/doc/libs/1_63_0/libs/iterator/doc/iterator_facade.html#core-operations

    void increment() { ++position; }
    void decrement() { --position; }
    void advance(difference_type offset) { position += offset; }
    bool equal(const EdgeIDIterator &other) const { return position == other.position; }
    const reference dereference() const { return position; }
    difference_type distance_to(const EdgeIDIterator &other) const
    {
        return static_cast<difference_type>(other.position - position);
    }

    value_type position;
};

// Non-owning immutable sub-graph view into a base graph.
// The part of the graph to select is determined by the recursive bisection state.
class GraphView
{
  public:
    using EdgeIterator = boost::filter_iterator<HasSamePartitionID, EdgeIDIterator>;

    GraphView(const BisectionGraph &graph,
              const RecursiveBisectionState &bisection_state,
              const RecursiveBisectionState::IDIterator begin,
              const RecursiveBisectionState::IDIterator end);

    // Number of nodes _in this sub-graph_.
    std::size_t NumberOfNodes() const;

    RecursiveBisectionState::IDIterator Begin() const;
    RecursiveBisectionState::IDIterator End() const;

    EdgeIterator EdgeBegin(const NodeID nid) const;
    EdgeIterator EdgeEnd(const NodeID nid) const;

  private:
    const BisectionGraph &bisection_graph;
    const RecursiveBisectionState &bisection_state;

    const RecursiveBisectionState::IDIterator begin;
    const RecursiveBisectionState::IDIterator end;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_GRAPHVIEW_HPP_
