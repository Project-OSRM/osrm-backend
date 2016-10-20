#include "util/exception.hpp"
#include "util/fingerprint.hpp"
#include "util/graph_loader.hpp"
#include "util/node_based_graph.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace osrm
{
namespace tools
{
std::shared_ptr<util::NodeBasedDynamicGraph>
loadNodeBasedGraph(boost::filesystem::path osrm_file_path,
                   std::vector<QueryNode> &internal_to_external_node_map)
{
    std::vector<NodeBasedEdge> edge_list;
    boost::filesystem::ifstream input_stream(osrm_file_path, std::ios::in | std::ios::binary);

    std::vector<NodeID> barrier_list;
    std::vector<NodeID> traffic_light_list;
    NodeID number_of_node_based_nodes = util::loadNodesFromFile(
        input_stream, barrier_list, traffic_light_list, internal_to_external_node_map);

    barrier_list.clear();
    barrier_list.shrink_to_fit();
    traffic_light_list.clear();
    traffic_light_list.shrink_to_fit();

    util::loadEdgesFromFile(input_stream, edge_list);

    if (edge_list.empty())
    {
        util::SimpleLogger().Write(logWARNING) << "The input data is empty, exiting.";
        return std::shared_ptr<util::NodeBasedDynamicGraph>();
    }

    return util::NodeBasedDynamicGraphFromEdges(number_of_node_based_nodes, edge_list);
}

void writeNoramlizedGraph(boost::filesystem::path output_path,
                   extractor::NodeBasedGraph& graph,
                   extractor::CompressedEdgeContainer container,
                   std::vector<QueryNode> &internal_to_external_node_map)
{
    boost::filesystem::ofstream output_stream(output_path, std::ios::out);

    for (const auto current_node : util::irange(0u, graph.GetNumberOfNodes()))
    {
        for (const auto current_edge : graph.GetAdjacentEdgeRange(current_node))
        {
            EdgeData &edge_data = graph.GetEdgeData(current_edge);
            if (edge_data.forward && edge_data.backward)
            {
            }
            else if (edge_data.backward)
            {
            }
            else
            {
                BOOST_ASSERT(edge_data.backward);
            }
        }
    }
}

}
}

int main(int argc, char *argv[])
{
    using namespace osrm;

    std::vector<osrm::extractor::QueryNode> coordinate_list;
    util::LogPolicy::GetInstance().Unmute();

    // enable logging
    if (argc < 2)
    {
        util::SimpleLogger().Write(logWARNING) << "usage:\n" << argv[0] << " <osrm>";
        return EXIT_FAILURE;
    }

    std::vector<QueryNode> internal_to_external_id_map;
    auto graph = tools::loadNodeBasedGraph(boost::filesystem::path(argv[1]), internal_to_external_id_map);

    extractor::CompressedEdgeContainer compressed_edge_container;
    extractor::GraphCompressor graph_compressor;
    graph_compressor.Compress(std::unordered_set<NodeID> {},
                              std::unordered_set<NodeID> {},
                              extractor::RestrictionMap {},
                              *node_based_graph,
                              compressed_edge_container);


    tools::writeNormalizedGraph(boost::filesystem::path(argv[2]), graph, compressed_edge_container, internal_to_external_id_map);

    return EXIT_SUCCESS;
}
