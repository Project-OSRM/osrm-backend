#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "equal_json.hpp"
#include "fixture.hpp"

#include "storage/tar.hpp"
#include "osrm/exception.hpp"
#include "osrm/osrm.hpp"

#include "../common/temporary_file.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
struct DirectLoadFixture
{
    const std::filesystem::path Root = std::filesystem::temp_directory_path() / random_string(16);
    const std::filesystem::path Base = Root / "monaco.osrm";

    DirectLoadFixture()
    {
        std::filesystem::create_directory(Root);
        for (const auto &file : std::filesystem::directory_iterator(OSRM_TEST_DATA_DIR "/ch"))
        {
            if (file.path().filename().string().starts_with("monaco.osrm."))
                std::filesystem::copy_file(file.path(), Root / file.path().filename());
        }
    }

    ~DirectLoadFixture()
    {
        std::error_code error;
        std::filesystem::remove_all(Root, error);
    }

    osrm::OSRM loadDataset() const
    {
        osrm::EngineConfig config;
        config.use_shared_memory = false;
        config.use_mmap = false;
        config.storage_config = {Base};
        config.algorithm = osrm::EngineConfig::Algorithm::CH;
        return osrm::OSRM{config};
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(options)

BOOST_AUTO_TEST_CASE(test_ch)
{
    using namespace osrm;
    EngineConfig config;
    config.use_shared_memory = false;
    config.storage_config = storage::StorageConfig(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    config.algorithm = EngineConfig::Algorithm::CH;
    OSRM osrm{config};
}

BOOST_AUTO_TEST_CASE(test_mld)
{
    using namespace osrm;
    EngineConfig config;
    config.use_shared_memory = false;
    config.storage_config = storage::StorageConfig(OSRM_TEST_DATA_DIR "/mld/monaco.osrm");
    config.algorithm = EngineConfig::Algorithm::MLD;
    OSRM osrm{config};
}

BOOST_FIXTURE_TEST_CASE(rejects_malformed_guidance_checksum, DirectLoadFixture)
{
    {
        osrm::storage::tar::FileWriter writer(Base.string() + ".edges",
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        const std::string entry = "/common/connectivity_checksum";
        writer.WriteElementCount64(entry, 1);
        writer.WriteFrom(entry, std::uint8_t{0});
    }

    BOOST_CHECK_EXCEPTION(
        loadDataset(),
        osrm::exception,
        [](const auto &error)
        {
            return std::string_view(error.what()).starts_with("Invalid connectivity checksum in ");
        });
}

BOOST_FIXTURE_TEST_CASE(rejects_graph_for_different_guidance, DirectLoadFixture)
{
    std::uint32_t checksum = 0;
    {
        osrm::storage::tar::FileReader reader(Base.string() + ".edges",
                                              osrm::storage::tar::FileReader::VerifyFingerprint);
        reader.ReadInto("/common/connectivity_checksum", checksum);
    }
    {
        osrm::storage::tar::FileWriter writer(Base.string() + ".hsgr",
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        const std::string entry = "/ch/connectivity_checksum";
        writer.WriteElementCount64(entry, 1);
        writer.WriteFrom(entry, checksum ^ std::uint32_t{1});
    }

    BOOST_CHECK_EXCEPTION(
        loadDataset(),
        osrm::exception,
        [](const auto &error)
        {
            return std::string_view(error.what()).find("does not equal to checksum") !=
                   std::string_view::npos;
        });
}

BOOST_AUTO_TEST_SUITE_END()
