#ifndef DYNAMICGRAPH_HPP
#define DYNAMICGRAPH_HPP

#include "util/deallocating_vector.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

#include "storage/io_fwd.hpp"

#include <boost/assert.hpp>

#include <cstdint>

#include <algorithm>
#include <atomic>
#include <limits>
#include <tuple>
#include <vector>

namespace osrm
{
namespace util
{
template <typename EdgeDataT> class DynamicGraph;

namespace serialization
{
template <typename EdgeDataT, bool UseSharedMemory>
void read(storage::io::FileReader &reader, DynamicGraph<EdgeDataT> &graph);

template <typename EdgeDataT, bool UseSharedMemory>
void write(storage::io::FileWriter &writer, const DynamicGraph<EdgeDataT> &graph);
}

template <typename EdgeDataT> class DynamicGraph
{
  public:
    using EdgeData = EdgeDataT;
    using NodeIterator = std::uint32_t;
    using EdgeIterator = std::uint32_t;
    using EdgeRange = range<EdgeIterator>;

    class InputEdge
    {
      public:
        NodeIterator source;
        NodeIterator target;
        EdgeDataT data;

        InputEdge()
            : source(std::numeric_limits<NodeIterator>::max()),
              target(std::numeric_limits<NodeIterator>::max())
        {
        }

        template <typename... Ts>
        InputEdge(NodeIterator source, NodeIterator target, Ts &&... data)
            : source(source), target(target), data(std::forward<Ts>(data)...)
        {
        }

        bool operator<(const InputEdge &rhs) const
        {
            return std::tie(source, target) < std::tie(rhs.source, rhs.target);
        }
    };

    // Constructs an empty graph with a given number of nodes.
    explicit DynamicGraph(NodeIterator nodes) : number_of_nodes(nodes), number_of_edges(0)
    {
        node_array.reserve(number_of_nodes);
        node_array.resize(number_of_nodes);

        edge_list.reserve(number_of_nodes * 1.1);
        edge_list.resize(number_of_nodes);
    }

    /**
     * Constructs a DynamicGraph from a list of edges sorted by source node id.
     */
    template <class ContainerT> DynamicGraph(const NodeIterator nodes, const ContainerT &graph)
    {
        // we need to cast here because DeallocatingVector does not have a valid const iterator
        BOOST_ASSERT(std::is_sorted(const_cast<ContainerT &>(graph).begin(),
                                    const_cast<ContainerT &>(graph).end()));

        number_of_nodes = nodes;
        number_of_edges = static_cast<EdgeIterator>(graph.size());
        node_array.resize(number_of_nodes + 1);
        EdgeIterator edge = 0;
        EdgeIterator position = 0;
        for (const auto node : irange(0u, number_of_nodes))
        {
            EdgeIterator last_edge = edge;
            while (edge < number_of_edges && graph[edge].source == node)
            {
                ++edge;
            }
            node_array[node].first_edge = position;
            node_array[node].edges = edge - last_edge;
            position += node_array[node].edges;
        }
        node_array.back().first_edge = position;
        edge_list.reserve(static_cast<std::size_t>(edge_list.size() * 1.1));
        edge_list.resize(position);
        edge = 0;
        for (const auto node : irange(0u, number_of_nodes))
        {
            for (const auto i : irange(node_array[node].first_edge,
                                       node_array[node].first_edge + node_array[node].edges))
            {
                edge_list[i].target = graph[edge].target;
                BOOST_ASSERT(edge_list[i].target < number_of_nodes);
                edge_list[i].data = graph[edge].data;
                ++edge;
            }
        }
    }

    unsigned GetNumberOfNodes() const { return number_of_nodes; }

    unsigned GetNumberOfEdges() const { return number_of_edges; }

    unsigned GetOutDegree(const NodeIterator n) const { return node_array[n].edges; }

    unsigned GetDirectedOutDegree(const NodeIterator n) const
    {
        unsigned degree = 0;
        for (const auto edge : irange(BeginEdges(n), EndEdges(n)))
        {
            if (!GetEdgeData(edge).reversed)
            {
                ++degree;
            }
        }
        return degree;
    }

    NodeIterator GetTarget(const EdgeIterator e) const { return NodeIterator(edge_list[e].target); }

    void SetTarget(const EdgeIterator e, const NodeIterator n) { edge_list[e].target = n; }

    EdgeDataT &GetEdgeData(const EdgeIterator e) { return edge_list[e].data; }

    const EdgeDataT &GetEdgeData(const EdgeIterator e) const { return edge_list[e].data; }

    EdgeIterator BeginEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array[n].first_edge);
    }

    EdgeIterator EndEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array[n].first_edge + node_array[n].edges);
    }

    EdgeRange GetAdjacentEdgeRange(const NodeIterator node) const
    {
        return irange(BeginEdges(node), EndEdges(node));
    }

    NodeIterator InsertNode()
    {
        node_array.emplace_back(node_array.back());
        number_of_nodes += 1;

        return number_of_nodes;
    }

    // adds an edge. Invalidates edge iterators for the source node
    EdgeIterator InsertEdge(const NodeIterator from, const NodeIterator to, const EdgeDataT &data)
    {
        Node &node = node_array[from];
        EdgeIterator one_beyond_last_of_node = node.edges + node.first_edge;
        // if we can't write at the end of this nodes edges
        // that is: the end is the end of the edge_list,
        //          or the beginning of the next nodes edges
        if (one_beyond_last_of_node == edge_list.size() || !isDummy(one_beyond_last_of_node))
        {
            // can we write before this nodes edges?
            if (node.first_edge != 0 && isDummy(node.first_edge - 1))
            {
                node.first_edge--;
                edge_list[node.first_edge] = edge_list[node.first_edge + node.edges];
            }
            else
            {
                // we have to move this nodes edges to the end of the edge_list
                EdgeIterator newFirstEdge = (EdgeIterator)edge_list.size();
                unsigned newSize = node.edges * 1.1 + 2;
                EdgeIterator requiredCapacity = newSize + edge_list.size();
                EdgeIterator oldCapacity = edge_list.capacity();
                // make sure there is enough space at the end
                if (requiredCapacity >= oldCapacity)
                {
                    edge_list.reserve(requiredCapacity * 1.1);
                }
                edge_list.resize(edge_list.size() + newSize);
                // move the edges over and invalidate the old ones
                for (const auto i : irange(0u, node.edges))
                {
                    edge_list[newFirstEdge + i] = edge_list[node.first_edge + i];
                    makeDummy(node.first_edge + i);
                }
                // invalidate until the end of edge_list
                for (const auto i : irange(node.edges + 1, newSize))
                {
                    makeDummy(newFirstEdge + i);
                }
                node.first_edge = newFirstEdge;
            }
        }
        // get the position for the edge that is to be inserted
        // and write it
        Edge &edge = edge_list[node.first_edge + node.edges];
        edge.target = to;
        edge.data = data;
        ++number_of_edges;
        ++node.edges;
        return EdgeIterator(node.first_edge + node.edges);
    }

    // removes an edge. Invalidates edge iterators for the source node
    void DeleteEdge(const NodeIterator source, const EdgeIterator e)
    {
        Node &node = node_array[source];
        --number_of_edges;
        --node.edges;
        BOOST_ASSERT(std::numeric_limits<unsigned>::max() != node.edges);
        const unsigned last = node.first_edge + node.edges;
        BOOST_ASSERT(std::numeric_limits<unsigned>::max() != last);
        // swap with last edge
        edge_list[e] = edge_list[last];
        makeDummy(last);
    }

    // removes all edges (source,target)
    int32_t DeleteEdgesTo(const NodeIterator source, const NodeIterator target)
    {
        int32_t deleted = 0;
        for (EdgeIterator i = BeginEdges(source), iend = EndEdges(source); i < iend - deleted; ++i)
        {
            if (edge_list[i].target == target)
            {
                do
                {
                    deleted++;
                    edge_list[i] = edge_list[iend - deleted];
                    makeDummy(iend - deleted);
                } while (i < iend - deleted && edge_list[i].target == target);
            }
        }

        number_of_edges -= deleted;
        node_array[source].edges -= deleted;

        return deleted;
    }

    // searches for a specific edge
    EdgeIterator FindEdge(const NodeIterator from, const NodeIterator to) const
    {
        for (const auto i : irange(BeginEdges(from), EndEdges(from)))
        {
            if (to == edge_list[i].target)
            {
                return i;
            }
        }
        return SPECIAL_EDGEID;
    }

    // searches for a specific edge
    EdgeIterator FindSmallestEdge(const NodeIterator from, const NodeIterator to) const
    {
        EdgeIterator smallest_edge = SPECIAL_EDGEID;
        EdgeWeight smallest_weight = INVALID_EDGE_WEIGHT;
        for (auto edge : GetAdjacentEdgeRange(from))
        {
            const NodeID target = GetTarget(edge);
            const EdgeWeight weight = GetEdgeData(edge).distance;
            if (target == to && weight < smallest_weight)
            {
                smallest_edge = edge;
                smallest_weight = weight;
            }
        }
        return smallest_edge;
    }

    EdgeIterator FindEdgeInEitherDirection(const NodeIterator from, const NodeIterator to) const
    {
        EdgeIterator tmp = FindEdge(from, to);
        return (SPECIAL_NODEID != tmp ? tmp : FindEdge(to, from));
    }

    EdgeIterator
    FindEdgeIndicateIfReverse(const NodeIterator from, const NodeIterator to, bool &result) const
    {
        EdgeIterator current_iterator = FindEdge(from, to);
        if (SPECIAL_NODEID == current_iterator)
        {
            current_iterator = FindEdge(to, from);
            if (SPECIAL_NODEID != current_iterator)
            {
                result = true;
            }
        }
        return current_iterator;
    }

  protected:
    bool isDummy(const EdgeIterator edge) const
    {
        return edge_list[edge].target == (std::numeric_limits<NodeIterator>::max)();
    }

    void makeDummy(const EdgeIterator edge)
    {
        edge_list[edge].target = (std::numeric_limits<NodeIterator>::max)();
    }

    struct Node
    {
        // index of the first edge
        EdgeIterator first_edge;
        // amount of edges
        unsigned edges;
    };

    struct Edge
    {
        NodeIterator target;
        EdgeDataT data;
    };

    NodeIterator number_of_nodes;
    std::atomic_uint number_of_edges;

    std::vector<Node> node_array;
    DeallocatingVector<Edge> edge_list;
};
}
}

#endif // DYNAMICGRAPH_HPP
