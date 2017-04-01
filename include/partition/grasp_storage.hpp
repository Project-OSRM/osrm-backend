#ifndef OSRM_PARTITION_GRASP_STORAGE_HPP
#define OSRM_PARTITION_GRASP_STORAGE_HPP

#include "partition/cell_storage.hpp"

#include "customizer/grasp_customization_graph.hpp"

#include "storage/io_fwd.hpp"

#include "util/assert.hpp"
#include "util/log.hpp"
#include "util/static_graph.hpp"
#include "util/vector_view.hpp"

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

namespace osrm
{
namespace partition
{
namespace detail
{
template <storage::Ownership Ownership> class GRASPStorageImpl;
}
using GRASPStorage = detail::GRASPStorageImpl<storage::Ownership::Container>;
using GRASPStorageView = detail::GRASPStorageImpl<storage::Ownership::View>;

namespace serialization
{
template <storage::Ownership Ownership>
inline void read(storage::io::FileReader &reader, detail::GRASPStorageImpl<Ownership> &storage);
template <storage::Ownership Ownership>
inline void write(storage::io::FileWriter &writer,
                  const detail::GRASPStorageImpl<Ownership> &storage);
}

namespace detail
{

template <storage::Ownership Ownership> class GRASPStorageImpl
{
    using GRASPData = customizer::GRASPData;
    using DownwardsGraph = util::StaticGraph<GRASPData, Ownership>;
    using DownwardEdge = typename DownwardsGraph::InputEdge;
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;

  public:
    using GRASPNodeEntry = typename DownwardsGraph::NodeArrayEntry;
    using GRASPEdgeEntry = typename DownwardsGraph::EdgeArrayEntry;

    GRASPStorageImpl() = default;

    template <typename GraphT,
              typename = std::enable_if<Ownership == storage::Ownership::Container>,
              typename CellStorageT>
    GRASPStorageImpl(const partition::MultiLevelPartition &partition,
                     const GraphT &base_graph,
                     const CellStorageT &cell_storage)
    {
        // first compute the highest levlel at which a node is a border node
        std::vector<LevelID> highest_border_level(base_graph.GetNumberOfNodes(), 0);
        for (auto node : util::irange<NodeID>(0, base_graph.GetNumberOfNodes()))
        {
            for (auto edge : base_graph.GetAdjacentEdgeRange(node))
            {
                auto target = base_graph.GetTarget(edge);
                auto level = partition.GetHighestDifferentLevel(node, target);
                highest_border_level[node] = std::max(highest_border_level[node], level);
                highest_border_level[target] = std::max(highest_border_level[target], level);
            }
        }

        std::vector<DownwardEdge> edges;
        const auto insert_downward_edges = [&highest_border_level, &edges](
            LevelID level, const NodeID child_node, const CellStorage::ConstCell &parent_cell) {
            for (auto parent_node : parent_cell.GetSourceNodes())
            {
                BOOST_ASSERT(child_node != parent_node);
                if (highest_border_level[parent_node] == level)
                {
                    edges.emplace_back(child_node, parent_node, INVALID_EDGE_WEIGHT);
                }
            }
            for (auto parent_node : parent_cell.GetDestinationNodes())
            {
                BOOST_ASSERT(child_node != parent_node);
                if (highest_border_level[parent_node] == level)
                {
                    edges.emplace_back(child_node, parent_node, INVALID_EDGE_WEIGHT);
                }
            }
        };

        // Level 1: We need to insert an arc to all nodes in the base graph
        // TODO: This is super wasteful IMHO, the paper wants it that way
        // but we could also try to fall back to a search on the base graph
        for (auto node : util::irange<NodeID>(0, base_graph.GetNumberOfNodes()))
        {
            if (highest_border_level[node] > 0)
                continue;

            auto parent_cell = cell_storage.GetCell(1, partition.GetCell(1, node));
            insert_downward_edges(1, node, parent_cell);
        }

        for (auto level : util::irange<LevelID>(2, partition.GetNumberOfLevels()))
        {
            for (auto cell_id : util::irange<CellID>(0, partition.GetNumberOfCells(level)))
            {
                auto parent_cell = cell_storage.GetCell(level, cell_id);

                for (auto sub_cell_id = partition.BeginChildren(level, cell_id);
                     sub_cell_id < partition.EndChildren(level, cell_id);
                     ++sub_cell_id)
                {
                    auto child_cell = cell_storage.GetCell(level - 1, sub_cell_id);

                    for (auto child_node : child_cell.GetSourceNodes())
                    {
                        if (highest_border_level[child_node] == level - 1)
                        {
                            insert_downward_edges(level, child_node, parent_cell);
                        }
                    }
                    for (auto child_node : child_cell.GetDestinationNodes())
                    {
                        if (highest_border_level[child_node] == level - 1)
                        {
                            insert_downward_edges(level, child_node, parent_cell);
                        }
                    }
                }
            }
        }

        tbb::parallel_sort(edges.begin(), edges.end());
        auto new_end = std::unique(edges.begin(), edges.end());
        edges.resize(new_end - edges.begin());
        downwards_graph = DownwardsGraph{base_graph.GetNumberOfNodes(), edges};
    }

    GRASPStorageImpl(Vector<GRASPNodeEntry> nodes_, Vector<GRASPEdgeEntry> edges_)
        : downwards_graph(std::move(nodes_), std::move(edges_))
    {
    }

    customizer::GRASPCustomizationGraph GetCustomizationGraph() const
    {
        auto edges = downwards_graph.ToEdges();

        auto edge_range = tbb::blocked_range<std::size_t>(0, edges.size());
        tbb::parallel_for(edge_range, [&](const auto &range) {
            for (auto idx = range.begin(); idx < range.end(); ++idx)
            {
                std::swap(edges[idx].source, edges[idx].target);
            }
        });

        tbb::parallel_sort(edges.begin(), edges.end());

        return customizer::GRASPCustomizationGraph(downwards_graph.GetNumberOfNodes(), edges);
    }

    void SetDownwardEdges(const customizer::GRASPCustomizationGraph &customization_graph)
    {
        auto range = tbb::blocked_range<NodeID>(0, customization_graph.GetNumberOfNodes());
        tbb::parallel_for(range, [&, this](const auto &range) {
            for (auto node = range.begin(); node < range.end(); ++node)
            {
                for (auto edge : customization_graph.GetAdjacentEdgeRange(node))
                {
                    const auto target = customization_graph.GetTarget(edge);
                    const auto rev_edge = downwards_graph.FindEdge(target, node);
                    BOOST_ASSERT(rev_edge != SPECIAL_EDGEID);
                    auto &data = customization_graph.GetEdgeData(edge);
                    auto &rev_data = downwards_graph.GetEdgeData(rev_edge);
                    rev_data.weight = data.weight;
                }
            }
        });
    }

    // Returns all incoming downward edges from higher boundary nodes
    auto GetDownwardEdgeRange(const NodeID node) const
    {
        return downwards_graph.GetAdjacentEdgeRange(node);
    }

    // Return source of downward edge
    auto GetSource(const EdgeID edge) const { return downwards_graph.GetTarget(edge); }

    // Return data of downward edge
    auto GetEdgeData(const EdgeID edge) const { return downwards_graph.GetEdgeData(edge); }

  private:
    friend void serialization::read<Ownership>(storage::io::FileReader &reader,
                                               GRASPStorageImpl &storage);
    friend void serialization::write<Ownership>(storage::io::FileWriter &writer,
                                                const GRASPStorageImpl &storage);

    // maps a node to its incoming edges "downward edges"
    DownwardsGraph downwards_graph;
};
}
}
}

#endif // OSRM_CUSTOMIZE_CELL_STORAGE_HPP
