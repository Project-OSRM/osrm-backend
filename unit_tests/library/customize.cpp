#include "../common/temporary_file.hpp"
#include "coordinates.hpp"
#include "equal_json.hpp"
#include "extractor/files.hpp"
#include "guidance/compact_turn_data.hpp"
#include "storage/view_factory.hpp"
#include "engine/datafacade/mmap_memory_allocator.hpp"
#include "osrm/customizer.hpp"
#include "osrm/customizer_config.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <thread>

namespace
{
struct CustomizationFixture
{
    std::filesystem::path Root = std::filesystem::temp_directory_path() / random_string(16);
    std::filesystem::path Source = Root / "monaco.osrm";
    CustomizationFixture()
    {
        std::filesystem::create_directory(Root);
        for (const auto &entry : std::filesystem::directory_iterator(OSRM_TEST_DATA_DIR "/mld"))
            if (entry.path().filename().string().starts_with("monaco.osrm."))
                std::filesystem::copy_file(entry.path(), Root / entry.path().filename());
    }
    ~CustomizationFixture() { std::filesystem::remove_all(Root); }
    osrm::CustomizationConfig getConfig() const
    {
        osrm::CustomizationConfig config;
        config.UseDefaultOutputNames(Source);
        config.requested_num_threads = 2;
        return config;
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
osrm::OSRM loadMLD(const std::filesystem::path &path, bool mmap)
{
    osrm::EngineConfig config;
    config.storage_config = {path};
    config.algorithm = osrm::EngineConfig::Algorithm::MLD;
    config.use_shared_memory = false;
    config.use_mmap = mmap;
    return osrm::OSRM(config);
}
} // namespace
BOOST_AUTO_TEST_SUITE(library_customize)
BOOST_AUTO_TEST_CASE(invalid_config)
{
    osrm::CustomizationConfig config;
    config.requested_num_threads = 2;
    BOOST_CHECK_THROW(osrm::customize(config), std::exception);
}
BOOST_FIXTURE_TEST_CASE(single_compact_representation, CustomizationFixture)
{
    auto config = getConfig();
    osrm::customize(config);
    BOOST_CHECK(!std::filesystem::exists(Source.string() + ".query"));
    {
        osrm::storage::tar::FileReader graph(Source.string() + ".mldgr",
                                             osrm::storage::tar::FileReader::VerifyFingerprint);
        BOOST_CHECK(graph.HasEntry("/mld/multilevelgraph/edge_array6"));
        BOOST_CHECK(!graph.HasEntry("/mld/multilevelgraph/edge_array"));
        BOOST_CHECK(graph.HasEntry("/common/turn_data/turn_instructions"));
        osrm::storage::tar::FileReader metrics(Source.string() + ".cell_metrics",
                                               osrm::storage::tar::FileReader::VerifyFingerprint);
        BOOST_CHECK(metrics.HasEntry("/mld/metrics/routability/shared/weights"));
        BOOST_CHECK(!metrics.HasEntry("/mld/metrics/routability/exclude/0/weights"));
    }
    osrm::NearestParameters nearest;
    nearest.coordinates = {get_dummy_location()};
    osrm::RouteParameters route;
    route.coordinates = get_locations_in_big_component();
    route.steps = true;
    for (bool mmap : {false, true})
    {
        osrm::json::Object a, b, route_a, route_b;
        {
            auto before = loadMLD(Source, mmap);
            BOOST_CHECK(before.Nearest(nearest, a) == osrm::Status::Ok);
            BOOST_CHECK(before.Route(route, route_a) == osrm::Status::Ok);
        }
        osrm::customize(config);
        auto after = loadMLD(Source, mmap);
        BOOST_CHECK(after.Nearest(nearest, b) == osrm::Status::Ok);
        CHECK_EQUAL_JSON(a, b);
        BOOST_CHECK(after.Route(route, route_b) == osrm::Status::Ok);
        CHECK_EQUAL_JSON(route_a, route_b);
    }
}
BOOST_FIXTURE_TEST_CASE(compact_geometry_views_round_trip, CustomizationFixture)
{
    osrm::engine::datafacade::MMapMemoryAllocator allocator(osrm::storage::StorageConfig{Source});
    const auto view =
        osrm::storage::make_segment_data_view(allocator.GetIndex(), "/common/segment_data");
    const auto output = Root / "geometry-copy";
    osrm::extractor::files::writeSegmentData(output, view);
    osrm::extractor::SegmentDataContainer restored;
    osrm::extractor::files::readSegmentData(output, restored);
    const auto &copy = std::as_const(restored);
    BOOST_REQUIRE_EQUAL(view.GetNumberOfGeometries(), copy.GetNumberOfGeometries());
    const auto equal = [](const auto &left, const auto &right)
    { BOOST_CHECK(std::ranges::equal(left, right)); };
    for (unsigned g = 0; g < view.GetNumberOfGeometries(); ++g)
    {
        equal(view.GetForwardGeometry(g), copy.GetForwardGeometry(g));
        equal(view.GetForwardWeights(g), copy.GetForwardWeights(g));
        equal(view.GetReverseWeights(g), copy.GetReverseWeights(g));
        equal(view.GetForwardDurations(g), copy.GetForwardDurations(g));
        equal(view.GetReverseDurations(g), copy.GetReverseDurations(g));
        equal(view.GetForwardDatasources(g), copy.GetForwardDatasources(g));
        equal(view.GetReverseDatasources(g), copy.GetReverseDatasources(g));
    }
}

BOOST_FIXTURE_TEST_CASE(repeated_traffic_keeps_original_turn_identities, CustomizationFixture)
{
    auto config = getConfig();
    const auto identities =
        osrm::guidance::readOriginalTurnData(Source.string() + ".edges").Mapping;
    config.updater_config.segment_speed_lookup_paths = {writeTraffic().string()};
    osrm::customize(config);
    osrm::extractor::SegmentDataContainer segments;
    osrm::extractor::files::readSegmentData(Source.string() + ".geometry", segments);
    bool updated = false;
    for (unsigned g = 0; g < segments.GetNumberOfGeometries(); ++g)
        for (auto value : segments.GetForwardDatasources(g))
            updated |= value != 0;
    BOOST_CHECK(updated);
    osrm::customize(config);
    BOOST_CHECK(osrm::guidance::readOriginalTurnData(Source.string() + ".edges").Mapping ==
                identities);
    auto engine = loadMLD(Source, true);
    osrm::NearestParameters nearest;
    nearest.coordinates = {get_dummy_location()};
    osrm::json::Object result;
    BOOST_CHECK(engine.Nearest(nearest, result) == osrm::Status::Ok);
}
BOOST_FIXTURE_TEST_CASE(redirected_output_uses_the_same_compact_format, CustomizationFixture)
{
    auto config = getConfig();
    config.output_path = Root / "other";
    osrm::customize(config);
    osrm::storage::tar::FileReader graph(config.output_path.string() + ".osrm.mldgr",
                                         osrm::storage::tar::FileReader::VerifyFingerprint);
    BOOST_CHECK(graph.HasEntry("/mld/multilevelgraph/edge_array6"));
}
BOOST_AUTO_TEST_SUITE_END()
