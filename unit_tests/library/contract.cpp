#include <boost/test/unit_test.hpp>

#include "../common/temporary_file.hpp"

#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/files.hpp"
#include "osrm/contractor.hpp"
#include "osrm/contractor_config.hpp"

#include "guidance/files.hpp"
#include "storage/compact_dataset.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace
{
struct TrafficOutputFixture
{
    const std::filesystem::path Root = std::filesystem::temp_directory_path() / random_string(16);
    const std::filesystem::path Source = Root / "monaco.osrm";

    TrafficOutputFixture()
    {
        std::filesystem::create_directory(Root);
        for (const auto &entry : std::filesystem::directory_iterator(OSRM_TEST_DATA_DIR "/ch"))
        {
            if (entry.path().filename().string().starts_with("monaco.osrm."))
                std::filesystem::copy_file(entry.path(), Root / entry.path().filename());
        }
    }

    ~TrafficOutputFixture()
    {
        std::error_code error;
        std::filesystem::remove_all(Root, error);
    }
};
} // namespace

namespace
{
struct ContractionFixture
{
    std::filesystem::path Root = std::filesystem::temp_directory_path() / random_string(16);
    std::filesystem::path Source = Root / "monaco.osrm";

    ContractionFixture()
    {
        std::filesystem::create_directory(Root);
        for (const auto &entry : std::filesystem::directory_iterator(OSRM_TEST_DATA_DIR "/ch"))
            if (entry.path().filename().string().starts_with("monaco.osrm."))
                std::filesystem::copy_file(entry.path(), Root / entry.path().filename());
    }
    ~ContractionFixture() { std::filesystem::remove_all(Root); }

    std::uint32_t readChecksum() const
    {
        osrm::storage::tar::FileReader reader(Source.string() + ".edges",
                                              osrm::storage::tar::FileReader::VerifyFingerprint);
        std::uint32_t checksum;
        reader.ReadInto("/common/connectivity_checksum", checksum);
        return checksum;
    }

    void writeUnusedMLD(std::uint32_t checksum) const
    {
        {
            osrm::storage::tar::FileWriter writer(
                Source.string() + ".mldgr", osrm::storage::tar::FileWriter::GenerateFingerprint);
            writer.WriteFrom("/mld/connectivity_checksum", checksum);
        }
        std::ofstream(Source.string() + ".cell_metrics") << "unused stale metrics";
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(library_contract)

BOOST_AUTO_TEST_CASE(test_contract_with_invalid_config)
{
    osrm::ContractorConfig config;
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_THROW(osrm::contract(config),
                      std::exception); // including osrm::util::exception, etc.
}

BOOST_FIXTURE_TEST_CASE(redirected_contraction_preserves_traffic_inputs, TrafficOutputFixture)
{
    const auto read_bytes = [](const std::string &path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::vector<char>(std::istreambuf_iterator<char>(stream), {});
    };
    std::map<std::string, std::vector<char>> original;
    for (const auto *suffix :
         {".geometry", ".datasource_names", ".turn_weight_penalties", ".turn_duration_penalties"})
    {
        auto bytes = read_bytes(Source.string() + suffix);
        BOOST_REQUIRE(!bytes.empty());
        original.emplace(suffix, std::move(bytes));
    }

    std::vector<osrm::util::Coordinate> coordinates;
    osrm::extractor::PackedOSMIDs osm_nodes;
    osrm::extractor::files::readNodes(Source.string() + ".nbg_nodes", coordinates, osm_nodes);
    std::vector<osrm::extractor::lookup::TurnIndexBlock> turns;
    osrm::extractor::files::readTurnPenaltiesIndex(Source.string() + ".turn_penalties_index",
                                                   turns);
    BOOST_REQUIRE(!turns.empty());

    const auto turn = turns.front();
    const auto speed = Root / "speed.csv";
    const auto penalty = Root / "penalty.csv";
    std::ofstream(speed) << osm_nodes[turn.from_id] << ',' << osm_nodes[turn.via_id] << ",5\n";
    std::ofstream(penalty) << osm_nodes[turn.from_id] << ',' << osm_nodes[turn.via_id] << ','
                           << osm_nodes[turn.to_id] << ",37\n";

    osrm::ContractorConfig config;
    config.UseDefaultOutputNames(Source);
    config.output_path = Root / "redirected";
    config.requested_num_threads = 2;
    config.updater_config.segment_speed_lookup_paths = {speed.string()};
    config.updater_config.turn_penalty_lookup_paths = {penalty.string()};
    BOOST_REQUIRE_NO_THROW(osrm::contract(config));

    for (const auto &[suffix, bytes] : original)
    {
        BOOST_CHECK(read_bytes(Source.string() + suffix) == bytes);
        BOOST_CHECK(std::filesystem::exists(config.output_path.string() + ".osrm" + suffix));
    }
    BOOST_CHECK(std::filesystem::exists(config.GetOutputPath(".osrm.hsgr")));
    std::vector<TurnPenalty> updated;
    osrm::extractor::files::readTurnDurationPenalty(
        config.output_path.string() + ".osrm.turn_duration_penalties", updated);
    BOOST_REQUIRE(!updated.empty());
    BOOST_CHECK(updated.front() == TurnPenalty{370});
}

BOOST_FIXTURE_TEST_CASE(contraction_replaces_stale_query_archive, ContractionFixture)
{
    writeUnusedMLD(readChecksum() ^ std::uint32_t{1});
    std::ofstream(Source.string() + ".query") << "stale query data";
    osrm::ContractorConfig config;
    config.UseDefaultOutputNames(Source);
    config.requested_num_threads = 2;
    BOOST_CHECK_NO_THROW(osrm::contract(config));
    BOOST_CHECK(!std::filesystem::exists(Source.string() + ".query"));
    osrm::storage::tar::FileReader graph(Source.string() + ".hsgr",
                                         osrm::storage::tar::FileReader::VerifyFingerprint);
    BOOST_CHECK(graph.HasEntry("/ch/turn_data/turn_instructions"));
    BOOST_CHECK(graph.HasEntry("/ch/turn_penalty/weight"));
}

BOOST_FIXTURE_TEST_CASE(partitioned_mld_waits_for_customization, ContractionFixture)
{
    writeUnusedMLD(readChecksum());
    std::filesystem::remove(Source.string() + ".query");
    osrm::ContractorConfig config;
    config.UseDefaultOutputNames(Source);
    config.requested_num_threads = 2;
    BOOST_CHECK_NO_THROW(osrm::contract(config));
    BOOST_CHECK(!std::filesystem::exists(Source.string() + ".query"));
}

BOOST_FIXTURE_TEST_CASE(inactive_preprocessing_edges_need_no_turn_record, ContractionFixture)
{
    const auto checksum = readChecksum();
    osrm::extractor::files::writeEdgeBasedGraph(
        Source.string() + ".ebg", 1, std::vector<osrm::extractor::EdgeBasedEdge>(1), checksum);
    osrm::guidance::files::writeTurnData(
        Source.string() + ".edges", osrm::guidance::TurnDataContainer{}, checksum);
    const std::vector<TurnPenalty> penalties;
    osrm::extractor::files::writeTurnWeightPenalty(Source.string() + ".turn_weight_penalties",
                                                   penalties);
    osrm::extractor::files::writeTurnDurationPenalty(Source.string() + ".turn_duration_penalties",
                                                     penalties);
    osrm::ContractorConfig config;
    config.UseDefaultOutputNames(Source);
    config.requested_num_threads = 2;
    BOOST_CHECK_NO_THROW(osrm::contract(config));
}

BOOST_AUTO_TEST_SUITE_END()
