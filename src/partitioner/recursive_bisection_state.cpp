#include "partitioner/recursive_bisection_state.hpp"
#include "extractor/tarjan_scc.hpp"
#include "partitioner/tarjan_graph_wrapper.hpp"

#include <algorithm>
#include <climits> // for CHAR_BIT
#include <numeric>
#include <set>
#include <unordered_map>

namespace osrm
{
namespace partitioner
{

RecursiveBisectionState::RecursiveBisectionState(BisectionGraph &bisection_graph_)
    : scc_levels(0), bisection_graph(bisection_graph_)
{
    bisection_ids.resize(bisection_graph.NumberOfNodes(), BisectionID{0});
}

RecursiveBisectionState::~RecursiveBisectionState() {}

BisectionID RecursiveBisectionState::GetBisectionID(const NodeID node) const
{
    return bisection_ids[node];
}

RecursiveBisectionState::NodeIterator
RecursiveBisectionState::ApplyBisection(const NodeIterator const_begin,
                                        const NodeIterator const_end,
                                        const std::size_t depth,
                                        const std::vector<bool> &partition)
{
    BOOST_ASSERT(depth >= scc_levels);
    // ensure that the iterators belong to the graph
    BOOST_ASSERT(bisection_graph.GetID(*const_begin) < bisection_graph.NumberOfNodes() &&
                 bisection_graph.GetID(*const_begin) + std::distance(const_begin, const_end) <=
                     bisection_graph.NumberOfNodes());
    // augment the partition ids
    const auto flag = BisectionID{1} << (sizeof(BisectionID) * CHAR_BIT - depth - 1);
    for (auto itr = const_begin; itr != const_end; ++itr)
    {
        const auto nid = std::distance(const_begin, itr);
        if (partition[nid])
            bisection_ids[itr->original_id] |= flag;
    }

    // Keep items with `0` as partition id to the left, move other to the right
    auto by_flag_bit = [this, flag](const auto &node) {
        return BisectionID{0} == (bisection_ids[node.original_id] & flag);
    };

    auto begin = bisection_graph.Begin() + std::distance(bisection_graph.CBegin(), const_begin);
    const auto end = begin + std::distance(const_begin, const_end);

    // remap the edges
    std::vector<NodeID> mapping(std::distance(const_begin, const_end), SPECIAL_NODEID);
    // calculate a mapping of all node ids
    std::size_t lesser_id = 0, upper_id = 0;
    std::transform(const_begin,
                   const_end,
                   mapping.begin(),
                   [by_flag_bit, &lesser_id, &upper_id](const auto &node) {
                       return by_flag_bit(node) ? lesser_id++ : upper_id++;
                   });

    // erase all edges that point into different partitions
    std::for_each(begin, end, [&](auto &node) {
        const auto node_flag = by_flag_bit(node);
        bisection_graph.RemoveEdges(node, [&](const BisectionGraph::EdgeT &edge) {
            const auto target_flag = by_flag_bit(*(const_begin + edge.target));
            return (node_flag != target_flag);
        });
    });

    auto center = std::stable_partition(begin, end, by_flag_bit);

    // remap all remaining edges
    std::for_each(const_begin, const_end, [&](const auto &node) {
        for (auto &edge : bisection_graph.Edges(node))
            edge.target = mapping[edge.target];
    });

    return const_begin + std::distance(begin, center);
}

std::vector<BisectionGraphView>
RecursiveBisectionState::PrePartitionWithSCC(const std::size_t small_component_size)
{
    // since our graphs are unidirectional, we don't realy need the scc. But tarjan is so nice and
    // assigns IDs and counts sizes
    TarjanGraphWrapper wrapped_graph(bisection_graph);
    extractor::TarjanSCC<TarjanGraphWrapper> scc_algo(wrapped_graph);
    scc_algo.Run();

    // Map Edges to Sccs
    const auto in_small = [&scc_algo, small_component_size](const NodeID node_id) {
        return scc_algo.GetComponentSize(scc_algo.GetComponentID(node_id)) <= small_component_size;
    };

    const constexpr std::size_t small_component_id = -1;
    std::unordered_map<std::size_t, std::size_t> component_map;
    const auto transform_id = [&](const NodeID node_id) -> std::size_t {
        if (in_small(node_id))
            return small_component_id;
        else
            return scc_algo.GetComponentID(node_id);
    };

    std::vector<NodeID> mapping(bisection_graph.NumberOfNodes(), SPECIAL_NODEID);
    for (const auto &node : bisection_graph.Nodes())
        mapping[node.original_id] = component_map[transform_id(node.original_id)]++;

    // needs to remove edges, if we should ever switch to directed graphs here
    std::stable_sort(
        bisection_graph.Begin(), bisection_graph.End(), [&](const auto &lhs, const auto &rhs) {
            return transform_id(lhs.original_id) < transform_id(rhs.original_id);
        });

    // remap all remaining edges
    std::for_each(bisection_graph.Begin(), bisection_graph.End(), [&](const auto &node) {
        for (auto &edge : bisection_graph.Edges(node))
            edge.target = mapping[edge.target];
    });

    std::vector<BisectionGraphView> views;
    auto last = bisection_graph.CBegin();
    auto last_id = transform_id(bisection_graph.Begin()->original_id);
    std::set<std::size_t> ordered_component_ids;
    for (auto itr = bisection_graph.CBegin(); itr != bisection_graph.CEnd(); ++itr)
    {
        auto itr_id = transform_id(itr->original_id);
        ordered_component_ids.insert(itr_id);
        if (last_id != itr_id)
        {
            views.push_back(BisectionGraphView(bisection_graph, last, itr));
            last_id = itr_id;
            last = itr;
        }
    }
    views.push_back(BisectionGraphView(bisection_graph, last, bisection_graph.CEnd()));

    bool has_small_component = [&]() {
        for (std::size_t i = 0; i < scc_algo.GetNumberOfComponents(); ++i)
            if (scc_algo.GetComponentSize(i) <= small_component_size)
                return true;
        return false;
    }();

    if (!has_small_component)
        views.push_back(
            BisectionGraphView(bisection_graph, bisection_graph.CEnd(), bisection_graph.CEnd()));

    // apply scc as bisections, we need scc_level bits for this with scc_levels =
    // ceil(log_2(components))
    scc_levels = ceil(log(views.size()) / log(2.0));

    const auto conscutive_component_id = [&](const NodeID nid) {
        const auto component_id = transform_id(nid);
        const auto itr = ordered_component_ids.find(component_id);
        BOOST_ASSERT(itr != ordered_component_ids.end());
        BOOST_ASSERT(static_cast<std::size_t>(std::distance(ordered_component_ids.begin(), itr)) <
                     ordered_component_ids.size());
        return std::distance(ordered_component_ids.begin(), itr);
    };

    const auto shift = sizeof(BisectionID) * CHAR_BIT - scc_levels;

    // store the component ids as first part of the bisection id
    for (const auto &node : bisection_graph.Nodes())
        bisection_ids[node.original_id] = conscutive_component_id(node.original_id) << shift;

    return views;
}

const std::vector<BisectionID> &RecursiveBisectionState::BisectionIDs() const
{
    return bisection_ids;
}

std::uint32_t RecursiveBisectionState::SCCDepth() const { return scc_levels; }

} // namespace partitioner
} // namespace osrm
