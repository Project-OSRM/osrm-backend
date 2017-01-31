#ifndef OSRM_PARTITION_GRAPH_HPP_
#define OSRM_PARTITION_GRAPH_HPP_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

#include "util/typedefs.hpp"
#include <boost/range/iterator_range.hpp>

namespace osrm
{
namespace partition
{

// wrapper for nodes to augment with a tag storing first edge id
template <typename Base> class NodeEntryWrapper : public Base
{
  public:
    template <typename... Args>
    NodeEntryWrapper(std::size_t edges_begin_, std::size_t edges_end_, Args &&... args)
        : Base(std::forward<Args>(args)...), edges_begin(edges_begin_), edges_end(edges_end_)
    {
    }

    std::size_t edges_begin;
    std::size_t edges_end;
};

template <typename Base> class GraphConstructionWrapper : public Base
{
  public:
    template <typename... Args>
    GraphConstructionWrapper(const NodeID source_, Args &&... args)
        : Base(std::forward<Args>(args)...), source(source_)
    {
    }

    NodeID source;

    Base Reduce() const { return *this; }
};

template <typename RandomIt> void groupEdgesBySource(RandomIt first, RandomIt last)
{
    std::sort(
        first, last, [](const auto &lhs, const auto &rhs) { return lhs.source < rhs.source; });
}

template <typename NodeEntryT, typename EdgeEntryT> class RemappableGraph
{
  public:
    using NodeT = NodeEntryT;
    using EdgeT = EdgeEntryT;

    using NodeIterator = typename std::vector<NodeT>::iterator;
    using ConstNodeIterator = typename std::vector<NodeT>::const_iterator;
    using EdgeIterator = typename std::vector<EdgeT>::iterator;
    using ConstEdgeIterator = typename std::vector<EdgeT>::const_iterator;

    // Constructs an empty graph with a given number of nodes.
    explicit RemappableGraph(std::vector<NodeT> nodes_, std::vector<EdgeT> edges_)
        : nodes(std::move(nodes_)), edges(std::move(edges_))
    {
    }

    unsigned NumberOfNodes() const { return nodes.size(); }

    auto &Node(const NodeID nid) { return nodes[nid]; }
    auto &Node(const NodeID nid) const { return nodes[nid]; }

    auto &Edge(const EdgeID eid) { return edges[eid]; }
    auto &Edge(const EdgeID eid) const { return edges[eid]; }

    auto Edges(const NodeID nid)
    {
        return boost::make_iterator_range(edges.begin() + nodes[nid].edges_begin,
                                          edges.begin() + nodes[nid].edges_end);
    }

    auto Edges(const NodeID nid) const
    {
        return boost::make_iterator_range(edges.begin() + nodes[nid].edges_begin,
                                          edges.begin() + nodes[nid].edges_end);
    }

    auto Edges(const NodeT &node)
    {
        return boost::make_iterator_range(edges.begin() + node.edges_begin,
                                          edges.begin() + node.edges_end);
    }

    auto Edges(const NodeT &node) const
    {
        return boost::make_iterator_range(edges.begin() + node.edges_begin,
                                          edges.begin() + node.edges_end);
    }

    auto BeginEdges(const NodeID nid) const { return edges.begin() + nodes[nid].edges_begin; }
    auto EndEdges(const NodeID nid) const { return edges.begin() + nodes[nid].edges_end; }

    NodeIterator Begin() { return nodes.begin(); }
    NodeIterator End() { return nodes.end(); }
    ConstNodeIterator CBegin() const { return nodes.cbegin(); }
    ConstNodeIterator CEnd() const { return nodes.cend(); }

  protected:
    std::vector<NodeT> nodes;
    std::vector<EdgeT> edges;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_GRAPH_HPP_
