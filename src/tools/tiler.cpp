#include "util/typedefs.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/dynamic_graph.hpp"
#include "util/static_graph.hpp"
#include "util/fingerprint.hpp"
#include "util/graph_loader.hpp"
#include "util/make_unique.hpp"
#include "util/exception.hpp"
#include "util/simple_logger.hpp"
#include "util/binary_heap.hpp"

#include "engine/datafacade/internal_datafacade.hpp"

#include "util/routed_options.hpp"

#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>

#include "osrm/coordinate.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

namespace osrm
{
namespace tools
{

struct BoundingBox {
    FixedPointCoordinate southwest;
    FixedPointCoordinate northeast;
};

BoundingBox TileToBBOX(int z, int x, int y) 
{
    // A small box in SF near the marina covering the intersection
    // of Powell and Embarcadero
    return { { static_cast<int32_t>(37.80781742045232 * COORDINATE_PRECISION) , static_cast<int32_t>(-122.4139380455017 * COORDINATE_PRECISION) },
             { static_cast<int32_t>(37.809410993963944 * COORDINATE_PRECISION), static_cast<int32_t>(-122.41186738014221 * COORDINATE_PRECISION) } };
}
}
}

int main(int argc, char *argv[])
{
    std::vector<osrm::extractor::QueryNode> coordinate_list;
    osrm::util::LogPolicy::GetInstance().Unmute();

    // enable logging
    if (argc < 5)
    {
        osrm::util::SimpleLogger().Write(logWARNING) << "usage:\n" << argv[0] << " <filename.osrm> <z> <x> <y>";
        return EXIT_FAILURE;
    }

    // Set up the datafacade for querying
    std::unordered_map<std::string, boost::filesystem::path> server_paths;
    server_paths["base"] = std::string(argv[1]);
    osrm::util::populate_base_path(server_paths);
    osrm::engine::datafacade::InternalDataFacade<osrm::contractor::QueryEdge::EdgeData> datafacade(server_paths);


    // Step 1 - convert z,x,y into tile bounds
    //
    
    int z = std::stoi(argv[2]);
    int x = std::stoi(argv[3]);
    int y = std::stoi(argv[4]);
    
    auto bbox = util::coordinate_calculation::mercator::TileToBBOX(z,x,y); // @karenzshea - implement this function!!
    
    // Step 2 - Get all the features from those bounds
    //
    //
    auto edges = datafacade.GetEdgesInBox(bbox.southwest, bbox.northeast);
    
    // Step 3 - Encode those features as Mapbox Vector Tiles
    //
    //
    for (const auto & edge : edges)
    {
        const auto a = datafacade.GetCoordinateOfNode(edge.u);
        const auto b = datafacade.GetCoordinateOfNode(edge.v);
        std::cout << "Feature: " << a << " to " << b << std::endl;
    }
    
    // Step 4 - Output the result
    //

    return EXIT_SUCCESS;
}
