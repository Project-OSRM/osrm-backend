#include "customizer/customizer.hpp"
#include "customizer/cell_customizer.hpp"
#include "partition/edge_based_graph_reader.hpp"
#include "partition/io.hpp"

#include "partition/cell_storage.hpp"
#include "partition/io.hpp"
#include "partition/multi_level_partition.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"

namespace osrm
{
namespace customize
{

int Customizer::Run(const CustomizationConfig &config)
{
    TIMER_START(loading_data);
    auto edge_based_graph = partition::LoadEdgeBasedGraph(config.edge_based_graph_path.string());
    util::Log() << "Loaded edge based graph for mapping partition ids: "
                << edge_based_graph->GetNumberOfEdges() << " edges, "
                << edge_based_graph->GetNumberOfNodes() << " nodes";

    osrm::partition::MultiLevelPartition mlp;
    partition::io::read(config.mld_partition_path, mlp);

    partition::CellStorage storage(mlp, *edge_based_graph);
    TIMER_STOP(loading_data);
    util::Log() << "Loading partition data took " << TIMER_SEC(loading_data) << " seconds";

    TIMER_START(cell_customize);
    CellCustomizer customizer(mlp);
    customizer.Customize(*edge_based_graph, storage);
    TIMER_STOP(cell_customize);
    util::Log() << "Cells customization took " << TIMER_SEC(cell_customize) << " seconds";

    TIMER_START(writing_mld_data);
    partition::io::write(config.mld_storage_path, storage);
    TIMER_STOP(writing_mld_data);
    util::Log() << "MLD customization writing took " << TIMER_SEC(writing_mld_data) << " seconds";

    return 0;
}

} // namespace customize
} // namespace osrm
