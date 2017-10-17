#include "contractor/contractor.hpp"
#include "contractor/contract_excludable_graph.hpp"
#include "contractor/contracted_edge_container.hpp"
#include "contractor/crc32_processor.hpp"
#include "contractor/files.hpp"
#include "contractor/graph_contractor.hpp"
#include "contractor/graph_contractor_adaptors.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/files.hpp"
#include "extractor/node_based_edge.hpp"

#include "storage/io.hpp"

#include "updater/updater.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/exclude_flag.hpp"
#include "util/filtered_graph.hpp"
#include "util/graph_loader.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/static_graph.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

namespace osrm
{
namespace contractor
{

int Contractor::Run()
{
    if (config.core_factor != 1.0)
    {
        util::Log(logWARNING)
            << "Using core factor is deprecated and will be ignored. Falling back to CH.";
        config.core_factor = 1.0;
    }

    if (config.use_cached_priority)
    {
        util::Log(logWARNING) << "Using cached priorities is deprecated and they will be ignored.";
    }

    TIMER_START(preparing);

    util::Log() << "Reading node weights.";
    std::vector<EdgeWeight> node_weights;
    {
        storage::io::FileReader reader(config.GetPath(".osrm.enw"),
                                       storage::io::FileReader::VerifyFingerprint);
        storage::serialization::read(reader, node_weights);
    }
    util::Log() << "Done reading node weights.";

    util::Log() << "Loading edge-expanded graph representation";

    std::vector<extractor::EdgeBasedEdge> edge_based_edge_list;

    updater::Updater updater(config.updater_config);
    EdgeID number_of_edge_based_nodes =
        updater.LoadAndUpdateEdgeExpandedGraph(edge_based_edge_list, node_weights);

    // Contracting the edge-expanded graph

    TIMER_START(contraction);

    std::vector<std::vector<bool>> node_filters;
    {
        extractor::EdgeBasedNodeDataContainer node_data;
        extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);

        extractor::ProfileProperties properties;
        extractor::files::readProfileProperties(config.GetPath(".osrm.properties"), properties);

        node_filters =
            util::excludeFlagsToNodeFilter(number_of_edge_based_nodes, node_data, properties);
    }

    RangebasedCRC32 crc32_calculator;
    const unsigned checksum = crc32_calculator(edge_based_edge_list);

    QueryGraph query_graph;
    std::vector<std::vector<bool>> edge_filters;
    std::vector<std::vector<bool>> cores;
    std::tie(query_graph, edge_filters) = contractExcludableGraph(
        toContractorGraph(number_of_edge_based_nodes, std::move(edge_based_edge_list)),
        std::move(node_weights),
        std::move(node_filters));
    TIMER_STOP(contraction);
    util::Log() << "Contracted graph has " << query_graph.GetNumberOfEdges() << " edges.";
    util::Log() << "Contraction took " << TIMER_SEC(contraction) << " sec";

    files::writeGraph(config.GetPath(".osrm.hsgr"), checksum, query_graph, edge_filters);

    TIMER_STOP(preparing);

    util::Log() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";

    util::Log() << "finished preprocessing";

    return 0;
}

} // namespace contractor
} // namespace osrm
