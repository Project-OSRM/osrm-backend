#ifndef DYNAMICGRAPH_HPP
#define DYNAMICGRAPH_HPP

#include "util/deallocating_vector.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/integer_range.hpp"
#include "util/permutation.hpp"
#include "util/typedefs.hpp"

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
namespace detail
{
// These types need to live outside of DynamicGraph
// to be not dependable. We need this for transforming graphs
// with different data.

template <typename EdgeIterator> struct DynamicNode
{
    // index of the first edge
    EdgeIterator first_edge;
    // amount of edges
    unsigned edges;
};

template <typename NodeIterator, typename EdgeDataT> struct DynamicEdge
{
    NodeIterator target;
    EdgeDataT data;
};
} // namespace detail

template <typename EdgeDataT> class DynamicGraph
{
  public:
    using EdgeData = EdgeDataT;
    using NodeIterator = std::uint32_t;
    using EdgeIterator = std::uint32_t;
    using EdgeRange = range<EdgeIterator>;

    using Node = detail::DynamicNode<EdgeIterator>;
    using Edge = detail::DynamicEdge<NodeIterator, EdgeDataT>;

    template <typename E> friend class DynamicGraph;

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

    DynamicGraph() : DynamicGraph(0) {}

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
        node_array.resize(number_of_nodes);
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

        BOOST_ASSERT(node_array.size() == number_of_nodes);
    }

    // Copy&move for the same data
    //

    DynamicGraph(const DynamicGraph &other)
    {
        number_of_nodes = other.number_of_nodes;
        // atomics can't be moved this is why we need an own constructor
        number_of_edges = static_cast<std::uint32_t>(other.number_of_edges);

        node_array = other.node_array;
        edge_list = other.edge_list;
    }

    DynamicGraph &operator=(const DynamicGraph &other)
    {
        auto copy_other = other;
        *this = std::move(other);
        return *this;
    }

    DynamicGraph(DynamicGraph &&other)
    {
        number_of_nodes = other.number_of_nodes;
        // atomics can't be moved this is why we need an own constructor
        number_of_edges = static_cast<std::uint32_t>(other.number_of_edges);

        node_array = std::move(other.node_array);
        edge_list = std::move(other.edge_list);
    }

    DynamicGraph &operator=(DynamicGraph &&other)
    {
        number_of_nodes = other.number_of_nodes;
        // atomics can't be moved this is why we need an own constructor
        number_of_edges = static_cast<std::uint32_t>(other.number_of_edges);

        node_array = std::move(other.node_array);
        edge_list = std::move(other.edge_list);

        return *this;
    }

    // Removes all edges to and from nodes for which filter(node_id) returns false
    template <typename Pred> auto Filter(Pred filter) const &
    {
        BOOST_ASSERT(node_array.size() == number_of_nodes);

        DynamicGraph other;

        other.number_of_nodes = number_of_nodes;
        other.number_of_edges = static_cast<std::uint32_t>(number_of_edges);
        other.edge_list.reserve(edge_list.size());
        other.node_array.resize(node_array.size());

        NodeID node_id = 0;
        std::transform(
            node_array.begin(), node_array.end(), other.node_array.begin(), [&](const Node &node) {
                const EdgeIterator first_edge = other.edge_list.size();

                BOOST_ASSERT(node_id < number_of_nodes);
                if (filter(node_id++))
                {
                    std::copy_if(edge_list.begin() + node.first_edge,
                                 edge_list.begin() + node.first_edge + node.edges,
                                 std::back_inserter(other.edge_list),
                                 [&](const auto &edge) { return filter(edge.target); });
                    const unsigned num_edges = other.edge_list.size() - first_edge;
                    return Node{first_edge, num_edges};
                }
                else
                {
                    return Node{first_edge, 0};
                }
            });

        return other;
    }

    unsigned GetNumberOfNodes() const { return number_of_nodes; }

    unsigned GetNumberOfEdges() const { return number_of_edges; }
    auto GetEdgeCapacity() const { return edge_list.size(); }

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

    void Renumber(const std::vector<NodeID> &old_to_new_node)
    {
        // permutate everything but the sentinel
        util::inplacePermutation(node_array.begin(), node_array.end(), old_to_new_node);

        // Build up edge permutation
        if (edge_list.size() >= std::numeric_limits<EdgeID>::max())
        {
            throw util::exception("There are too many edges, OSRM only supports 2^32" + SOURCE_REF);
        }

        EdgeID new_edge_index = 0;
        std::vector<EdgeID> old_to_new_edge(edge_list.size(), SPECIAL_EDGEID);
        for (auto node : util::irange<NodeID>(0, number_of_nodes))
        {
            auto new_first_edge = new_edge_index;
            // move all filled edges
            for (auto edge : GetAdjacentEdgeRange(node))
            {
                edge_list[edge].target = old_to_new_node[edge_list[edge].target];
                BOOST_ASSERT(edge_list[edge].target != SPECIAL_NODEID);
                old_to_new_edge[edge] = new_edge_index++;
            }
            node_array[node].first_edge = new_first_edge;
        }
        auto number_of_valid_edges = new_edge_index;

        // move all dummy edges to the end of the renumbered range
        for (auto edge : util::irange<NodeID>(0, edge_list.size()))
        {
            if (old_to_new_edge[edge] == SPECIAL_EDGEID)
            {
                BOOST_ASSERT(isDummy(edge));
                old_to_new_edge[edge] = new_edge_index++;
            }
        }
        BOOST_ASSERT(std::find(old_to_new_edge.begin(), old_to_new_edge.end(), SPECIAL_EDGEID) ==
                     old_to_new_edge.end());
        util::inplacePermutation(edge_list.begin(), edge_list.end(), old_to_new_edge);
        // Remove useless dummy nodes at the end
        edge_list.resize(number_of_valid_edges);
        number_of_edges = number_of_valid_edges;
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

    NodeIterator number_of_nodes;
    std::atomic_uint number_of_edges;

    std::vector<Node> node_array;
    DeallocatingVector<Edge> edge_list;
};
} // namespace util
} // namespace osrm

#endif // DYNAMICGRAPH_HPP
