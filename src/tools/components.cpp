#include "extractor/files.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/tarjan_scc.hpp"

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/dynamic_graph.hpp"
#include "util/fingerprint.hpp"
#include "util/log.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/filesystem.hpp>
#include <boost/function_output_iterator.hpp>

#include <tbb/parallel_sort.h>

#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace osrm
{
namespace tools
{

using TarjanGraph = util::StaticGraph<void>;
using TarjanEdge = util::static_graph_details::SortableEdgeWithData<void>;

std::size_t loadGraph(const std::string &path,
                      std::vector<util::Coordinate> &coordinate_list,
                      extractor::PackedOSMIDs &osm_node_ids,
                      std::vector<TarjanEdge> &graph_edge_list)
{
    std::vector<extractor::NodeBasedEdge> edge_list;
    std::vector<extractor::NodeBasedEdgeAnnotation> annotation_data;

    auto nop = boost::make_function_output_iterator([](auto) {});

    extractor::files::readRawNBGraph(
        path, nop, nop, coordinate_list, osm_node_ids, edge_list, annotation_data);

    // Building a node-based graph
    for (const auto &input_edge : edge_list)
    {
        if (input_edge.source == input_edge.target)
        {
            continue;
        }

        if (input_edge.flags.forward)
        {
            graph_edge_list.emplace_back(input_edge.source, input_edge.target);
        }

        if (input_edge.flags.backward)
        {
            graph_edge_list.emplace_back(input_edge.target, input_edge.source);
        }
    }

    return osm_node_ids.size();
}

struct FeatureWriter
{
    FeatureWriter(std::ostream &out_) : out(out_)
    {
        out << "{\"type\":\"FeatureCollection\",\"features\":[";
    }

    void AddLine(const util::Coordinate from,
                 const util::Coordinate to,
                 const OSMNodeID from_id,
                 const OSMNodeID to_id,
                 const std::string &type)
    {
        const auto from_lon = static_cast<double>(util::toFloating(from.lon));
        const auto from_lat = static_cast<double>(util::toFloating(from.lat));
        const auto to_lon = static_cast<double>(util::toFloating(to.lon));
        const auto to_lat = static_cast<double>(util::toFloating(to.lat));

        static bool first = true;

        if (!first)
        {
            out << ",";
        }

        out << "{\"type\":\"Feature\",\"properties\":{\"from\":" << from_id << ","
            << "\"to\":" << to_id << ",\"type\":\"" << type
            << "\"},\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[" << from_lon << ","
            << from_lat << "],[" << to_lon << "," << to_lat << "]]}}";

        first = false;
    }

    ~FeatureWriter() { out << "]}" << std::flush; }

    std::ostream &out;
};

//
}
}

int main(int argc, char *argv[])
{
    using namespace osrm;

    util::LogPolicy::GetInstance().Unmute();

    if (argc < 3)
    {
        util::Log(logWARNING) << "Usage: " << argv[0] << " map.osrm components.geojson";
        return EXIT_FAILURE;
    }

    const std::string inpath{argv[1]};
    const std::string outpath{argv[2]};

    if (boost::filesystem::exists(outpath))
    {
        util::Log(logWARNING) << "Components file " << outpath << " already exists";
        return EXIT_FAILURE;
    }

    std::ofstream outfile{outpath};

    if (!outfile)
    {
        util::Log(logWARNING) << "Unable to open components file " << outpath << " for writing";
        return EXIT_FAILURE;
    }

    std::vector<tools::TarjanEdge> graph_edge_list;
    std::vector<util::Coordinate> coordinate_list;
    extractor::PackedOSMIDs osm_node_ids;
    auto number_of_nodes = tools::loadGraph(inpath, coordinate_list, osm_node_ids, graph_edge_list);

    tbb::parallel_sort(graph_edge_list.begin(), graph_edge_list.end());

    const auto graph = std::make_shared<osrm::tools::TarjanGraph>(number_of_nodes, graph_edge_list);
    graph_edge_list.clear();
    graph_edge_list.shrink_to_fit();

    util::Log() << "Starting SCC graph traversal";

    extractor::TarjanSCC<tools::TarjanGraph> tarjan{*graph};
    tarjan.Run();

    util::Log() << "Identified: " << tarjan.GetNumberOfComponents() << " components";
    util::Log() << "Identified " << tarjan.GetSizeOneCount() << " size one components";

    std::uint64_t total_network_length = 0;

    tools::FeatureWriter writer{outfile};

    for (const NodeID source : osrm::util::irange(0u, graph->GetNumberOfNodes()))
    {
        for (const auto current_edge : graph->GetAdjacentEdgeRange(source))
        {
            const auto target = graph->GetTarget(current_edge);

            if (source < target || SPECIAL_EDGEID == graph->FindEdge(target, source))
            {
                BOOST_ASSERT(current_edge != SPECIAL_EDGEID);
                BOOST_ASSERT(source != SPECIAL_NODEID);
                BOOST_ASSERT(target != SPECIAL_NODEID);

                total_network_length += 100 * util::coordinate_calculation::greatCircleDistance(
                                                  coordinate_list[source], coordinate_list[target]);

                auto source_component_id = tarjan.GetComponentID(source);
                auto target_component_id = tarjan.GetComponentID(target);

                auto source_component_size = tarjan.GetComponentSize(source_component_id);
                auto target_component_size = tarjan.GetComponentSize(target_component_id);

                const auto smallest = std::min(source_component_size, target_component_size);

                if (smallest < 1000)
                {
                    auto same_component = source_component_id == target_component_id;
                    std::string type = same_component ? "inner" : "border";

                    writer.AddLine(coordinate_list[source],
                                   coordinate_list[target],
                                   osm_node_ids[source],
                                   osm_node_ids[target],
                                   type);
                }
            }
        }
    }

    util::Log() << "Total network distance: " << (total_network_length / 100 / 1000) << " km";
}
