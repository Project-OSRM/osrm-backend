#include "contractor/contractor.hpp"
#include "contractor/files.hpp"
#include "contractor/graph_contractor.hpp"
#include "contractor/graph_contractor_adaptors.hpp"

#include "extractor/files.hpp"

#include "updater/updater.hpp"

#include "util/exclude_flag.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <cstdint>
#include <vector>

#include <tbb/global_control.h>

namespace osrm::contractor
{

int Contractor::Run()
{
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism,
                           config.requested_num_threads);

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

    // Contracting the edge-expanded graph

    TIMER_START(contraction);

    std::string metric_name;
    // filters on way classes like: 'toll', 'motorway', 'ferry', 'restricted', 'tunnel', ...
    // max. 7 classes can be defined
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
    std::tie(query_graph, edge_filters) = contractExcludableGraph(
        toContractorGraph(number_of_edge_based_nodes, edge_based_edge_list), node_filters);
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
