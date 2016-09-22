#include "extractor/tarjan_scc.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/dynamic_graph.hpp"
#include "util/exception.hpp"
#include "util/fingerprint.hpp"
#include "util/graph_loader.hpp"
#include "util/make_unique.hpp"
#include "util/simple_logger.hpp"
#include "util/static_graph.hpp"
#include "util/typedefs.hpp"

#include <boost/filesystem.hpp>

#if defined(__APPLE__) || defined(_WIN32)
#include <gdal.h>
#include <ogrsf_frmts.h>
#else
#include <gdal/gdal.h>
#include <gdal/ogrsf_frmts.h>
#endif

#include "osrm/coordinate.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace osrm
{
namespace tools
{

struct TarjanEdgeData
{
    TarjanEdgeData() : distance(INVALID_EDGE_WEIGHT), name_id(INVALID_NAMEID) {}
    TarjanEdgeData(unsigned distance, unsigned name_id) : distance(distance), name_id(name_id) {}
    unsigned distance;
    unsigned name_id;
};

using TarjanGraph = util::StaticGraph<TarjanEdgeData>;
using TarjanEdge = TarjanGraph::InputEdge;

void deleteFileIfExists(const std::string &file_name)
{
    if (boost::filesystem::exists(file_name))
    {
        boost::filesystem::remove(file_name);
    }
}

std::size_t loadGraph(const char *path,
                      std::vector<extractor::QueryNode> &coordinate_list,
                      std::vector<TarjanEdge> &graph_edge_list)
{
    std::ifstream input_stream(path, std::ifstream::in | std::ifstream::binary);
    if (!input_stream.is_open())
    {
        throw util::exception("Cannot open osrm file");
    }

    // load graph data
    std::vector<extractor::NodeBasedEdge> edge_list;
    std::vector<NodeID> traffic_light_node_list;
    std::vector<NodeID> barrier_node_list;

    auto number_of_nodes = util::loadNodesFromFile(
        input_stream, barrier_node_list, traffic_light_node_list, coordinate_list);

    util::loadEdgesFromFile(input_stream, edge_list);

    traffic_light_node_list.clear();
    traffic_light_node_list.shrink_to_fit();

    // Building an node-based graph
    for (const auto &input_edge : edge_list)
    {
        if (input_edge.source == input_edge.target)
        {
            continue;
        }

        if (input_edge.forward)
        {
            graph_edge_list.emplace_back(input_edge.source,
                                         input_edge.target,
                                         (std::max)(input_edge.weight, 1),
                                         input_edge.name_id);
        }
        if (input_edge.backward)
        {
            graph_edge_list.emplace_back(input_edge.target,
                                         input_edge.source,
                                         (std::max)(input_edge.weight, 1),
                                         input_edge.name_id);
        }
    }

    return number_of_nodes;
}
}
}

int main(int argc, char *argv[])
{
    using namespace osrm;

    std::vector<osrm::extractor::QueryNode> coordinate_list;
    osrm::util::LogPolicy::GetInstance().Unmute();

    // enable logging
    if (argc < 2)
    {
        osrm::util::SimpleLogger().Write(logWARNING) << "usage:\n" << argv[0] << " <osrm>";
        return EXIT_FAILURE;
    }

    std::vector<osrm::tools::TarjanEdge> graph_edge_list;
    auto number_of_nodes = osrm::tools::loadGraph(argv[1], coordinate_list, graph_edge_list);

    tbb::parallel_sort(graph_edge_list.begin(), graph_edge_list.end());
    const auto graph = std::make_shared<osrm::tools::TarjanGraph>(number_of_nodes, graph_edge_list);
    graph_edge_list.clear();
    graph_edge_list.shrink_to_fit();

    osrm::util::SimpleLogger().Write() << "Starting SCC graph traversal";

    auto tarjan =
        osrm::util::make_unique<osrm::extractor::TarjanSCC<osrm::tools::TarjanGraph>>(graph);
    tarjan->Run();
    osrm::util::SimpleLogger().Write() << "identified: " << tarjan->GetNumberOfComponents()
                                       << " many components";
    osrm::util::SimpleLogger().Write() << "identified " << tarjan->GetSizeOneCount()
                                       << " size 1 SCCs";

    // output
    TIMER_START(SCC_RUN_SETUP);

    // remove files from previous run if exist
    osrm::tools::deleteFileIfExists("component.dbf");
    osrm::tools::deleteFileIfExists("component.shx");
    osrm::tools::deleteFileIfExists("component.shp");

    OGRRegisterAll();

    const char *psz_driver_name = "ESRI Shapefile";
    auto *po_driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(psz_driver_name);
    if (nullptr == po_driver)
    {
        throw osrm::util::exception("ESRI Shapefile driver not available");
    }
    auto *po_datasource = po_driver->CreateDataSource("component.shp", nullptr);

    if (nullptr == po_datasource)
    {
        throw osrm::util::exception("Creation of output file failed");
    }

    auto *po_srs = new OGRSpatialReference();
    po_srs->importFromEPSG(4326);

    auto *po_layer = po_datasource->CreateLayer("component", po_srs, wkbLineString, nullptr);

    if (nullptr == po_layer)
    {
        throw osrm::util::exception("Layer creation failed.");
    }
    TIMER_STOP(SCC_RUN_SETUP);
    osrm::util::SimpleLogger().Write() << "shapefile setup took "
                                       << TIMER_MSEC(SCC_RUN_SETUP) / 1000. << "s";

    uint64_t total_network_length = 0;
    osrm::util::Percent percentage(graph->GetNumberOfNodes());
    TIMER_START(SCC_OUTPUT);
    for (const NodeID source : osrm::util::irange(0u, graph->GetNumberOfNodes()))
    {
        percentage.PrintIncrement();
        for (const auto current_edge : graph->GetAdjacentEdgeRange(source))
        {
            const auto target = graph->GetTarget(current_edge);

            if (source < target || SPECIAL_EDGEID == graph->FindEdge(target, source))
            {
                total_network_length +=
                    100 * osrm::util::coordinate_calculation::greatCircleDistance(
                              coordinate_list[source], coordinate_list[target]);

                BOOST_ASSERT(current_edge != SPECIAL_EDGEID);
                BOOST_ASSERT(source != SPECIAL_NODEID);
                BOOST_ASSERT(target != SPECIAL_NODEID);

                const unsigned size_of_containing_component =
                    std::min(tarjan->GetComponentSize(tarjan->GetComponentID(source)),
                             tarjan->GetComponentSize(tarjan->GetComponentID(target)));

                // edges that end on bollard nodes may actually be in two distinct components
                if (size_of_containing_component < 1000)
                {
                    OGRLineString line_string;
                    line_string.addPoint(
                        static_cast<double>(osrm::util::toFloating(coordinate_list[source].lon)),
                        static_cast<double>(osrm::util::toFloating(coordinate_list[source].lat)));
                    line_string.addPoint(
                        static_cast<double>(osrm::util::toFloating(coordinate_list[target].lon)),
                        static_cast<double>(osrm::util::toFloating(coordinate_list[target].lat)));

                    OGRFeature *po_feature = OGRFeature::CreateFeature(po_layer->GetLayerDefn());

                    po_feature->SetGeometry(&line_string);
                    if (OGRERR_NONE != po_layer->CreateFeature(po_feature))
                    {
                        throw osrm::util::exception("Failed to create feature in shapefile.");
                    }
                    OGRFeature::DestroyFeature(po_feature);
                }
            }
        }
    }
    OGRSpatialReference::DestroySpatialReference(po_srs);
    OGRDataSource::DestroyDataSource(po_datasource);
    TIMER_STOP(SCC_OUTPUT);
    osrm::util::SimpleLogger().Write()
        << "generating output took: " << TIMER_MSEC(SCC_OUTPUT) / 1000. << "s";

    osrm::util::SimpleLogger().Write()
        << "total network distance: " << static_cast<uint64_t>(total_network_length / 100 / 1000.)
        << " km";

    osrm::util::SimpleLogger().Write() << "finished component analysis";
    return EXIT_SUCCESS;
}
