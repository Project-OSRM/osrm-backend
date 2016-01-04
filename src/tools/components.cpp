#include "util/typedefs.hpp"
#include "extractor/tarjan_scc.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/dynamic_graph.hpp"
#include "util/static_graph.hpp"
#include "util/fingerprint.hpp"
#include "util/graph_loader.hpp"
#include "util/make_unique.hpp"
#include "util/osrm_exception.hpp"
#include "util/simple_logger.hpp"

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

namespace
{

struct TarjanEdgeData
{
    TarjanEdgeData() : distance(INVALID_EDGE_WEIGHT), name_id(INVALID_NAMEID) {}
    TarjanEdgeData(unsigned distance, unsigned name_id) : distance(distance), name_id(name_id) {}
    unsigned distance;
    unsigned name_id;
};

using TarjanGraph = StaticGraph<TarjanEdgeData>;
using TarjanEdge = TarjanGraph::InputEdge;

void deleteFileIfExists(const std::string &file_name)
{
    if (boost::filesystem::exists(file_name))
    {
        boost::filesystem::remove(file_name);
    }
}
}

std::size_t loadGraph(const char *path,
                      std::vector<QueryNode> &coordinate_list,
                      std::vector<TarjanEdge> &graph_edge_list)
{
    std::ifstream input_stream(path, std::ifstream::in | std::ifstream::binary);
    if (!input_stream.is_open())
    {
        throw osrm::exception("Cannot open osrm file");
    }

    // load graph data
    std::vector<NodeBasedEdge> edge_list;
    std::vector<NodeID> traffic_light_node_list;
    std::vector<NodeID> barrier_node_list;

    auto number_of_nodes = loadNodesFromFile(input_stream, barrier_node_list,
                                             traffic_light_node_list, coordinate_list);

    loadEdgesFromFile(input_stream, edge_list);

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
            graph_edge_list.emplace_back(input_edge.source, input_edge.target,
                                         (std::max)(input_edge.weight, 1), input_edge.name_id);
        }
        if (input_edge.backward)
        {
            graph_edge_list.emplace_back(input_edge.target, input_edge.source,
                                         (std::max)(input_edge.weight, 1), input_edge.name_id);
        }
    }

    return number_of_nodes;
}

int main(int argc, char *argv[])
{
    std::vector<QueryNode> coordinate_list;

    LogPolicy::GetInstance().Unmute();
    try
    {
        // enable logging
        if (argc < 2)
        {
            SimpleLogger().Write(logWARNING) << "usage:\n" << argv[0] << " <osrm>";
            return -1;
        }

        std::vector<TarjanEdge> graph_edge_list;
        auto number_of_nodes = loadGraph(argv[1], coordinate_list, graph_edge_list);

        tbb::parallel_sort(graph_edge_list.begin(), graph_edge_list.end());
        const auto graph = std::make_shared<TarjanGraph>(number_of_nodes, graph_edge_list);
        graph_edge_list.clear();
        graph_edge_list.shrink_to_fit();

        SimpleLogger().Write() << "Starting SCC graph traversal";

        auto tarjan = osrm::make_unique<TarjanSCC<TarjanGraph>>(graph);
        tarjan->run();
        SimpleLogger().Write() << "identified: " << tarjan->get_number_of_components()
                               << " many components";
        SimpleLogger().Write() << "identified " << tarjan->get_size_one_count() << " size 1 SCCs";

        // output
        TIMER_START(SCC_RUN_SETUP);

        // remove files from previous run if exist
        deleteFileIfExists("component.dbf");
        deleteFileIfExists("component.shx");
        deleteFileIfExists("component.shp");

        Percent percentage(graph->GetNumberOfNodes());

        OGRRegisterAll();

        const char *psz_driver_name = "ESRI Shapefile";
        auto *po_driver =
            OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(psz_driver_name);
        if (nullptr == po_driver)
        {
            throw osrm::exception("ESRI Shapefile driver not available");
        }
        auto *po_datasource = po_driver->CreateDataSource("component.shp", nullptr);

        if (nullptr == po_datasource)
        {
            throw osrm::exception("Creation of output file failed");
        }

        auto *po_srs = new OGRSpatialReference();
        po_srs->importFromEPSG(4326);

        auto *po_layer = po_datasource->CreateLayer("component", po_srs, wkbLineString, nullptr);

        if (nullptr == po_layer)
        {
            throw osrm::exception("Layer creation failed.");
        }
        TIMER_STOP(SCC_RUN_SETUP);
        SimpleLogger().Write() << "shapefile setup took " << TIMER_MSEC(SCC_RUN_SETUP) / 1000.
                               << "s";

        uint64_t total_network_length = 0;
        percentage.reinit(graph->GetNumberOfNodes());
        TIMER_START(SCC_OUTPUT);
        for (const NodeID source : osrm::irange(0u, graph->GetNumberOfNodes()))
        {
            percentage.printIncrement();
            for (const auto current_edge : graph->GetAdjacentEdgeRange(source))
            {
                const TarjanGraph::NodeIterator target = graph->GetTarget(current_edge);

                if (source < target || SPECIAL_EDGEID == graph->FindEdge(target, source))
                {
                    total_network_length +=
                        100 * coordinate_calculation::great_circle_distance(
                                  coordinate_list[source].lat, coordinate_list[source].lon,
                                  coordinate_list[target].lat, coordinate_list[target].lon);

                    BOOST_ASSERT(current_edge != SPECIAL_EDGEID);
                    BOOST_ASSERT(source != SPECIAL_NODEID);
                    BOOST_ASSERT(target != SPECIAL_NODEID);

                    const unsigned size_of_containing_component =
                        std::min(tarjan->get_component_size(tarjan->get_component_id(source)),
                                 tarjan->get_component_size(tarjan->get_component_id(target)));

                    // edges that end on bollard nodes may actually be in two distinct components
                    if (size_of_containing_component < 1000)
                    {
                        OGRLineString line_string;
                        line_string.addPoint(coordinate_list[source].lon / COORDINATE_PRECISION,
                                             coordinate_list[source].lat / COORDINATE_PRECISION);
                        line_string.addPoint(coordinate_list[target].lon / COORDINATE_PRECISION,
                                            coordinate_list[target].lat / COORDINATE_PRECISION);

                        OGRFeature *po_feature = OGRFeature::CreateFeature(po_layer->GetLayerDefn());

                        po_feature->SetGeometry(&line_string);
                        if (OGRERR_NONE != po_layer->CreateFeature(po_feature))
                        {
                            throw osrm::exception("Failed to create feature in shapefile.");
                        }
                        OGRFeature::DestroyFeature(po_feature);
                    }
                }
            }
        }
        OGRSpatialReference::DestroySpatialReference(po_srs);
        OGRDataSource::DestroyDataSource(po_datasource);
        TIMER_STOP(SCC_OUTPUT);
        SimpleLogger().Write() << "generating output took: " << TIMER_MSEC(SCC_OUTPUT) / 1000.
                               << "s";

        SimpleLogger().Write() << "total network distance: "
                               << static_cast<uint64_t>(total_network_length / 100 / 1000.)
                               << " km";

        SimpleLogger().Write() << "finished component analysis";
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    }
    return 0;
}
