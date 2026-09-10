#include <boost/test/unit_test.hpp>

#include "../common/temporary_file.hpp"
#include "coordinates.hpp"
#include "fixture.hpp"

#include "storage/tar.hpp"
#include "osrm/isochrone_parameters.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/status.hpp"

#include <filesystem>
#include <limits>
#include <string>
#include <variant>
#include <vector>

namespace
{

osrm::IsochroneParameters validParameters()
{
    osrm::IsochroneParameters parameters;
    parameters.coordinates.push_back(get_dummy_location());
    parameters.contours_seconds = {300.};
    return parameters;
}

std::string code(const osrm::json::Object &result)
{ return std::get<osrm::json::String>(result.values.at("code")).value; }

std::string message(const osrm::json::Object &result)
{ return std::get<osrm::json::String>(result.values.at("message")).value; }

std::filesystem::path makeLegacyCHDataset()
{
    const auto source_base = std::filesystem::path{OSRM_TEST_DATA_DIR "/ch/monaco.osrm"};
    const auto directory =
        std::filesystem::temp_directory_path() / ("osrm-legacy-isochrone-" + random_string(8));
    std::filesystem::create_directory(directory);
    const auto target_base = directory / source_base.filename();

    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(source_base.parent_path()))
        {
            if (!entry.is_regular_file() ||
                !entry.path().filename().string().starts_with(source_base.filename().string() +
                                                              ".") ||
                entry.path().extension() == ".hsgr")
            {
                continue;
            }

            std::filesystem::copy_file(entry.path(),
                                       directory / entry.path().filename(),
                                       std::filesystem::copy_options::overwrite_existing);
        }

        const auto source_hsgr = source_base.string() + ".hsgr";
        const auto target_hsgr = target_base.string() + ".hsgr";
        osrm::storage::tar::FileReader reader{source_hsgr,
                                              osrm::storage::tar::FileReader::VerifyFingerprint};
        osrm::storage::tar::FileWriter writer{target_hsgr,
                                              osrm::storage::tar::FileWriter::HasNoFingerprint};
        std::vector<osrm::storage::tar::FileReader::FileEntry> entries;
        reader.List(std::back_inserter(entries));
        for (const auto &entry : entries)
        {
            if (entry.name.find("/isochrone/") != std::string::npos)
                continue;

            std::vector<char> bytes(entry.size);
            reader.ReadInto(entry.name, bytes.data(), bytes.size());
            writer.WriteFrom(entry.name, bytes.data(), bytes.size());
        }
    }
    catch (...)
    {
        std::filesystem::remove_all(directory);
        throw;
    }

    return target_base;
}

std::filesystem::path makeLegacyMLDDataset()
{
    const auto source_base = std::filesystem::path{OSRM_TEST_DATA_DIR "/mld/monaco.osrm"};
    const auto directory =
        std::filesystem::temp_directory_path() / ("osrm-legacy-isochrone-" + random_string(8));
    std::filesystem::create_directory(directory);
    const auto target_base = directory / source_base.filename();

    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(source_base.parent_path()))
        {
            if (!entry.is_regular_file() ||
                !entry.path().filename().string().starts_with(source_base.filename().string() +
                                                              ".") ||
                entry.path().extension() == ".mldgr")
            {
                continue;
            }

            std::filesystem::copy_file(entry.path(),
                                       directory / entry.path().filename(),
                                       std::filesystem::copy_options::overwrite_existing);
        }

        const auto source_mldgr = source_base.string() + ".mldgr";
        const auto target_mldgr = target_base.string() + ".mldgr";
        osrm::storage::tar::FileReader reader{source_mldgr,
                                              osrm::storage::tar::FileReader::VerifyFingerprint};
        osrm::storage::tar::FileWriter writer{target_mldgr,
                                              osrm::storage::tar::FileWriter::HasNoFingerprint};
        std::vector<osrm::storage::tar::FileReader::FileEntry> entries;
        reader.List(std::back_inserter(entries));
        for (const auto &entry : entries)
        {
            if (entry.name.find("/isochrone/") != std::string::npos)
                continue;

            std::vector<char> bytes(entry.size);
            reader.ReadInto(entry.name, bytes.data(), bytes.size());
            writer.WriteFrom(entry.name, bytes.data(), bytes.size());
        }
    }
    catch (...)
    {
        std::filesystem::remove_all(directory);
        throw;
    }

    return target_base;
}

} // namespace

BOOST_AUTO_TEST_SUITE(isochrone)

BOOST_AUTO_TEST_CASE(json_api_returns_a_geojson_feature_collection)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    auto parameters = validParameters();
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_REQUIRE(status == osrm::Status::Ok);
    BOOST_CHECK_EQUAL(code(result), "Ok");
    BOOST_CHECK_EQUAL(std::get<osrm::json::String>(result.values.at("type")).value,
                      "FeatureCollection");
    const auto &features = std::get<osrm::json::Array>(result.values.at("features")).values;
    BOOST_REQUIRE_EQUAL(features.size(), 1);
    const auto &feature = std::get<osrm::json::Object>(features.front());
    BOOST_CHECK_EQUAL(std::get<osrm::json::String>(feature.values.at("type")).value, "Feature");

    const auto &waypoints = std::get<osrm::json::Array>(result.values.at("waypoints")).values;
    BOOST_REQUIRE_EQUAL(waypoints.size(), 1);
    const auto &waypoint = std::get<osrm::json::Object>(waypoints.front());
    BOOST_CHECK(waypoint.values.contains("name"));
    BOOST_CHECK(waypoint.values.contains("location"));
    BOOST_CHECK(waypoint.values.contains("distance"));
    BOOST_CHECK(waypoint.values.contains("hint"));
}

BOOST_AUTO_TEST_CASE(ch_json_api_returns_a_geojson_feature_collection)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    auto parameters = validParameters();
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_REQUIRE(status == osrm::Status::Ok);
    BOOST_CHECK_EQUAL(code(result), "Ok");
    BOOST_CHECK_EQUAL(std::get<osrm::json::String>(result.values.at("type")).value,
                      "FeatureCollection");
    const auto &features = std::get<osrm::json::Array>(result.values.at("features")).values;
    BOOST_REQUIRE_EQUAL(features.size(), 1);
}

BOOST_AUTO_TEST_CASE(json_api_rejects_invalid_parameters)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    auto parameters = validParameters();
    parameters.contours_seconds.clear();
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Error);
    BOOST_CHECK_EQUAL(code(result), "InvalidOptions");
    BOOST_CHECK_EQUAL(message(result), "Invalid isochrone parameters.");
}

BOOST_AUTO_TEST_CASE(json_api_rejects_an_invalid_direction_enumerator)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    auto parameters = validParameters();
    parameters.direction = static_cast<osrm::IsochroneParameters::Direction>(255);
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Error);
    BOOST_CHECK_EQUAL(code(result), "InvalidOptions");
    BOOST_CHECK_EQUAL(message(result), "Invalid isochrone parameters.");
}

BOOST_AUTO_TEST_CASE(json_api_rejects_a_contour_below_the_duration_precision)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    auto parameters = validParameters();
    parameters.contours_seconds = {0.01};
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Error);
    BOOST_CHECK_EQUAL(code(result), "InvalidValue");
    BOOST_CHECK_EQUAL(message(result),
                      "Contour is below the supported duration precision of 0.1 seconds.");
}

BOOST_AUTO_TEST_CASE(json_api_accepts_a_contour_at_exactly_one_decisecond)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    auto parameters = validParameters();
    parameters.contours_seconds = {0.1};
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Ok);
    BOOST_CHECK_EQUAL(code(result), "Ok");
}

BOOST_AUTO_TEST_CASE(json_api_rejects_a_contour_that_overflows_the_internal_duration)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    auto parameters = validParameters();
    parameters.contours_seconds = {std::numeric_limits<double>::max()};
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Error);
    BOOST_CHECK_EQUAL(code(result), "InvalidValue");
    BOOST_CHECK_EQUAL(message(result),
                      "Contour exceeds the maximum supported duration in seconds.");
}

BOOST_AUTO_TEST_CASE(json_api_enforces_the_configured_contour_limit)
{
    osrm::EngineConfig config;
    config.storage_config = {OSRM_TEST_DATA_DIR "/mld/monaco.osrm"};
    config.use_shared_memory = false;
    config.algorithm = osrm::EngineConfig::Algorithm::MLD;
    config.max_isochrone_contours = 1;
    osrm::OSRM routing_machine{config};

    auto parameters = validParameters();
    parameters.contours_seconds = {300., 600.};
    osrm::json::Object result;

    const auto status = routing_machine.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Error);
    BOOST_CHECK_EQUAL(code(result), "TooBig");
    BOOST_CHECK_EQUAL(message(result),
                      "Number of contours_seconds is higher than current maximum (1)");
}

BOOST_AUTO_TEST_CASE(generic_api_rejects_non_json_result_types)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    const auto parameters = validParameters();

    osrm::engine::api::ResultT string_result = std::string();
    const auto string_status = osrm.Isochrone(parameters, string_result);
    BOOST_CHECK(string_status == osrm::Status::Error);
    BOOST_REQUIRE(std::holds_alternative<std::string>(string_result));
    BOOST_CHECK_EQUAL(
        std::get<std::string>(string_result),
        "code=NotImplemented message=The isochrone service only supports JSON/GeoJSON "
        "output.");

    osrm::engine::api::ResultT flatbuffers_result = flatbuffers::FlatBufferBuilder();
    const auto flatbuffers_status = osrm.Isochrone(parameters, flatbuffers_result);
    BOOST_CHECK(flatbuffers_status == osrm::Status::Error);
    BOOST_REQUIRE(std::holds_alternative<flatbuffers::FlatBufferBuilder>(flatbuffers_result));
    BOOST_CHECK_GT(std::get<flatbuffers::FlatBufferBuilder>(flatbuffers_result).GetSize(), 0U);
}

BOOST_AUTO_TEST_CASE(json_api_rejects_a_non_json_parameter_format)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/mld/monaco.osrm", osrm::EngineConfig::Algorithm::MLD);
    auto parameters = validParameters();
    parameters.format = osrm::engine::api::BaseParameters::OutputFormatType::FLATBUFFERS;
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Error);
    BOOST_CHECK_EQUAL(code(result), "NotImplemented");
    BOOST_CHECK_EQUAL(message(result), "The isochrone service only supports JSON/GeoJSON output.");
}

BOOST_AUTO_TEST_CASE(ch_without_an_isochrone_graph_reports_unavailable)
{
    const auto legacy_base = makeLegacyCHDataset();
    auto osrm = getOSRM(legacy_base.string());
    const auto parameters = validParameters();
    osrm::json::Object result;

    const auto status = osrm.Isochrone(parameters, result);

    BOOST_CHECK(status == osrm::Status::Error);
    BOOST_CHECK_EQUAL(code(result), "NotImplemented");
    BOOST_CHECK_EQUAL(message(result), "Isochrone search is not available for this dataset.");

    std::filesystem::remove_all(legacy_base.parent_path());
}

BOOST_AUTO_TEST_CASE(mld_without_an_isochrone_graph_preserves_route_and_reports_unavailable)
{
    const auto legacy_base = makeLegacyMLDDataset();
    try
    {
        auto osrm = getOSRM(legacy_base.string(), osrm::EngineConfig::Algorithm::MLD);

        osrm::RouteParameters route_parameters;
        route_parameters.coordinates = {get_dummy_location(), get_dummy_location()};
        osrm::json::Object route_result;
        BOOST_CHECK(osrm.Route(route_parameters, route_result) == osrm::Status::Ok);
        BOOST_CHECK_EQUAL(code(route_result), "Ok");

        const auto parameters = validParameters();
        osrm::json::Object isochrone_result;
        const auto isochrone_status = osrm.Isochrone(parameters, isochrone_result);

        BOOST_CHECK(isochrone_status == osrm::Status::Error);
        BOOST_CHECK_EQUAL(code(isochrone_result), "NotImplemented");
        BOOST_CHECK_EQUAL(message(isochrone_result),
                          "Isochrone search is not available for this dataset.");
    }
    catch (...)
    {
        std::filesystem::remove_all(legacy_base.parent_path());
        throw;
    }
    std::filesystem::remove_all(legacy_base.parent_path());
}

BOOST_AUTO_TEST_SUITE_END()
