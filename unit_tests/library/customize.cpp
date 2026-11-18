#include <boost/test/unit_test.hpp>

#include "osrm/customizer.hpp"
#include "osrm/customizer_config.hpp"

#include "../common/temporary_file.hpp"
#include "coordinates.hpp"
#include "equal_json.hpp"

#include "extractor/files.hpp"
#include "storage/compact.hpp"
#include "storage/compact_dataset.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"

#include <fstream>
#include <map>
#include <thread>

namespace
{
struct CustomizationFixture
{
    std::filesystem::path Root = std::filesystem::temp_directory_path() / random_string(16);
    std::filesystem::path Source = Root / "source with spaces" / "monaco.osrm";
    std::filesystem::path Target = Root / "compact generation" / "monaco.osrm";

    CustomizationFixture()
    {
        std::filesystem::create_directories(Source.parent_path());
        for (const auto &entry : std::filesystem::directory_iterator(OSRM_TEST_DATA_DIR "/mld"))
            if (entry.path().filename().string().starts_with("monaco.osrm."))
                std::filesystem::copy_file(entry.path(),
                                           Source.parent_path() / entry.path().filename());
    }
    ~CustomizationFixture() { std::filesystem::remove_all(Root); }

    osrm::CustomizationConfig getConfig() const
    {
        osrm::CustomizationConfig config;
        config.UseDefaultOutputNames(Source);
        config.requested_num_threads = 2;
        config.CompactOutputPath = Target;
        return config;
    }

    void checkGeneration(const std::filesystem::path &target,
                         const std::filesystem::path &mld_input = {}) const
    {
        const auto reference = Root / random_string(16) / "reference.osrm";
        osrm::storage::compactDataset(Source, reference, mld_input);
        unsigned count = 0;
        for (const auto &entry : std::filesystem::directory_iterator(reference.parent_path()))
        {
            const auto suffix = entry.path().string().substr(reference.string().size());
            const auto output = std::filesystem::path(target.string() + suffix);
            BOOST_REQUIRE(std::filesystem::is_regular_file(output));
            BOOST_CHECK(!std::filesystem::is_symlink(output));
            BOOST_CHECK_EQUAL(std::filesystem::hard_link_count(output), 1);
            std::ifstream a(entry.path(), std::ios::binary), b(output, std::ios::binary);
            BOOST_CHECK(std::equal(std::istreambuf_iterator<char>(a),
                                   std::istreambuf_iterator<char>(),
                                   std::istreambuf_iterator<char>(b),
                                   std::istreambuf_iterator<char>()));
            ++count;
        }
        unsigned expected_count = 20;
        for (const auto *suffix : {".openareas", ".openareas.ramIndex", ".openareas.fileIndex"})
            expected_count += std::filesystem::exists(Source.string() + suffix);
        BOOST_CHECK_EQUAL(count, expected_count);
        unsigned output_count = 0;
        for (const auto &entry : std::filesystem::directory_iterator(target.parent_path()))
        {
            BOOST_CHECK(std::filesystem::is_regular_file(entry.path()));
            ++output_count;
        }
        BOOST_CHECK_EQUAL(output_count, expected_count);
    }

    std::filesystem::path writeTraffic() const
    {
        osrm::extractor::SegmentDataContainer segments;
        osrm::extractor::files::readSegmentData(Source.string() + ".geometry", segments);
        std::vector<osrm::util::Coordinate> coordinates;
        osrm::extractor::PackedOSMIDs ids;
        osrm::extractor::files::readNodes(Source.string() + ".nbg_nodes", coordinates, ids);
        const auto traffic = Root / "traffic.csv";
        std::ofstream stream(traffic);
        for (unsigned geometry = 0; geometry < segments.GetNumberOfGeometries(); ++geometry)
        {
            const auto nodes = segments.GetForwardGeometry(geometry);
            const auto weights = segments.GetForwardWeights(geometry);
            for (unsigned offset = 0; offset < weights.size(); ++offset)
                if (weights[offset] > SegmentWeight{0} && weights[offset] != INVALID_SEGMENT_WEIGHT)
                {
                    stream << ids[nodes[offset]] << ',' << ids[nodes[offset + 1]] << ",1\n";
                    return traffic;
                }
        }
        BOOST_FAIL("Fixture must have an updatable road segment");
        return traffic;
    }
};

osrm::OSRM loadMLD(const std::filesystem::path &path)
{
    osrm::EngineConfig config;
    config.storage_config = {path};
    config.use_shared_memory = false;
    config.use_mmap = false;
    config.algorithm = osrm::EngineConfig::Algorithm::MLD;
    return osrm::OSRM(config);
}
} // namespace

BOOST_AUTO_TEST_SUITE(library_customize)

BOOST_AUTO_TEST_CASE(test_customize_with_invalid_config)
{
    using namespace osrm;

    osrm::CustomizationConfig config;
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_THROW(osrm::customize(config),
                      std::exception); // including osrm::util::exception, etc.
}

BOOST_FIXTURE_TEST_CASE(first_customization_emits_compact_query_dataset, CustomizationFixture)
{
    for (const auto &suffix : {".mldgr", ".cell_metrics", ".datasource_names"})
        std::filesystem::remove(Source.string() + suffix);
    const auto config = getConfig();
    osrm::customize(config);
    BOOST_CHECK(!osrm::storage::isCompactDataset(config.base_path));
    BOOST_CHECK(osrm::storage::isCompactDataset(osrm::storage::StorageConfig(Target).base_path));
    checkGeneration(Target);
    auto original = loadMLD(Source);
    auto compact = loadMLD(Target);
    osrm::RouteParameters route;
    route.coordinates = get_locations_in_big_component();
    route.steps = true;
    route.annotations = true;
    route.annotations_type = osrm::RouteParameters::AnnotationsType::All;
    osrm::json::Object a, b;
    BOOST_CHECK(original.Route(route, a) == compact.Route(route, b));
    CHECK_EQUAL_JSON(a, b);
    for (const auto &coordinate : route.coordinates)
    {
        osrm::NearestParameters nearest;
        nearest.coordinates = {coordinate};
        nearest.number_of_results = 5;
        BOOST_CHECK(original.Nearest(nearest, a) == compact.Nearest(nearest, b));
        CHECK_EQUAL_JSON(a, b);
    }
}

BOOST_FIXTURE_TEST_CASE(split_outputs_and_traffic_recustomization, CustomizationFixture)
{
    auto config = getConfig();
    for (const auto &suffix : {".mldgr", ".cell_metrics"})
        std::filesystem::remove(Source.string() + suffix);
    const auto metrics = Root / "metrics" / "monaco";
    std::filesystem::create_directory(metrics.parent_path());
    config.output_path = metrics;
    osrm::customize(config);
    BOOST_CHECK(!std::filesystem::exists(Source.string() + ".mldgr"));
    checkGeneration(Target, metrics);
    auto first = loadMLD(Target);
    osrm::NearestParameters nearest;
    nearest.coordinates = {get_dummy_location()};
    osrm::json::Object first_result, after_result;
    BOOST_CHECK(first.Nearest(nearest, first_result) == osrm::Status::Ok);

    config.updater_config.segment_speed_lookup_paths = {writeTraffic().string()};
    config.CompactOutputPath = Root / "updated" / "monaco.osrm";
    osrm::customize(config);
    osrm::extractor::SegmentDataContainer updated;
    osrm::extractor::files::readSegmentData(Source.string() + ".geometry", updated);
    bool has_traffic = false;
    for (unsigned geometry = 0; geometry < updated.GetNumberOfGeometries(); ++geometry)
        for (auto datasource : updated.GetForwardDatasources(geometry))
            has_traffic |= datasource != 0;
    BOOST_CHECK(has_traffic);
    checkGeneration(config.CompactOutputPath, metrics);
    std::filesystem::rename(Source.parent_path(), Root / "hidden-source");
    std::filesystem::rename(metrics.parent_path(), Root / "hidden-metrics");
    auto independent = loadMLD(config.CompactOutputPath);
    BOOST_CHECK(independent.Nearest(nearest, after_result) == osrm::Status::Ok);
    BOOST_CHECK(first.Nearest(nearest, after_result) == osrm::Status::Ok);
    CHECK_EQUAL_JSON(first_result, after_result);
}

BOOST_FIXTURE_TEST_CASE(compact_preflight_runs_before_updates, CustomizationFixture)
{
    auto config = getConfig();
    config.updater_config.segment_speed_lookup_paths = {"nonexistent-traffic.csv"};
    std::filesystem::create_directory(Target.parent_path());
    const auto sentinel = Target.parent_path() / "keep-me";
    std::ofstream(sentinel) << "existing generation";
    std::map<std::filesystem::path, std::pair<std::uintmax_t, std::filesystem::file_time_type>>
        before;
    for (const auto &entry : std::filesystem::directory_iterator(Source.parent_path()))
        before.emplace(entry.path(), std::make_pair(entry.file_size(), entry.last_write_time()));
    BOOST_CHECK_EXCEPTION(
        osrm::customize(config),
        osrm::util::exception,
        [](const auto &error)
        {
            return std::string(error.what()).find("Output directory must be new") !=
                   std::string::npos;
        });
    BOOST_CHECK(std::filesystem::exists(sentinel));
    for (const auto &[path, state] : before)
    {
        BOOST_CHECK_EQUAL(std::filesystem::file_size(path), state.first);
        BOOST_CHECK(std::filesystem::last_write_time(path) == state.second);
    }
    config.CompactOutputPath = Root / "missing" / "new" / "monaco.osrm";
    BOOST_CHECK_THROW(osrm::customize(config), osrm::util::exception);
    BOOST_CHECK(!std::filesystem::exists(Root / "missing"));
}

BOOST_FIXTURE_TEST_CASE(failed_customization_does_not_publish_compact_output, CustomizationFixture)
{
    auto config = getConfig();
    config.updater_config.segment_speed_lookup_paths = {"nonexistent-traffic.csv"};
    BOOST_CHECK_THROW(osrm::customize(config), std::exception);
    BOOST_CHECK(!std::filesystem::exists(Target.parent_path()));
}

BOOST_FIXTURE_TEST_CASE(default_customization_keeps_mutable_format, CustomizationFixture)
{
    auto config = getConfig();
    config.CompactOutputPath.clear();
    osrm::customize(config);
    BOOST_CHECK(!osrm::storage::isCompactDataset(config.base_path));
    BOOST_CHECK(!std::filesystem::exists(Target.parent_path()));
    auto raw = loadMLD(Source);
    osrm::NearestParameters nearest;
    nearest.coordinates = {get_dummy_location()};
    osrm::json::Object result;
    BOOST_CHECK(raw.Nearest(nearest, result) == osrm::Status::Ok);
}

BOOST_AUTO_TEST_SUITE_END()
