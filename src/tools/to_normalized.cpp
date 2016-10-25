#include "extractor/compressed_edge_container.hpp"
#include "extractor/graph_compressor.hpp"
#include "extractor/restriction_map.hpp"

#include "util/exception.hpp"
#include "util/fingerprint.hpp"
#include "util/graph_loader.hpp"
#include "util/node_based_graph.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"

#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>

namespace osrm
{
namespace tools
{
std::shared_ptr<util::NodeBasedDynamicGraph>
loadNodeBasedGraph(boost::filesystem::path osrm_file_path,
                   std::vector<NodeID> &barrier_list,
                   std::vector<extractor::QueryNode> &internal_to_external_node_map)
{
    std::vector<extractor::NodeBasedEdge> edge_list;
    boost::filesystem::ifstream input_stream(osrm_file_path, std::ios::in | std::ios::binary);

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

std::string classificationToString(extractor::guidance::RoadClassification classification)
{
    using namespace extractor::guidance;
    std::string base;
    switch (classification.GetClass())
    {
    case RoadPriorityClass::MOTORWAY:
        base = "motorway";
        break;
    case RoadPriorityClass::TRUNK:
        base = "trunk";
        break;
    case RoadPriorityClass::PRIMARY:
        base = "primary";
        break;
    case RoadPriorityClass::SECONDARY:
        base = "secondary";
        break;
    case RoadPriorityClass::TERTIARY:
        base = "tertiary";
        break;
    case RoadPriorityClass::MAIN_RESIDENTIAL:
        base = "residential";
        break;
    case RoadPriorityClass::SIDE_RESIDENTIAL:
        base = "living_street";
        break;
    case RoadPriorityClass::BIKE_PATH:
        base = "cyclepath";
        break;
    case RoadPriorityClass::FOOT_PATH:
        base = "footpath";
        break;
    // FIXME we lose information here
    case RoadPriorityClass::LINK_ROAD:
    case RoadPriorityClass::CONNECTIVITY:
        base = "unclassified";
        break;
    }
    if (classification.IsLinkClass())
    {
        base += "_link";
    }

    return base;
}

void writeNormalizedGraph(boost::filesystem::path output_path,
                          const util::NodeBasedDynamicGraph &graph,
                          const extractor::CompressedEdgeContainer &compressed_edge_container,
                          const std::vector<extractor::QueryNode> &internal_to_external_node_map)
{
    boost::filesystem::ofstream out(output_path, std::ios::out);
    out << std::setprecision(12);

    std::size_t way_id = 1;

    for (const auto source_node : util::irange(0u, graph.GetNumberOfNodes()))
    {
        for (const auto fwd_edge : graph.GetAdjacentEdgeRange(source_node))
        {
            auto target_node = graph.GetTarget(fwd_edge);

            // we have every edge at twice (even for oneways)
            // so we pick based on id precedence
            if (source_node > target_node)
            {
                continue;
            }

            auto rev_edge = graph.FindEdge(target_node, source_node);
            BOOST_ASSERT(rev_edge != SPECIAL_EDGEID);
            auto &fwd_edge_data = graph.GetEdgeData(fwd_edge);
            auto &rev_edge_data = graph.GetEdgeData(rev_edge);

            if (!fwd_edge_data.startpoint || !rev_edge_data.startpoint)
                continue;

            // if this is a oneway in fordward direction if the reverse edge is just an imcoming
            // edge
            auto oneway = rev_edge_data.reversed;
            // if this is a oneway in reverse direction if the forward edge is just an imcoming edge
            auto inv_oneway = fwd_edge_data.reversed;
            BOOST_ASSERT(!oneway || !inv_oneway);

            const auto &fwd_bucket = compressed_edge_container.GetBucketReference(fwd_edge);
            const auto &rev_bucket = compressed_edge_container.GetBucketReference(rev_edge);

            std::stringstream coordinates;
            std::stringstream refs;
            coordinates << std::setprecision(12);
            refs << std::setprecision(12);

            const auto encode_coordinates_refs =
                [&coordinates, &refs, &internal_to_external_node_map](
                    const extractor::CompressedEdgeContainer::OnewayEdgeBucket &fwd_bucket,
                    const extractor::CompressedEdgeContainer::OnewayEdgeBucket &rev_bucket) {
                    const auto &first_query_node =
                        internal_to_external_node_map[rev_bucket.back().node_id];
                    coordinates << "[" << util::toFloating(first_query_node.lon) << ","
                                << util::toFloating(first_query_node.lat) << "],";
                    refs << first_query_node.node_id << ",";

                    for (auto idx = 0UL; idx < fwd_bucket.size() - 1; ++idx)
                    {
                        const auto &query_node =
                            internal_to_external_node_map[fwd_bucket[idx].node_id];
                        coordinates << "[" << util::toFloating(query_node.lon) << ","
                                    << util::toFloating(query_node.lat) << "],";
                        refs << query_node.node_id << ",";
                    }

                    const auto &last_query_node =
                        internal_to_external_node_map[fwd_bucket.back().node_id];
                    coordinates << "[" << util::toFloating(last_query_node.lon) << ","
                                << util::toFloating(last_query_node.lat) << "]";
                    refs << last_query_node.node_id;
                };

            if (inv_oneway)
            {
                encode_coordinates_refs(rev_bucket, fwd_bucket);
            }
            else
            {
                encode_coordinates_refs(fwd_bucket, rev_bucket);
            }

            out << "{\"type\":\"Feature\",\"geometry\":{\"type\":\"LineString\",\"coordinates\":["
                << coordinates.str() << "]},\"properties\":{\"id\":\"" << (way_id++)
                << "\",\"refs\":[" << refs.str() << "], \"oneway\":" << (oneway | inv_oneway)
                << ",\"highway\":\"" << classificationToString(fwd_edge_data.road_classification)
                << "\"}}\n";

            constexpr bool DEBUG_INTERSECTIONS = false;
            if (DEBUG_INTERSECTIONS)
            {
                const auto &first = fwd_bucket.back();
                const auto &last = rev_bucket.back();
                const auto &first_query_node = internal_to_external_node_map[first.node_id];
                const auto &last_query_node = internal_to_external_node_map[last.node_id];
                out << "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":["
                    << util::toFloating(first_query_node.lon) << ","
                    << util::toFloating(first_query_node.lat) << "]},\"properties\":{\"id\":\""
                    << first_query_node.node_id << "\"}}\n";
                out << "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\",\"coordinates\":["
                    << util::toFloating(last_query_node.lon) << ","
                    << util::toFloating(last_query_node.lat) << "]},\"properties\":{\"id\":\""
                    << last_query_node.node_id << "\"}}\n";
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

    if (argc < 3)
    {
        util::SimpleLogger().Write(logWARNING) << "usage:\n"
                                               << argv[0]
                                               << " berlin.osrm noramlized_graph.geojson";
        return EXIT_FAILURE;
    }

    std::vector<extractor::QueryNode> internal_to_external_id_map;
    std::vector<NodeID> barrier_list;
    auto graph = tools::loadNodeBasedGraph(
        boost::filesystem::path(argv[1]), barrier_list, internal_to_external_id_map);

    extractor::CompressedEdgeContainer compressed_edge_container;
    extractor::GraphCompressor graph_compressor;

    extractor::RestrictionMap restriction_map;
    std::unordered_set<NodeID> barrier_nodes_map(barrier_list.begin(), barrier_list.end());
    barrier_list.clear();
    barrier_list.shrink_to_fit();

    graph_compressor.Compress(barrier_nodes_map,
                              std::unordered_set<NodeID>{},
                              restriction_map,
                              *graph,
                              compressed_edge_container);

    tools::writeNormalizedGraph(boost::filesystem::path(argv[2]),
                                *graph,
                                compressed_edge_container,
                                internal_to_external_id_map);

    return EXIT_SUCCESS;
}
