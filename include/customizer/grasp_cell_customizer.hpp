#ifndef OSRM_GRASP_CELLS_CUSTOMIZER_HPP
#define OSRM_GRASP_CELLS_CUSTOMIZER_HPP

#include "customizer/cell_customizer.hpp"
#include "customizer/grasp_customization_graph.hpp"

#include "partition/grasp_storage.hpp"
#include "partition/multi_level_partition.hpp"

namespace osrm
{
namespace customizer
{

class GRASPCellCustomizer : public CellCustomizer
{
  public:
    GRASPCellCustomizer(const partition::MultiLevelPartition &partition) : CellCustomizer(partition)
    {
    }

    template <typename GraphT>
    void CustomizeCell(const GraphT &graph,
                   CellCustomizer::Heap &heap,
                   partition::CellStorage &cells,
                   GRASPCustomizationGraph &customization_graph,
                   LevelID level,
                   CellID id)
    {
        CellCustomizer::CustomizeCell(graph, heap, cells, level, id, [&](const NodeID source) {
            for (auto edge : customization_graph.GetAdjacentEdgeRange(source))
            {
                auto target = customization_graph.GetTarget(edge);
                auto &data = customization_graph.GetEdgeData(edge);
                if (heap.WasInserted(target))
                {
                    data.weight = std::min(data.weight, heap.GetKey(target));
                }
            }
        }, CellCustomizer::TerminateOnExhaution {});
    }

    template <typename GraphT>
    void
    Customize(const GraphT &graph, partition::CellStorage &cells, partition::GRASPStorage &grasp)
    {
        auto customization_graph = grasp.GetCustomizationGraph();

        CellCustomizer::Customize(graph,
                                  cells,
                                  [this, &customization_graph](const GraphT &graph,
                                                               Heap &heap,
                                                               partition::CellStorage &cells,
                                                               LevelID level,
                                                               CellID id) {
                                      CustomizeCell(graph, heap, cells, customization_graph, level, id);
                                  });

        grasp.SetDownwardEdges(customization_graph);
    }

};
}
}

#endif // OSRM_CELLS_CUSTOMIZER_HPP
