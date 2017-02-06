#include "partition/recursive_bisection_stats.hpp"

#include <boost/assert.hpp>

#include <bitset>
#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace osrm
{
namespace partition
{
void printBisectionStats(std::vector<RecursiveBisectionState::BisectionID> const &bisection_ids,
                         const BisectionGraph &graph)
{
    BOOST_ASSERT(graph.NumberOfNodes() == bisection_ids.size());
    std::size_t total_border_nodes = 0;
    std::unordered_map<RecursiveBisectionState::BisectionID, std::size_t> cell_sizes[32];
    std::unordered_map<RecursiveBisectionState::BisectionID, std::size_t> border_nodes[32];

    std::unordered_set<RecursiveBisectionState::BisectionID> all_ids[32];

    std::uint32_t flag = 0;
    for (std::uint32_t level = 0; level < 32; ++level)
    {
        std::uint32_t bit = 1u << (31 - level);
        flag |= bit;

        for (std::size_t i = 0; i < bisection_ids.size(); ++i)
            all_ids[level].insert(bisection_ids[i] & flag);

        if (level > 0 && all_ids[level - 1] == all_ids[level])
            break;

        std::cout << "Level" << std::endl;
        for (auto itr : all_ids[level])
            std::cout << "\t" << std::bitset<32>(itr) << std::endl;

        for (const auto &node : graph.Nodes())
        {
            const auto bisection_id_node = bisection_ids[node.original_id];

            // subrange is not partitioned anymore
            if (all_ids[level].count((bisection_id_node & flag) ^ bit) == 0)
                continue;

            auto is_border_node = false;
            for (const auto &edge : graph.Edges(node))
            {
                if (bisection_ids[edge.target] != bisection_id_node)
                    is_border_node = true;
            }

            if (is_border_node && level == 0)
                ++total_border_nodes;

            cell_sizes[level][bisection_id_node & flag]++;
            if (is_border_node)
            {
                for (const auto &edge : graph.Edges(node))
                {
                    if ((bisection_id_node & flag) != (bisection_ids[edge.target] & flag))
                    {
                        border_nodes[level][bisection_id_node & flag]++;
                        break;
                    }
                }
            }
        }
    }

    std::cout << "Partition statistics\n";
    std::cout << "Total border vertices: " << total_border_nodes << std::endl;
    unsigned level = 0;
    do
    {
        std::size_t min_size = -1, max_size = 0, total_size = 0;
        std::size_t min_border = -1, max_border = 1, total_border = 0;

        const auto summarize =
            [](const std::unordered_map<RecursiveBisectionState::BisectionID, std::size_t> &map,
               std::size_t &min,
               std::size_t &max,
               std::size_t &total) {
                for (const auto itr : map)
                {
                    min = std::min(min, itr.second);
                    max = std::max(max, itr.second);
                    total += itr.second;
                }
            };

        summarize(cell_sizes[level], min_size, max_size, total_size);
        summarize(border_nodes[level], min_border, max_border, total_border);

        std::cout << "Level: " << level << " Cells: " << cell_sizes[level].size();
        if (cell_sizes[level].size() > 1)
            std::cout << " Border: " << min_border << " " << max_border << " "
                      << total_border / (double)cell_sizes[level].size();

        std::cout << " Cell Sizes: " << min_size << " " << max_size << " "
                  << total_size / (double)cell_sizes[level].size()
                  << " Total Vertices In Level: " << total_size;

        std::cout << std::endl;

    } while (level < 31 && cell_sizes[++level].size() > 1);
}

} // namespace partition
} // namespace osrm
