#ifndef OSRM_CELLS_CUSTOMIZER_HPP
#define OSRM_CELLS_CUSTOMIZER_HPP

#include "partitioner/cell_storage.hpp"
#include "partitioner/multi_level_partition.hpp"
#include "util/query_heap.hpp"

#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>

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
        EdgeDuration duration;
        EdgeDistance distance;
    };

  public:
    using Heap =
        util::QueryHeap<NodeID, NodeID, EdgeWeight, HeapData, util::ArrayStorage<NodeID, int>>;
    using HeapPtr = tbb::enumerable_thread_specific<Heap>;

    CellCustomizer(const partitioner::MultiLevelPartition &partition) : partition(partition) {}

    template <typename GraphT>
    void Customize(const GraphT &graph,
                   Heap &heap,
                   const partitioner::CellStorage &cells,
                   const std::vector<bool> &allowed_nodes,
                   CellMetric &metric,
                   LevelID level,
                   CellID id) const
    {
        auto cell = cells.GetCell(metric, level, id);
        auto destinations = cell.GetDestinationNodes();

        // for each source do forward search
        for (auto source : cell.GetSourceNodes())
        {
            if (!allowed_nodes[source])
            {
                continue;
            }

            std::unordered_set<NodeID> destinations_set;
            for (const auto destination : destinations)
            {
                if (allowed_nodes[destination])
                {
                    destinations_set.insert(destination);
                }
            }
            heap.Clear();
            heap.Insert(source, 0, {false, 0, 0});

            // explore search space
            while (!heap.Empty() && !destinations_set.empty())
            {
                const NodeID node = heap.DeleteMin();
                const EdgeWeight weight = heap.GetKey(node);
                const EdgeDuration duration = heap.GetData(node).duration;
                const EdgeDistance distance = heap.GetData(node).distance;

                RelaxNode(graph,
                          cells,
                          allowed_nodes,
                          metric,
                          heap,
                          level,
                          node,
                          weight,
                          duration,
                          distance);

                destinations_set.erase(node);
            }

            // fill a map of destination nodes to placeholder pointers
            auto weights = cell.GetOutWeight(source);
            auto durations = cell.GetOutDuration(source);
            auto distances = cell.GetOutDistance(source);
            for (auto &destination : destinations)
            {
                BOOST_ASSERT(!weights.empty());
                BOOST_ASSERT(!durations.empty());
                BOOST_ASSERT(!distances.empty());

                const bool inserted = heap.WasInserted(destination);
                weights.front() = inserted ? heap.GetKey(destination) : INVALID_EDGE_WEIGHT;
                durations.front() =
                    inserted ? heap.GetData(destination).duration : MAXIMAL_EDGE_DURATION;
                distances.front() =
                    inserted ? heap.GetData(destination).distance : INVALID_EDGE_DISTANCE;

                weights.advance_begin(1);
                durations.advance_begin(1);
                distances.advance_begin(1);
            }
            BOOST_ASSERT(weights.empty());
            BOOST_ASSERT(durations.empty());
            BOOST_ASSERT(distances.empty());
        }
    }

    template <typename GraphT>
    void Customize(const GraphT &graph,
                   const partitioner::CellStorage &cells,
                   const std::vector<bool> &allowed_nodes,
                   CellMetric &metric) const
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
                                      Customize(
                                          graph, heap, cells, allowed_nodes, metric, level, id);
                                  }
                              });
        }
    }

  private:
    template <typename GraphT>
    void RelaxNode(const GraphT &graph,
                   const partitioner::CellStorage &cells,
                   const std::vector<bool> &allowed_nodes,
                   const CellMetric &metric,
                   Heap &heap,
                   LevelID level,
                   NodeID node,
                   EdgeWeight weight,
                   EdgeDuration duration,
                   EdgeDistance distance) const
    {
        auto first_level = level == 1;
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
                auto subcell = cells.GetCell(metric, level - 1, subcell_id);
                auto subcell_destination = subcell.GetDestinationNodes().begin();
                auto subcell_duration = subcell.GetOutDuration(node).begin();
                auto subcell_distance = subcell.GetOutDistance(node).begin();
                for (auto subcell_weight : subcell.GetOutWeight(node))
                {
                    if (subcell_weight != INVALID_EDGE_WEIGHT)
                    {
                        const NodeID to = *subcell_destination;
                        if (!allowed_nodes[to])
                        {
                            continue;
                        }

                        const EdgeWeight to_weight = weight + subcell_weight;
                        const EdgeDuration to_duration = duration + *subcell_duration;
                        const EdgeDistance to_distance = distance + *subcell_distance;
                        if (!heap.WasInserted(to))
                        {
                            heap.Insert(to, to_weight, {true, to_duration, to_distance});
                        }
                        else if (std::tie(to_weight, to_duration, to_distance) <
                                 std::tie(heap.GetKey(to),
                                          heap.GetData(to).duration,
                                          heap.GetData(to).distance))
                        {
                            heap.DecreaseKey(to, to_weight);
                            heap.GetData(to) = {true, to_duration, to_distance};
                        }
                    }

                    ++subcell_destination;
                    ++subcell_duration;
                    ++subcell_distance;
                }
            }
        }

        // Relax base graph edges if a sub-cell border edge
        for (auto edge : graph.GetInternalEdgeRange(level, node))
        {
            const NodeID to = graph.GetTarget(edge);
            if (!allowed_nodes[to])
            {
                continue;
            }

            const auto &data = graph.GetEdgeData(edge);
            if (data.forward &&
                (first_level ||
                 partition.GetCell(level - 1, node) != partition.GetCell(level - 1, to)))
            {
                const EdgeWeight to_weight = weight + data.weight;
                const EdgeDuration to_duration = duration + data.duration;
                const EdgeDistance to_distance = distance + data.distance;
                if (!heap.WasInserted(to))
                {
                    heap.Insert(
                        to, to_weight, {false, duration + data.duration, distance + data.distance});
                }
                else if (std::tie(to_weight, to_duration, to_distance) <
                         std::tie(
                             heap.GetKey(to), heap.GetData(to).duration, heap.GetData(to).distance))
                {
                    heap.DecreaseKey(to, to_weight);
                    heap.GetData(to) = {false, to_duration, to_distance};
                }
            }
        }
    }

    const partitioner::MultiLevelPartition &partition;
};
}
}

#endif // OSRM_CELLS_CUSTOMIZER_HPP
