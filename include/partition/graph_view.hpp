#ifndef OSRM_PARTITION_GRAPHVIEW_HPP_
#define OSRM_PARTITION_GRAPHVIEW_HPP_

#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection_state.hpp"

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace osrm
{
namespace partition
{

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

// a wrapper around the EdgeIDs returned by the static graph to make them iterable
class EdgeIDIterator : public boost::iterator_facade<EdgeIDIterator,
                                                     EdgeID const,
                                                     boost::random_access_traversal_tag>
{
    using Base =
        boost::iterator_facade<EdgeIDIterator, EdgeID const, boost::random_access_traversal_tag>;

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

class GraphView
{
  public:
    using EdgeIterator = boost::filter_iterator<HasSamePartitionID, EdgeIDIterator>;

    GraphView(const BisectionGraph &graph,
              const RecursiveBisectionState &bisection_state,
              const RecursiveBisectionState::IDIterator begin,
              const RecursiveBisectionState::IDIterator end);

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
