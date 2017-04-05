#ifndef OSRM_CELLS_CUSTOMIZER_HPP
#define OSRM_CELLS_CUSTOMIZER_HPP

#include "partition/cell_storage.hpp"
#include "partition/multi_level_partition.hpp"
#include "util/binary_heap.hpp"

#include <tbb/enumerable_thread_specific.h>

#include <unordered_set>

namespace osrm
{
namespace customizer
{

class CellCustomizer
{
  private:
    struct HeapData
    {
        bool from_clique;
    };

  public:
    using Heap =
        util::BinaryHeap<NodeID, NodeID, EdgeWeight, HeapData, util::ArrayStorage<NodeID, int>>;
    using HeapPtr = tbb::enumerable_thread_specific<Heap>;

    CellCustomizer(const partition::MultiLevelPartition &partition) : partition(partition) {}

    template <typename GraphT>
    void Customize(
        const GraphT &graph, Heap &heap, partition::CellStorage &cells, LevelID level, CellID id)
    {
        auto cell = cells.GetCell(level, id);
        auto destinations = cell.GetDestinationNodes();

        // for each source do forward search
        for (auto source : cell.GetSourceNodes())
        {
            std::unordered_set<NodeID> destinations_set(destinations.begin(), destinations.end());
            heap.Clear();
            heap.Insert(source, 0, {false});

            // explore search space
            while (!heap.Empty() && !destinations_set.empty())
            {
                const NodeID node = heap.DeleteMin();
                const EdgeWeight weight = heap.GetKey(node);

                if (level == 1)
                    RelaxNode<true>(graph, cells, heap, level, node, weight);
                else
                    RelaxNode<false>(graph, cells, heap, level, node, weight);

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
        Heap heap_exemplar(graph.GetNumberOfNodes());
        HeapPtr heaps(heap_exemplar);

        for (std::size_t level = 1; level < partition.GetNumberOfLevels(); ++level)
        {
            tbb::parallel_for(tbb::blocked_range<std::size_t>(0, partition.GetNumberOfCells(level)),
                              [&](const tbb::blocked_range<std::size_t> &range) {
                                  auto &heap = heaps.local();
                                  for (auto id = range.begin(), end = range.end(); id != end; ++id)
                                  {
                                      Customize(graph, heap, cells, level, id);
                                  }
                              });
        }
    }

  private:
    template <bool first_level, typename GraphT>
    void RelaxNode(const GraphT &graph,
                   const partition::CellStorage &cells,
                   Heap &heap,
                   LevelID level,
                   NodeID node,
                   EdgeWeight weight) const
    {
        BOOST_ASSERT(heap.WasInserted(node));

        if (!first_level)
        {
            // if we reaches this node from a clique arc we don't need to scan
            // the clique arcs again because of the triangle inequality
            //
            // d(parent, node) + d(node, v) >= d(parent, v)
            //
            // And if there is a path (parent, node, v) there must also be a
            // clique arc (parent, v) with d(parent, v).
            if (!heap.GetData(node).from_clique)
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
                            heap.Insert(to, to_weight, {true});
                        }
                        else if (to_weight < heap.GetKey(to))
                        {
                            heap.DecreaseKey(to, to_weight);
                            heap.GetData(to).from_clique = true;
                        }
                    }

                    ++subcell_destination;
                }
            }
        }

        // Relax base graph edges if a sub-cell border edge
        for (auto edge : graph.GetInternalEdgeRange(level, node))
        {
            const NodeID to = graph.GetTarget(edge);
            const auto &data = graph.GetEdgeData(edge);
            if (data.forward &&
                (first_level ||
                 partition.GetCell(level - 1, node) != partition.GetCell(level - 1, to)))
            {
                const EdgeWeight to_weight = data.weight + weight;
                if (!heap.WasInserted(to))
                {
                    heap.Insert(to, to_weight, {false});
                }
                else if (to_weight < heap.GetKey(to))
                {
                    heap.DecreaseKey(to, to_weight);
                    heap.GetData(to).from_clique = false;
                }
            }
        }
    }

    const partition::MultiLevelPartition &partition;
};
}
}

#endif // OSRM_CELLS_CUSTOMIZER_HPP
