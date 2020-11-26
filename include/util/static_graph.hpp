#ifndef STATIC_GRAPH_HPP
#define STATIC_GRAPH_HPP

#include "util/graph_traits.hpp"
#include "util/integer_range.hpp"
#include "util/percent.hpp"
#include "util/permutation.hpp"
#include "util/typedefs.hpp"
#include "util/vector_view.hpp"

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace osrm
{
namespace util
{
template <typename EdgeDataT, storage::Ownership Ownership> class StaticGraph;

namespace serialization
{
template <typename EdgeDataT, storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          StaticGraph<EdgeDataT, Ownership> &graph);

template <typename EdgeDataT, storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const StaticGraph<EdgeDataT, Ownership> &graph);
} // namespace serialization

namespace static_graph_details
{

using NodeIterator = NodeID;
using EdgeIterator = NodeID;

struct NodeArrayEntry
{
    // index of the first edge
    EdgeIterator first_edge;
};

template <typename EdgeDataT> struct EdgeArrayEntry;

template <typename EdgeDataT> struct EdgeArrayEntry
{
    NodeID target;
    EdgeDataT data;
};

template <> struct EdgeArrayEntry<void>
{
    NodeID target;
};

template <typename EdgeDataT> struct SortableEdgeWithData;

template <> struct SortableEdgeWithData<void>
{
    NodeIterator source;
    NodeIterator target;

    SortableEdgeWithData() = default;

    SortableEdgeWithData(NodeIterator source, NodeIterator target) : source(source), target(target)
    {
    }

    bool operator<(const SortableEdgeWithData &right) const
    {
        return std::tie(source, target) < std::tie(right.source, right.target);
    }

    bool operator==(const SortableEdgeWithData &right) const
    {
        return std::tie(source, target) == std::tie(right.source, right.target);
    }
};

template <typename EdgeDataT> struct SortableEdgeWithData : SortableEdgeWithData<void>
{
    using Base = SortableEdgeWithData<void>;

    EdgeDataT data;

    SortableEdgeWithData() = default;

    template <typename... Ts>
    SortableEdgeWithData(NodeIterator source, NodeIterator target, Ts &&... data)
        : Base{source, target}, data{std::forward<Ts>(data)...}
    {
    }
};

template <typename EntryT, typename OtherEdge>
EntryT edgeToEntry(const OtherEdge &from, std::true_type)
{
    return EntryT{from.target, from.data};
}
template <typename EntryT, typename OtherEdge>
EntryT edgeToEntry(const OtherEdge &from, std::false_type)
{
    return EntryT{from.target};
}

} // namespace static_graph_details

template <typename EdgeDataT, storage::Ownership Ownership = storage::Ownership::Container>
class StaticGraph
{
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

  public:
    using InputEdge = static_graph_details::SortableEdgeWithData<EdgeDataT>;
    using NodeIterator = static_graph_details::NodeIterator;
    using EdgeIterator = static_graph_details::EdgeIterator;
    using EdgeRange = range<EdgeIterator>;
    using NodeArrayEntry = static_graph_details::NodeArrayEntry;
    using EdgeArrayEntry = static_graph_details::EdgeArrayEntry<EdgeDataT>;

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const
    {
        return irange(BeginEdges(node), EndEdges(node));
    }

    StaticGraph() {}

    template <typename ContainerT> StaticGraph(const std::uint32_t nodes, const ContainerT &edges)
    {
        BOOST_ASSERT(std::is_sorted(const_cast<ContainerT &>(edges).begin(),
                                    const_cast<ContainerT &>(edges).end()));

        InitializeFromSortedEdgeRange(nodes, edges.begin(), edges.end());
    }

    StaticGraph(Vector<NodeArrayEntry> node_array_, Vector<EdgeArrayEntry> edge_array_)
        : node_array(std::move(node_array_)), edge_array(std::move(edge_array_))
    {
        BOOST_ASSERT(!node_array.empty());

        number_of_nodes = static_cast<decltype(number_of_nodes)>(node_array.size() - 1);
        number_of_edges = static_cast<decltype(number_of_edges)>(node_array.back().first_edge);
        BOOST_ASSERT(number_of_edges <= edge_array.size());
        BOOST_ASSERT(number_of_nodes == node_array.size() - 1);
    }

    unsigned GetNumberOfNodes() const { return number_of_nodes; }

    unsigned GetNumberOfEdges() const { return number_of_edges; }

    unsigned GetOutDegree(const NodeIterator n) const { return EndEdges(n) - BeginEdges(n); }

    inline NodeIterator GetTarget(const EdgeIterator e) const
    {
        return NodeIterator(edge_array[e].target);
    }

    auto &GetEdgeData(const EdgeIterator e) { return edge_array[e].data; }

    const auto &GetEdgeData(const EdgeIterator e) const { return edge_array[e].data; }

    EdgeIterator BeginEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array.at(n).first_edge);
    }

    EdgeIterator EndEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array.at(n + 1).first_edge);
    }

    // searches for a specific edge
    EdgeIterator FindEdge(const NodeIterator from, const NodeIterator to) const
    {
        for (const auto i : irange(BeginEdges(from), EndEdges(from)))
        {
            if (to == edge_array[i].target)
            {
                return i;
            }
        }
        return SPECIAL_EDGEID;
    }

    /**
     * Finds the edge with the smallest `.weight` going from `from` to `to`
     * @param from the source node ID
     * @param to the target node ID
     * @param filter a functor that returns a `bool` that determines whether an edge should be
     * tested or not.
     *   Takes `EdgeData` as a parameter.
     * @return the ID of the smallest edge if any were found that satisfied *filter*, or
     * `SPECIAL_EDGEID` if no
     *   matching edge is found.
     */
    template <typename FilterFunction>
    EdgeIterator
    FindSmallestEdge(const NodeIterator from, const NodeIterator to, FilterFunction &&filter) const
    {
        static_assert(traits::HasDataMember<EdgeArrayEntry>::value,
                      "Filtering on .data not possible without .data member attribute");

        EdgeIterator smallest_edge = SPECIAL_EDGEID;
        EdgeWeight smallest_weight = INVALID_EDGE_WEIGHT;
        for (auto edge : GetAdjacentEdgeRange(from))
        {
            const NodeID target = GetTarget(edge);
            const auto &data = GetEdgeData(edge);
            if (target == to && data.weight < smallest_weight &&
                std::forward<FilterFunction>(filter)(data))
            {
                smallest_edge = edge;
                smallest_weight = data.weight;
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
        std::vector<NodeID> new_to_old_node(number_of_nodes);
        for (auto node : util::irange<NodeID>(0, number_of_nodes))
            new_to_old_node[old_to_new_node[node]] = node;

        Vector<NodeArrayEntry> new_node_array(node_array.size());

        // Build up edge permutation
        auto new_edge_index = 0;
        std::vector<EdgeID> old_to_new_edge(edge_array.size(), SPECIAL_EDGEID);
        for (auto node : util::irange<NodeID>(0, number_of_nodes))
        {
            auto new_first_edge = new_edge_index;
            for (auto edge : GetAdjacentEdgeRange(new_to_old_node[node]))
            {
                edge_array[edge].target = old_to_new_node[edge_array[edge].target];
                old_to_new_edge[edge] = new_edge_index++;
            }
            new_node_array[node].first_edge = new_first_edge;
        }
        new_node_array.back().first_edge = new_edge_index;
        node_array = std::move(new_node_array);
        BOOST_ASSERT(std::find(old_to_new_edge.begin(), old_to_new_edge.end(), SPECIAL_EDGEID) ==
                     old_to_new_edge.end());

        util::inplacePermutation(edge_array.begin(), edge_array.end(), old_to_new_edge);
    }

    friend void serialization::read<EdgeDataT, Ownership>(storage::tar::FileReader &reader,
                                                          const std::string &name,
                                                          StaticGraph<EdgeDataT, Ownership> &graph);
    friend void
    serialization::write<EdgeDataT, Ownership>(storage::tar::FileWriter &writer,
                                               const std::string &name,
                                               const StaticGraph<EdgeDataT, Ownership> &graph);

  protected:
    template <typename IterT>
    void InitializeFromSortedEdgeRange(const std::uint32_t nodes, IterT begin, IterT end)
    {
        number_of_nodes = nodes;
        number_of_edges = static_cast<EdgeIterator>(std::distance(begin, end));
        node_array.reserve(number_of_nodes + 1);
        node_array.push_back(NodeArrayEntry{0u});
        auto iter = begin;
        for (auto node : util::irange(0u, nodes))
        {
            iter =
                std::find_if(iter, end, [node](const auto &edge) { return edge.source != node; });
            unsigned offset = std::distance(begin, iter);
            node_array.push_back(NodeArrayEntry{offset});
        }
        BOOST_ASSERT_MSG(
            iter == end,
            ("Still " + std::to_string(std::distance(iter, end)) + " edges left.").c_str());
        BOOST_ASSERT(node_array.size() == number_of_nodes + 1);

        edge_array.resize(number_of_edges);
        std::transform(begin, end, edge_array.begin(), [](const auto &from) {
            return static_graph_details::edgeToEntry<EdgeArrayEntry>(
                from, traits::HasDataMember<EdgeArrayEntry>{});
        });
    }

  protected:
    NodeIterator number_of_nodes;
    EdgeIterator number_of_edges;

    Vector<NodeArrayEntry> node_array;
    Vector<EdgeArrayEntry> edge_array;
};

} // namespace util
} // namespace osrm

#endif // STATIC_GRAPH_HPP
