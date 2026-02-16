#ifndef OSRM_PARTITIONER_REMOVE_UNCONNECTED_HPP
#define OSRM_PARTITIONER_REMOVE_UNCONNECTED_HPP

#include "util/log.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <vector>

namespace osrm::partitioner
{
using Partition = std::vector<CellID>;

template <typename GraphT>
std::size_t removeUnconnectedBoundaryNodes(const GraphT &edge_based_graph,
                                           std::vector<Partition> &partitions)
{
    auto num_unconnected = 0;
    auto could_not_fix = 0;
    for (int level_index = partitions.size() - 1; level_index >= 0; level_index--)
    {
        struct Witness
        {
            NodeID id;
            std::size_t induced_border_edges;
        };
        std::vector<Witness> witnesses;
        for (NodeID node = 0; node < edge_based_graph.GetNumberOfNodes(); ++node)
        {
            witnesses.clear();

            bool is_source = false;
            bool is_target = false;

            const auto cell_id = partitions[level_index][node];
            for (auto edge : edge_based_graph.GetAdjacentEdgeRange(node))
            {
                const auto data = edge_based_graph.GetEdgeData(edge);
                const auto target = edge_based_graph.GetTarget(edge);
                const auto target_cell_id = partitions[level_index][target];
                if (target_cell_id == cell_id)
                {
                    is_source |= data.forward;
                    is_target |= data.backward;
                }
                else
                {
                    witnesses.push_back({target, 0});
                }
            }

            const auto unconnected = witnesses.size() > 0 && !is_source && !is_target;

            if (unconnected)
            {
                num_unconnected++;

                if (level_index < static_cast<int>(partitions.size() - 1))
                {
                    auto new_end =
                        std::remove_if(witnesses.begin(),
                                       witnesses.end(),
                                       [&](const auto &witness) {
                                           return partitions[level_index + 1][node] !=
                                                  partitions[level_index + 1][witness.id];
                                       });
                    witnesses.resize(new_end - witnesses.begin());
                }
                if (witnesses.size() == 0)
                {
                    could_not_fix++;
                    continue;
                }

                for (auto &witness : witnesses)
                {
                    for (auto edge : edge_based_graph.GetAdjacentEdgeRange(node))
                    {
                        auto target = edge_based_graph.GetTarget(edge);
                        for (auto sublevel_index = level_index; sublevel_index >= 0;
                             --sublevel_index)
                        {
                            if (partitions[sublevel_index][target] !=
                                partitions[sublevel_index][witness.id])
                                witness.induced_border_edges++;
                        }
                    }
                }

                auto best_witness =
                    std::min_element(witnesses.begin(),
                                     witnesses.end(),
                                     [](const auto &lhs, const auto &rhs) {
                                         return lhs.induced_border_edges < rhs.induced_border_edges;
                                     });
                BOOST_ASSERT(best_witness != witnesses.end());

                // assign `node` to same subcells as `best_witness`
                for (auto sublevel_index = level_index; sublevel_index >= 0; --sublevel_index)
                {
                    partitions[sublevel_index][node] = partitions[sublevel_index][best_witness->id];
                }
            }
        }
    }

    if (could_not_fix > 0)
        util::Log(logWARNING) << "Could not fix " << could_not_fix << " unconnected boundary nodes";

    return num_unconnected;
}
} // namespace osrm::partitioner

#endif
