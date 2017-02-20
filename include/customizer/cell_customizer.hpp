#ifndef OSRM_CELLS_CUSTOMIZER_HPP
#define OSRM_CELLS_CUSTOMIZER_HPP

#include "partition/cell_storage.hpp"
#include "partition/multi_level_partition.hpp"
#include "util/binary_heap.hpp"

#include <boost/thread/tss.hpp>
#include <unordered_set>

namespace osrm
{
namespace customizer
{

class CellCustomizer
{

  public:
    CellCustomizer(const partition::MultiLevelPartition &partition) : partition(partition) {}

    template <typename GraphT>
    void Customize(const GraphT &graph,
                   partition::CellStorage &cells,
                   partition::LevelID level,
                   partition::CellID id)
    {
        auto cell = cells.GetCell(level, id);
        auto destinations = cell.GetDestinationNodes();

        // for each source do forward search
        for (auto source : cell.GetSourceNodes())
        {
            std::unordered_set<NodeID> destinations_set(destinations.begin(), destinations.end());
            Heap heap(graph.GetNumberOfNodes());
            heap.Insert(source, 0, {});

            // explore search space
            while (!heap.Empty() && !destinations_set.empty())
            {
                const NodeID node = heap.DeleteMin();
                const EdgeWeight weight = heap.GetKey(node);

                if (level == 1)
                    RelaxNode<true>(graph, cells, heap, level, id, node, weight);
                else
                    RelaxNode<false>(graph, cells, heap, level, id, node, weight);

                destinations_set.erase(node);
            }

            // fill a map of destination nodes to placeholder pointers
            auto destination_iter = destinations.begin();
            for (auto &weight : cell.GetOutWeight(source))
            {
                BOOST_ASSERT(destination_iter != destinations.end());
                const auto destination = *destination_iter++;
                weight =
                    heap.WasInserted(destination) ? heap.GetKey(destination) : INVALID_EDGE_WEIGHT;
            }
        }
    }

    template <typename GraphT> void Customize(const GraphT &graph, partition::CellStorage &cells)
    {
        for (std::size_t level = 1; level < partition.GetNumberOfLevels(); ++level)
        {
            tbb::parallel_for(tbb::blocked_range<std::size_t>(0, partition.GetNumberOfCells(level)),
                              [&](const tbb::blocked_range<std::size_t> &range) {
                                  for (auto id = range.begin(), end = range.end(); id != end; ++id)
                                  {
                                      Customize(graph, cells, level, id);
                                  }
                              });
        }
    }

  private:
    struct HeapData
    {
    };
    using Heap = util::
        BinaryHeap<NodeID, NodeID, EdgeWeight, HeapData, util::UnorderedMapStorage<NodeID, int>>;
    using HeapPtr = boost::thread_specific_ptr<Heap>;

    template <bool first_level, typename GraphT>
    void RelaxNode(const GraphT &graph,
                   const partition::CellStorage &cells,
                   Heap &heap,
                   partition::LevelID level,
                   partition::CellID id,
                   NodeID node,
                   EdgeWeight weight) const
    {
        if (!first_level)
        {
            // Relax sub-cell nodes
            auto subcell_id = partition.GetCell(level - 1, node);
            auto subcell = cells.GetCell(level - 1, subcell_id);
            auto subcell_destination = subcell.GetDestinationNodes().begin();
            for (auto subcell_weight : subcell.GetOutWeight(node))
            {
                if (subcell_weight != INVALID_EDGE_WEIGHT)
                {
                    const NodeID to = *subcell_destination;
                    const EdgeWeight to_weight = subcell_weight + weight;
                    if (!heap.WasInserted(to))
                    {
                        heap.Insert(to, to_weight, {});
                    }
                    else if (to_weight < heap.GetKey(to))
                    {
                        heap.DecreaseKey(to, to_weight);
                    }
                }

                ++subcell_destination;
            }
        }

        // Relax base graph edges if a sub-cell border edge
        for (auto edge : graph.GetAdjacentEdgeRange(node))
        {
            const NodeID to = graph.GetTarget(edge);
            const auto &data = graph.GetEdgeData(edge);
            if (data.forward && partition.GetCell(level, to) == id &&
                (first_level ||
                 partition.GetCell(level - 1, node) != partition.GetCell(level - 1, to)))
            {
                const EdgeWeight to_weight = data.weight + weight;
                if (!heap.WasInserted(to))
                {
                    heap.Insert(to, to_weight, {});
                }
                else if (to_weight < heap.GetKey(to))
                {
                    heap.DecreaseKey(to, to_weight);
                }
            }
        }
    }

    const partition::MultiLevelPartition &partition;
};
}
}

#endif // OSRM_CELLS_CUSTOMIZER_HPP
