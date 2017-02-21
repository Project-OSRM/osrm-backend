#include "partition/annotated_partition.hpp"

#include <algorithm>
#include <climits> // for CHAR_BIT
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <string>
#include <unordered_map>

#include "util/timing_util.hpp"

namespace osrm
{
namespace partition
{

namespace
{

// the shift value needed to access the most significant bit of the bisection ID
const constexpr auto SHIFT_TO_MSB_BISECTION_ID = sizeof(BisectionID) * CHAR_BIT - 1;
// an invalid ID for a cell
const constexpr std::uint32_t INVALID_CELLID = std::numeric_limits<std::uint32_t>::max();

auto masked(const BisectionID id, const std::int32_t level)
{
    // special treatment for negative level
    if (level == -1)
        return 0u;

    // 0.01.1 with 1 starting at the level+1_th most significant bit (level = 0 -> 01..1)
    const auto cut_below_level = (1 << (SHIFT_TO_MSB_BISECTION_ID - level)) - 1;
    const auto mask = std::numeric_limits<BisectionID>::max() ^ cut_below_level;
    return id & mask;
}

// create a comparator for a given level
auto makeCompare(const std::uint32_t level)
{
    return [level](const AnnotatedPartition::SizedID lhs, const AnnotatedPartition::SizedID rhs) {
        return masked(lhs.id, level) < masked(rhs.id, level);
    };
}

// build a tree of cells from the IDs present:
auto leftChild(const BisectionID id_prefix, const std::int32_t /*level*/) { return id_prefix; }

// given the prefix 10.... on level 1 (second level), the the right child would be
// 101.... on level 2
auto rightChild(const BisectionID id_prefix, const std::int32_t level)
{
    return id_prefix | (1 << (SHIFT_TO_MSB_BISECTION_ID - (level + 1)));
}

// get the range of all children
auto getChildrenRange(const std::vector<AnnotatedPartition::SizedID> &implicit_tree,
                      const BisectionID id_prefix,
                      const std::int32_t level)
{
    AnnotatedPartition::SizedID id = {id_prefix, 0};

    // find all elements of the same prefix as id_prefi
    auto range =
        std::equal_range(implicit_tree.begin(), implicit_tree.end(), id, makeCompare(level));

    // don't ever return our sentinel element as included
    if (range.second == implicit_tree.end())
        --range.second;

    return range;
}

auto getCellSize(const std::vector<AnnotatedPartition::SizedID> &implicit_tree,
                 const BisectionID id_prefix,
                 const std::uint32_t level)
{
    auto range = getChildrenRange(implicit_tree, id_prefix, level);
    return range.second->count - range.first->count;
}

bool hasChildren(const std::vector<AnnotatedPartition::SizedID> &implicit_tree,
                 const BisectionID id_prefix,
                 const std::uint32_t level)
{
    auto range = getChildrenRange(implicit_tree, id_prefix, level);
    return std::distance(range.first, range.second) > 1;
}

} // namespace

AnnotatedPartition::AnnotatedPartition(const BisectionGraph &graph,
                                       const std::vector<BisectionID> &bisection_ids)
{
    // create a sorted vector of bisection ids that exist in the network
    std::vector<SizedID> implicit_tree = [&]() {
        std::map<BisectionID, SizedID> existing_ids;

        // insert an ID into the sized_id set or increase the count if the element should be already
        // present in the set of known ids
        const auto insert_or_augment = [&existing_ids](const BisectionID id) {
            SizedID sized_id = {id, 1};
            auto maybe_existing_id = existing_ids.find(id);
            if (maybe_existing_id == existing_ids.end())
                existing_ids[id] = sized_id;
            else
                maybe_existing_id->second.count++;
        };
        std::for_each(bisection_ids.begin(), bisection_ids.end(), insert_or_augment);

        std::vector<SizedID> result;
        result.resize(existing_ids.size() + 1);
        std::transform(existing_ids.begin(),
                       existing_ids.end(),
                       result.begin(),
                       [](const auto &pair) { return pair.second; });

        // sentinel
        result.back() = {std::numeric_limits<BisectionID>::max(), 0};

        return result;
    }();

    // calculate a prefix sum over all sorted IDs, this allows to get the size of any partition in
    // the array/level based on the prefix and lower bound on prefixes.
    // e.g 00,01,10,11 allow to search for (0) (1) to find (00) and (10) as lower bounds. The
    // difference in count is the size of all cells in the left part of the partition.
    std::transform(implicit_tree.begin(),
                   implicit_tree.end(),
                   implicit_tree.begin(),
                   [sum = std::size_t{0}](SizedID id) mutable {
                       const auto new_sum = sum + id.count;
                       id.count = sum;
                       sum = new_sum;
                       return id;
                   });

    PrintBisection(implicit_tree, graph, bisection_ids);
    SearchLevels(implicit_tree, graph, bisection_ids);
}

void AnnotatedPartition::PrintBisection(const std::vector<SizedID> &implicit_tree,
                                        const BisectionGraph &graph,
                                        const std::vector<BisectionID> &bisection_ids) const
{
    // print some statistics on the bisection tree
    std::queue<BisectionID> id_queue;
    id_queue.push(0);

    const auto add_child = [&id_queue, &implicit_tree](const BisectionID prefix,
                                                       const std::uint32_t level) {
        const auto child_range = getChildrenRange(implicit_tree, prefix, level);
        if (std::distance(child_range.first, child_range.second) > 1)
            id_queue.push(prefix);
    };

    std::vector<std::pair<BisectionID, std::int32_t>> current_level;
    for (std::int32_t level = -1; !id_queue.empty(); ++level)
    {
        auto level_size = id_queue.size();
        current_level.clear();
        while (level_size--)
        {
            const auto prefix = id_queue.front();
            id_queue.pop();
            if (level == -1 || hasChildren(implicit_tree, prefix, level))
            {
                current_level.push_back(
                    std::pair<BisectionID, std::uint32_t>(leftChild(prefix, level), level + 1));
                current_level.push_back(
                    std::pair<BisectionID, std::uint32_t>(rightChild(prefix, level), level + 1));
            }
            add_child(leftChild(prefix, level), level);
            add_child(rightChild(prefix, level), level);
        }
        if (!current_level.empty())
        {
            const auto cell_ids = ComputeCellIDs(current_level, graph, bisection_ids);
            const auto stats = AnalyseLevel(graph, cell_ids);
            stats.logMachinereadable(std::cout, "bisection", level, level == -1);
        }
    }
}

void AnnotatedPartition::SearchLevels(const std::vector<SizedID> &implicit_tree,
                                      const BisectionGraph &graph,
                                      const std::vector<BisectionID> &bisection_ids) const
{
    std::vector<std::pair<BisectionID, std::int32_t>> current_level;

    // start searching with level 0 at prefix 0
    current_level.push_back({static_cast<BisectionID>(0), -1});
    std::int32_t level = -1;

    const auto print_level = [&]() {
        if (current_level.empty())
            return;

        const auto cell_ids = ComputeCellIDs(current_level, graph, bisection_ids);
        const auto stats = AnalyseLevel(graph, cell_ids);
        stats.logMachinereadable(std::cout, "dfs-balanced", level, level == -1);
        ++level;
    };

    std::size_t max_size = 0.5 * graph.NumberOfNodes();
    std::queue<std::pair<BisectionID, std::int32_t>> id_queue;
    while (!current_level.empty())
    {
        std::size_t total_size = 0;
        std::size_t count = 0;
        for (auto element : current_level)
        {
            // don't relax final cells
            if (element.second == -1 || hasChildren(implicit_tree, element.first, element.second))
            {
                total_size += getCellSize(
                    implicit_tree, leftChild(element.first, element.second), element.second + 1);
                id_queue.push(std::pair<BisectionID, std::uint32_t>(
                    leftChild(element.first, element.second), element.second + 1));

                total_size += getCellSize(
                    implicit_tree, rightChild(element.first, element.second), element.second + 1);
                id_queue.push(std::pair<BisectionID, std::uint32_t>(
                    rightChild(element.first, element.second), element.second + 1));
                count += 2;
            }
        }
        auto avg_size = (total_size / static_cast<double>(count));

        current_level.clear();

        const auto relax = [&id_queue, &implicit_tree, avg_size, &current_level](
            const std::pair<BisectionID, std::uint32_t> &element) {
            const auto size = getCellSize(implicit_tree, element.first, element.second);
            if (!hasChildren(implicit_tree, element.first, element.second))
            {
                current_level.push_back(element);
            }
            else
            {
                const auto left = leftChild(element.first, element.second);
                const auto right = rightChild(element.first, element.second);

                const auto get_penalty = [avg_size](const auto size) {
                    return std::abs(size - avg_size);
                };

                if (get_penalty(size) <
                    0.5 * (get_penalty(getCellSize(implicit_tree, left, element.second + 1)) +
                           get_penalty(getCellSize(implicit_tree, right, element.second + 1))))
                {
                    current_level.push_back(element);
                }
                else
                {
                    id_queue.push(std::pair<BisectionID, std::uint32_t>(left, element.second + 1));
                    id_queue.push(std::pair<BisectionID, std::uint32_t>(right, element.second + 1));
                }
            }
        };

        while (!id_queue.empty())
        {
            relax(id_queue.front());
            id_queue.pop();
        }
        print_level();
        max_size *= 0.5;
    }
}

AnnotatedPartition::LevelMetrics
AnnotatedPartition::AnalyseLevel(const BisectionGraph &graph,
                                 const std::vector<std::uint32_t> &cell_ids) const
{
    std::unordered_map<std::uint32_t, std::size_t> cell_sizes;
    std::unordered_map<std::uint32_t, std::size_t> border_nodes;
    std::unordered_map<std::uint32_t, std::size_t> border_arcs;

    // compute basic metrics of the level
    std::size_t border_nodes_total = 0;
    std::size_t border_arcs_total = 0;
    std::size_t contained_nodes = 0;

    // only border nodes on the lowest level can be border nodes in general
    for (const auto &node : graph.Nodes())
    {
        const auto cell_id = cell_ids[node.original_id];
        if (cell_id == INVALID_CELLID)
            continue;

        ++contained_nodes;

        const auto edge_range = graph.Edges(node);
        const auto border_arcs_at_node = std::count_if(
            edge_range.begin(), edge_range.end(), [&cell_id, &cell_ids, &graph](const auto &edge) {
                const auto target_cell_id = cell_ids[graph.Node(edge.target).original_id];
                return target_cell_id != cell_id;
            });

        cell_sizes[cell_id]++;
        border_arcs[cell_id] += border_arcs_at_node;
        border_arcs_total += border_arcs_at_node;
        if (border_arcs_at_node)
        {
            border_nodes[cell_id]++;
            ++border_nodes_total;
        }
    }

    const auto by_size = [](const std::pair<std::uint32_t, std::size_t> &lhs,
                            const std::pair<std::uint32_t, std::size_t> &rhs) {
        return lhs.second < rhs.second;
    };
    const auto max_nodes =
        border_nodes.empty()
            ? 0
            : std::max_element(border_nodes.begin(), border_nodes.end(), by_size)->second;
    const auto max_arcs =
        border_arcs.empty()
            ? 0
            : std::max_element(border_arcs.begin(), border_arcs.end(), by_size)->second;

    const auto squarded_size = [](const std::size_t accumulated,
                                  const std::pair<std::uint32_t, std::size_t> &element) {
        return accumulated + element.second * element.second;
    };

    const auto memory =
        4 * std::accumulate(border_arcs.begin(), border_nodes.end(), std::size_t(0), squarded_size);

    std::vector<std::size_t> cell_sizes_vec;
    cell_sizes_vec.resize(cell_sizes.size());
    std::transform(cell_sizes.begin(),
                   cell_sizes.end(),
                   cell_sizes_vec.begin(),
                   [](const auto &pair) { return pair.second; });
    return {border_nodes_total,
            border_arcs_total,
            contained_nodes,
            border_nodes.size(),
            max_nodes,
            max_arcs,
            memory,
            std::move(cell_sizes_vec)};
}

std::vector<std::uint32_t>
AnnotatedPartition::ComputeCellIDs(std::vector<std::pair<BisectionID, std::int32_t>> &prefixes,
                                   const BisectionGraph &graph,
                                   const std::vector<BisectionID> &bisection_ids) const
{
    std::vector<std::uint32_t> cell_ids(graph.NumberOfNodes(), INVALID_CELLID);

    std::sort(prefixes.begin(), prefixes.end(), [](const auto lhs, const auto rhs) {
        return lhs.first < rhs.first;
    });

    for (const auto &node : graph.Nodes())
    {
        // find the cell_id of node in the current levels
        const auto id = bisection_ids[node.original_id];

        const auto is_prefixed_by = [id](const auto &prefix) {
            return masked(id, prefix.second) == prefix.first;
        };

        const auto prefix = std::lower_bound(
            prefixes.begin(), prefixes.end(), id, [&](const auto prefix, const BisectionID id) {
                return prefix.first < masked(id, prefix.second);
            });

        if (prefix == prefixes.end())
            continue;

        if (is_prefixed_by(*prefix))
            cell_ids[node.original_id] = std::distance(prefixes.begin(), prefix);
    }
    return cell_ids;
}

} // namespace partition
} // namespace osrm
