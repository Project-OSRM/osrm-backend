#include "contractor/contractor.hpp"
#include "contractor/contract_excludable_graph.hpp"
#include "contractor/contracted_edge_container.hpp"
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

#include <boost/assert.hpp>
#include <tbb/global_control.h>

namespace osrm::contractor
{

int Contractor::Run()
{
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism,
                           config.requested_num_threads);

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
    extractor::files::readEdgeBasedNodeWeights(config.GetPath(".osrm.enw"), node_weights);
    util::Log() << "Done reading node weights.";

    util::Log() << "Loading edge-expanded graph representation";

    std::vector<extractor::EdgeBasedEdge> edge_based_edge_list;

    updater::Updater updater(config.updater_config);
    std::uint32_t connectivity_checksum = 0;
    EdgeID number_of_edge_based_nodes = updater.LoadAndUpdateEdgeExpandedGraph(
        edge_based_edge_list, node_weights, connectivity_checksum);

    // Convert node weights for oneway streets to INVALID_EDGE_WEIGHT
    for (auto &weight : node_weights)
    {
        weight = (from_alias<EdgeWeight::value_type>(weight) & 0x80000000) ? INVALID_EDGE_WEIGHT
                                                                           : weight;
    }

    // Contracting the edge-expanded graph

    TIMER_START(contraction);

    std::string metric_name;
    std::vector<std::vector<bool>> node_filters;
    {
        extractor::EdgeBasedNodeDataContainer node_data;
        extractor::files::readNodeData(config.GetPath(".osrm.ebg_nodes"), node_data);

        extractor::ProfileProperties properties;
        extractor::files::readProfileProperties(config.GetPath(".osrm.properties"), properties);
        metric_name = properties.GetWeightName();

        node_filters =
            util::excludeFlagsToNodeFilter(number_of_edge_based_nodes, node_data, properties);
    }

    QueryGraph query_graph;
    std::vector<std::vector<bool>> edge_filters;
    std::vector<std::vector<bool>> cores;
    std::tie(query_graph, edge_filters) = contractExcludableGraph(
        toContractorGraph(number_of_edge_based_nodes, std::move(edge_based_edge_list)),
        std::move(node_weights),
        node_filters);
    TIMER_STOP(contraction);
    util::Log() << "Contracted graph has " << query_graph.GetNumberOfEdges() << " edges.";
    util::Log() << "Contraction took " << TIMER_SEC(contraction) << " sec";

    std::unordered_map<std::string, ContractedMetric> metrics = {
        {metric_name, {std::move(query_graph), std::move(edge_filters)}}};

    files::writeGraph(config.GetPath(".osrm.hsgr"), metrics, connectivity_checksum);

    TIMER_STOP(preparing);

    util::Log() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";

    util::Log() << "finished preprocessing";

    return 0;
}

} // namespace osrm::contractor
