#include "storage/compact.hpp"
#include "customizer/edge_based_graph.hpp"
#include "extractor/node_data_container.hpp"
#include "storage/compact_dataset.hpp"

#include "../common/temporary_file.hpp"
#include "coordinates.hpp"
#include "equal_json.hpp"
#include "fixture.hpp"

#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/tile_parameters.hpp"
#include "engine/api/trip_parameters.hpp"

#include <boost/test/unit_test.hpp>

#include <fstream>
#include <map>

namespace
{
struct CompactFixture
{
    std::filesystem::path root = std::filesystem::temp_directory_path() / random_string(16);
    std::filesystem::path source = root / "source" / "monaco.osrm";
    std::filesystem::path target = root / "compact" / "monaco.osrm";

    CompactFixture()
    {
        std::filesystem::create_directories(source.parent_path());
        for (const auto &entry : std::filesystem::directory_iterator(OSRM_TEST_DATA_DIR "/mld"))
            if (entry.path().filename().string().starts_with("monaco.osrm."))
                std::filesystem::copy_file(entry.path(),
                                           source.parent_path() / entry.path().filename());
    }
    ~CompactFixture() { std::filesystem::remove_all(root); }
};

template <typename Parameters, typename Query>
void checkQuery(const osrm::OSRM &original,
                const osrm::OSRM &compact,
                const Parameters &parameters,
                Query query)
{
    osrm::json::Object before, after;
    const auto before_status = (original.*query)(parameters, before);
    const auto after_status = (compact.*query)(parameters, after);
    BOOST_CHECK(before_status == osrm::Status::Ok);
    BOOST_CHECK(before_status == after_status);
    CHECK_EQUAL_JSON(before, after);
}
} // namespace

BOOST_AUTO_TEST_SUITE(compact)

BOOST_FIXTURE_TEST_CASE(exact_services_and_independent_output, CompactFixture)
{
    std::map<std::filesystem::path, std::pair<std::uintmax_t, std::filesystem::file_time_type>>
        inputs;
    for (const auto &entry : std::filesystem::directory_iterator(source.parent_path()))
        inputs.emplace(entry.path(), std::make_pair(entry.file_size(), entry.last_write_time()));
    osrm::storage::compactDataset(source, target);
    for (const auto &[path, state] : inputs)
    {
        BOOST_CHECK_EQUAL(std::filesystem::file_size(path), state.first);
        BOOST_CHECK(std::filesystem::last_write_time(path) == state.second);
    }
    BOOST_CHECK(
        !std::filesystem::exists(target.parent_path() / osrm::storage::COMPACT_INCOMPLETE_FILE));
    for (const auto &entry : std::filesystem::directory_iterator(target.parent_path()))
    {
        BOOST_CHECK(entry.is_regular_file());
        BOOST_CHECK(!entry.is_symlink());
        BOOST_CHECK(std::filesystem::hard_link_count(entry.path()) == 1);
    }
    osrm::EngineConfig original_config;
    original_config.storage_config = {source};
    original_config.use_shared_memory = false;
    original_config.use_mmap = false;
    original_config.algorithm = osrm::EngineConfig::Algorithm::MLD;
    osrm::OSRM original(original_config);
    osrm::EngineConfig config;
    config.storage_config = {target};
    config.use_shared_memory = false;
    config.use_mmap = false;
    config.algorithm = osrm::EngineConfig::Algorithm::MLD;
    std::filesystem::rename(source.parent_path(), root / "hidden-source");
    osrm::OSRM optimized(config);
    BOOST_CHECK(config.share_cell_metrics);

    // No source paths may be needed by the new dataset after preparation.
    const auto coordinates = get_locations_in_big_component();
    using RouteQuery =
        osrm::Status (osrm::OSRM::*)(const osrm::RouteParameters &, osrm::json::Object &) const;
    using NearestQuery =
        osrm::Status (osrm::OSRM::*)(const osrm::NearestParameters &, osrm::json::Object &) const;
    using TableQuery =
        osrm::Status (osrm::OSRM::*)(const osrm::TableParameters &, osrm::json::Object &) const;
    using TripQuery =
        osrm::Status (osrm::OSRM::*)(const osrm::TripParameters &, osrm::json::Object &) const;
    using MatchQuery =
        osrm::Status (osrm::OSRM::*)(const osrm::MatchParameters &, osrm::json::Object &) const;
    osrm::RouteParameters route;
    route.coordinates = coordinates;
    route.steps = true;
    route.annotations = true;
    route.annotations_type = osrm::RouteParameters::AnnotationsType::All;
    route.overview = osrm::RouteParameters::OverviewType::Full;
    checkQuery(original, optimized, route, static_cast<RouteQuery>(&osrm::OSRM::Route));
    route.exclude = {"motorway"};
    checkQuery(original, optimized, route, static_cast<RouteQuery>(&osrm::OSRM::Route));

    for (const auto &coordinate : coordinates)
    {
        osrm::NearestParameters nearest;
        nearest.coordinates = {coordinate};
        nearest.number_of_results = 5;
        checkQuery(original, optimized, nearest, static_cast<NearestQuery>(&osrm::OSRM::Nearest));
    }
    osrm::TableParameters table;
    table.coordinates = coordinates;
    table.annotations = osrm::TableParameters::AnnotationsType::All;
    checkQuery(original, optimized, table, static_cast<TableQuery>(&osrm::OSRM::Table));
    osrm::TripParameters trip;
    trip.coordinates = coordinates;
    trip.steps = true;
    checkQuery(original, optimized, trip, static_cast<TripQuery>(&osrm::OSRM::Trip));
    osrm::MatchParameters match;
    match.coordinates = get_split_trace_locations();
    match.steps = true;
    match.radiuses = {50., 50., 50., 50.};
    checkQuery(original, optimized, match, static_cast<MatchQuery>(&osrm::OSRM::Match));
    for (unsigned x = 8528; x <= 8530; ++x)
        for (unsigned y = 5975; y <= 5977; ++y)
        {
            osrm::TileParameters tile{x, y, 14};
            std::string before, after;
            BOOST_CHECK(original.Tile(tile, before) == optimized.Tile(tile, after));
            BOOST_CHECK_EQUAL(before, after);
        }
}

BOOST_FIXTURE_TEST_CASE(refuses_existing_output_and_compact_input, CompactFixture)
{
    std::filesystem::create_directory(target.parent_path());
    const auto sentinel = target.parent_path() / "keep-me";
    std::ofstream(sentinel) << "existing user file\n";
    BOOST_CHECK_THROW(osrm::storage::compactDataset(source, target), osrm::util::exception);
    BOOST_CHECK(std::filesystem::exists(sentinel));
    std::filesystem::remove_all(target.parent_path());
    osrm::storage::compactDataset(source, target);
    BOOST_CHECK_THROW(osrm::storage::compactDataset(target, root / "second" / "monaco.osrm"),
                      osrm::util::exception);
    BOOST_CHECK(!std::filesystem::exists(root / "second"));

    osrm::EngineConfig config;
    config.storage_config = {target};
    config.use_shared_memory = false;
    config.use_mmap = false;
    config.algorithm = osrm::EngineConfig::Algorithm::CH;
    BOOST_CHECK_THROW(osrm::OSRM{config}, osrm::util::exception);
    config.algorithm = osrm::EngineConfig::Algorithm::MLD;
    config.use_mmap = true;
    BOOST_CHECK_THROW(osrm::OSRM{config}, osrm::util::exception);
}

BOOST_FIXTURE_TEST_CASE(failed_conversion_removes_only_its_output, CompactFixture)
{
    {
        osrm::storage::tar::FileWriter writer(source.string() + ".mldgr",
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        writer.Close();
    }
    BOOST_CHECK_THROW(osrm::storage::compactDataset(source, target), osrm::util::exception);
    BOOST_CHECK(!std::filesystem::exists(target.parent_path()));
    BOOST_CHECK(std::filesystem::exists(source.string() + ".mldgr"));
}

BOOST_AUTO_TEST_CASE(packed_node_boundaries_and_flags)
{
    std::uint64_t words[] = {~std::uint64_t(0), 12345ULL | (7ULL << 25)};
    osrm::extractor::NodeBasedEdgeAnnotation annotation{};
    annotation.classes = 7;
    const osrm::extractor::EdgeBasedNodeDataView view{{words, 2}, {&annotation, 1}};
    const auto maximum = view.GetNode(0), ordinary = view.GetNode(1);
    BOOST_CHECK_EQUAL(maximum.geometry_id.id, 0xffffff);
    BOOST_CHECK(maximum.geometry_id.forward);
    BOOST_CHECK_EQUAL(maximum.component_id.id, 0x1ffff);
    BOOST_CHECK(maximum.component_id.is_tiny);
    BOOST_CHECK_EQUAL(maximum.annotation_id, 0xfffff);
    BOOST_CHECK(maximum.segregated);
    BOOST_CHECK_EQUAL(ordinary.geometry_id.id, 12345);
    BOOST_CHECK_EQUAL(ordinary.component_id.id, 7);
    BOOST_CHECK(!ordinary.geometry_id.forward && !ordinary.component_id.is_tiny &&
                !ordinary.segregated);
    BOOST_CHECK_EQUAL(ordinary.annotation_id, 0);
    BOOST_CHECK_EQUAL(view.GetClassData(1), 7);
}

BOOST_AUTO_TEST_CASE(packed_edge_boundaries_and_adjacency)
{
    using Graph = osrm::util::StaticGraph<osrm::customizer::EdgeBasedGraphEdgeData,
                                          osrm::storage::Ownership::View>;
    Graph::NodeArrayEntry nodes[] = {{0}, {2}, {3}, {4}};
    std::uint8_t bytes[4 * 6 + 8]{};
    const std::uint32_t targets[] = {1, 2, 0, (1u << 25) - 1};
    const std::uint32_t turns[] = {0, (1u << 22) - 1, 1234, 777};
    for (unsigned i = 0; i < 4; ++i)
    {
        const auto bits = std::uint64_t(targets[i]) | (std::uint64_t(turns[i]) << 25);
        std::memcpy(bytes + i * 6, &bits, 6);
    }
    Graph graph{{nodes, 4}, {}};
    graph.bindPackedEdges({bytes, sizeof(bytes)});
    const auto &view = graph;
    for (unsigned i = 0; i < 4; ++i)
    {
        BOOST_CHECK_EQUAL(view.GetTarget(i), targets[i]);
        BOOST_CHECK_EQUAL(view.GetEdgeData(i).turn_id, turns[i]);
    }
    BOOST_CHECK_EQUAL(view.GetNumberOfNodes(), 3);
    BOOST_CHECK_EQUAL(view.GetNumberOfEdges(), 4);
    BOOST_CHECK_EQUAL(view.BeginEdges(1), 2);
    BOOST_CHECK_EQUAL(view.EndEdges(1), 3);
    BOOST_CHECK_EQUAL(view.FindEdge(0, 2), 1);
    BOOST_CHECK_EQUAL(view.FindEdge(0, 0), SPECIAL_EDGEID);
}

BOOST_FIXTURE_TEST_CASE(ambiguous_output_path_cannot_redirect_writes, CompactFixture)
{
    const auto ambiguous_parent = root / "ambiguous.osrm";
    std::filesystem::create_directory(ambiguous_parent);
    const auto redirected_parent = root / "ambiguous" / "new";
    std::filesystem::create_directories(redirected_parent);
    const auto sentinel = redirected_parent / "map.osrm.osrm.edges";
    const std::string expected = "Existing data must remain untouched";
    std::ofstream(sentinel) << expected;
    BOOST_CHECK_THROW(osrm::storage::compactDataset(source, ambiguous_parent / "new" / "map.osrm"),
                      osrm::util::exception);
    BOOST_CHECK(!std::filesystem::exists(ambiguous_parent / "new"));
    std::ifstream stream(sentinel);
    const std::string actual((std::istreambuf_iterator<char>(stream)), {});
    BOOST_CHECK_EQUAL(actual, expected);
}

BOOST_FIXTURE_TEST_CASE(mismatched_redirected_graph_is_rejected, CompactFixture)
{
    const auto wrong = root / "wrong.osrm";
    {
        osrm::storage::tar::FileReader reader(source.string() + ".mldgr",
                                              osrm::storage::tar::FileReader::VerifyFingerprint);
        osrm::storage::tar::FileWriter writer(wrong.string() + ".mldgr",
                                              osrm::storage::tar::FileWriter::HasNoFingerprint);
        std::vector<osrm::storage::tar::FileReader::FileEntry> entries;
        reader.List(std::back_inserter(entries));
        for (const auto &entry : entries)
        {
            if (entry.name == "/mld/connectivity_checksum")
            {
                std::uint32_t checksum;
                reader.ReadInto(entry.name, checksum);
                checksum ^= 0xffffffff;
                writer.WriteFrom(entry.name, checksum);
            }
            else
            {
                std::vector<char> bytes(entry.size);
                reader.ReadInto(entry.name, bytes.data(), bytes.size());
                writer.WriteFrom(entry.name, bytes.data(), bytes.size());
            }
        }
        writer.Close();
    }
    std::filesystem::copy_file(source.string() + ".cell_metrics", wrong.string() + ".cell_metrics");
    BOOST_CHECK_EXCEPTION(
        osrm::storage::compactDataset(source, target, wrong),
        osrm::util::exception,
        [](const auto &error)
        { return std::string(error.what()).find("connectivity checksums") != std::string::npos; });
    BOOST_CHECK(!std::filesystem::exists(target.parent_path()));
}

BOOST_AUTO_TEST_SUITE_END()
