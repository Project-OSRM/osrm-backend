#include "engine/datafacade/internal_datafacade.hpp"
#include "engine/engine_config.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/integer_range.hpp"

#include <boost/filesystem.hpp>

#include <fstream>

using namespace osrm;

void dumpSpeedsToCSV(engine::datafacade::InternalDataFacade &facade, const char *path)
{
    std::ofstream out(path);

    auto dump_segment_speeds = [&out, &facade](const std::vector<NodeID> &ids,
                                               const std::vector<EdgeWeight> &durations) {
        for (auto segment_index : util::irange<std::size_t>(0, durations.size()))
        {
            auto first_id = ids[segment_index];
            auto second_id = ids[segment_index + 1];
            auto duration = durations[segment_index] / 10.;
            auto first_coordinate = facade.GetCoordinateOfNode(first_id);
            auto second_coordinate = facade.GetCoordinateOfNode(second_id);
            auto distance = util::coordinate_calculation::haversineDistance(first_coordinate,
                                                                            second_coordinate);
            auto segment_speed_kmh = 3.6 * (distance / duration);
            out << facade.GetOSMNodeIDOfNode(first_id) << ","
                << facade.GetOSMNodeIDOfNode(second_id) << "," << segment_speed_kmh << std::endl;
        }
    };

    for (auto edge_id : util::irange<EdgeID>(0, facade.GetNumberOfNonShortcutEdges()))
    {
        const auto geometry_index = facade.GetGeometryIndexForEdgeID(edge_id);
        if (geometry_index.forward)
        {
            const auto forward_geometry = facade.GetUncompressedForwardGeometry(geometry_index.id);
            const auto forward_durations = facade.GetUncompressedForwardWeights(geometry_index.id);
            dump_segment_speeds(forward_geometry, forward_durations);
        }
        else
        {
            const auto reverse_geometry = facade.GetUncompressedReverseGeometry(geometry_index.id);
            const auto reverse_durations = facade.GetUncompressedReverseWeights(geometry_index.id);
            dump_segment_speeds(reverse_geometry, reverse_durations);
        }
    }
}

int main(int argc, const char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();

    if (argc < 3)
    {
        util::SimpleLogger().Write(logWARNING) << "You need to specify the path to an .osrm file "
                                                  "and the output path.\nExample: "
                                                  "./osrm-speeds-to-csv berlin.osrm speeds.csv";
        return EXIT_FAILURE;
    }

    engine::EngineConfig config;
    boost::filesystem::path base_path(argv[1]);
    config.storage_config = storage::StorageConfig(base_path);

    if (!config.IsValid())
    {
        if (!boost::filesystem::is_regular_file(config.storage_config.ram_index_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.ram_index_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.file_index_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.file_index_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.hsgr_data_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.hsgr_data_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.nodes_data_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.nodes_data_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.edges_data_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.edges_data_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.core_data_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.core_data_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.geometries_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.geometries_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.timestamp_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.timestamp_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.datasource_names_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.datasource_names_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.datasource_indexes_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.datasource_indexes_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.names_data_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.names_data_path
                                                   << " is not found";
        }
        if (!boost::filesystem::is_regular_file(config.storage_config.properties_path))
        {
            util::SimpleLogger().Write(logWARNING) << config.storage_config.properties_path
                                                   << " is not found";
        }
        return EXIT_FAILURE;
    }

    engine::datafacade::InternalDataFacade facade(config.storage_config);

    dumpSpeedsToCSV(facade, argv[2]);

    util::SimpleLogger().Write() << "shutdown completed";
}
catch (const std::bad_alloc &e)
{
    util::SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    util::SimpleLogger().Write(logWARNING)
        << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
