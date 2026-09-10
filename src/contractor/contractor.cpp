#include "contractor/contractor.hpp"
#include "contractor/files.hpp"
#include "contractor/graph_contractor.hpp"
#include "contractor/graph_contractor_adaptors.hpp"

#include "engine/isochrone/duration_graph_builder.hpp"

#include "extractor/files.hpp"
#include "extractor/isochrone_transition.hpp"

#include "updater/updater.hpp"

#include "util/exception.hpp"
#include "util/exclude_flag.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <cstdint>
#include <filesystem>
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

    std::vector<extractor::IsochroneTransition> transitions;
    bool has_preserved_transitions = false;
    if (config.generate_isochrone_data)
    {
        has_preserved_transitions =
            extractor::files::readIsochroneTransitions(config.GetPath(".osrm.ebg"), transitions);
        if (!has_preserved_transitions)
        {
            if (std::filesystem::exists(config.base_path.string() + ".osrm.partition"))
            {
                throw util::exception(
                    "Cannot generate exact isochrone data from a partitioned graph without "
                    "preserved transitions. Re-run osrm-partition with "
                    "--generate-isochrone-data." +
                    std::string(SOURCE_REF));
            }
        }
    }

    updater::Updater updater(config.updater_config);
    updater::TurnPenaltyMetrics turn_penalties;
    std::vector<EdgeDuration> node_duration_lower_bounds;
    std::vector<EdgeWeight> node_weight_lower_bounds;
    updater::EdgeExpandedGraphUpdateOptions update_options;
    if (config.generate_isochrone_data)
    {
        if (has_preserved_transitions)
            update_options.isochrone_transitions = &transitions;
        else
            update_options.generated_isochrone_transitions = &transitions;
        update_options.turn_penalties = &turn_penalties;
        update_options.node_duration_lower_bounds = &node_duration_lower_bounds;
        update_options.node_weight_lower_bounds = &node_weight_lower_bounds;
    }
    std::uint32_t connectivity_checksum = 0;
    engine::isochrone::DurationGraph isochrone_graph;
    EdgeID number_of_edge_based_nodes;
    if (config.generate_isochrone_data)
    {
        std::vector<EdgeDuration> node_durations;
        number_of_edge_based_nodes = updater.LoadAndUpdateEdgeExpandedGraph(edge_based_edge_list,
                                                                            node_weights,
                                                                            node_durations,
                                                                            connectivity_checksum,
                                                                            update_options);
        isochrone_graph = engine::isochrone::buildDurationGraph(number_of_edge_based_nodes,
                                                                transitions,
                                                                node_weights,
                                                                node_durations,
                                                                turn_penalties.weight_penalties,
                                                                turn_penalties.duration_penalties,
                                                                node_duration_lower_bounds,
                                                                node_weight_lower_bounds);
        std::vector<extractor::IsochroneTransition>{}.swap(transitions);
    }
    else
    {
        number_of_edge_based_nodes = updater.LoadAndUpdateEdgeExpandedGraph(
            edge_based_edge_list, node_weights, connectivity_checksum);
    }

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
        {metric_name,
         {std::move(query_graph), std::move(edge_filters), std::move(isochrone_graph)}}};

    files::writeGraph(config.GetOutputPath(".osrm.hsgr"), metrics, connectivity_checksum);

    TIMER_STOP(preparing);

    util::Log() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";

    util::Log() << "finished preprocessing";

    return 0;
}

} // namespace osrm::contractor
