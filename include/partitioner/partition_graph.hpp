#ifndef OSRM_PARTITIONER_GRAPH_HPP_
#define OSRM_PARTITIONER_GRAPH_HPP_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <vector>

#include "util/typedefs.hpp"
#include <boost/range/iterator_range.hpp>

namespace osrm
{
namespace partitioner
{

// forward declaration to allow finding friends
template <typename NodeEntryT, typename EdgeEntryT> class RemappableGraph;

// wrapper for nodes to augment with a tag storing first edge id
template <typename Base> class NodeEntryWrapper : public Base
{
  public:
    template <typename... Args>
    NodeEntryWrapper(std::size_t edges_begin_, std::size_t edges_end_, Args &&... args)
        : Base(std::forward<Args>(args)...), edges_begin(edges_begin_), edges_end(edges_end_)
    {
    }

  private:
    // only to be modified by the graph itself
    std::size_t edges_begin;
    std::size_t edges_end;

    // give the graph access to the node data wrapper
    template <typename NodeEntryT, typename EdgeEntryT> friend class RemappableGraph;
};

using RemappableGraphNode = NodeEntryWrapper<struct zero_base_class>;

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

    auto BeginEdges(const NodeT &node) const { return edges.begin() + node.edges_begin; }
    auto EndEdges(const NodeT &node) const { return edges.begin() + node.edges_end; }
    auto BeginEdges(const NodeT &node) { return edges.begin() + node.edges_begin; }
    auto EndEdges(const NodeT &node) { return edges.begin() + node.edges_end; }

    EdgeID BeginEdgeID(const NodeID nid) const { return nodes[nid].edges_begin; }
    EdgeID EndEdgeID(const NodeID nid) const { return nodes[nid].edges_end; }

    // iterate over all nodes
    auto Nodes() { return boost::make_iterator_range(nodes.begin(), nodes.end()); }
    auto Nodes() const { return boost::make_iterator_range(nodes.begin(), nodes.end()); }

    NodeID GetID(const NodeT &node) const
    {
        BOOST_ASSERT(&node >= &nodes[0] && &node <= &nodes.back());
        return (&node - &nodes[0]);
    }
    EdgeID GetID(const EdgeT &edge) const
    {
        BOOST_ASSERT(&edge >= &edges[0] && &edge <= &edges.back());
        return (&edge - &edges[0]);
    }

    NodeIterator Begin() { return nodes.begin(); }
    NodeIterator End() { return nodes.end(); }
    ConstNodeIterator CBegin() const { return nodes.cbegin(); }
    ConstNodeIterator CEnd() const { return nodes.cend(); }

    // removes the edges from the graph that return true for the filter, returns new end
    template <typename FilterT> auto RemoveEdges(NodeT &node, FilterT filter)
    {
        BOOST_ASSERT(&node >= &nodes[0] && &node <= &nodes.back());
        // required since we are not on std++17 yet, otherwise we are missing an argument_type
        const auto negate_filter = [&](const EdgeT &edge) { return !filter(edge); };
        const auto center = std::stable_partition(BeginEdges(node), EndEdges(node), negate_filter);
        const auto remaining_edges = std::distance(BeginEdges(node), center);
        node.edges_end = node.edges_begin + remaining_edges;
        return center;
    }

  protected:
    std::vector<NodeT> nodes;
    std::vector<EdgeT> edges;
};

} // namespace partitioner
} // namespace osrm

#endif // OSRM_PARTITIONER_GRAPH_HPP_
