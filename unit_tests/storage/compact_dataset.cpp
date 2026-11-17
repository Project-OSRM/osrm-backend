#include "storage/compact_dataset.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <fstream>

namespace
{
struct DatasetDirectory
{
    std::filesystem::path path = std::filesystem::temp_directory_path() / random_string(16);
    DatasetDirectory() { std::filesystem::create_directory(path); }
    ~DatasetDirectory() { std::filesystem::remove_all(path); }
};

void writeVersion(const std::filesystem::path &base, std::uint64_t version)
{
    osrm::storage::tar::FileWriter writer(base.string() + ".osrm.properties",
                                          osrm::storage::tar::FileWriter::GenerateFingerprint);
    writer.WriteElementCount64(osrm::storage::COMPACT_VERSION_ENTRY, 1);
    writer.WriteFrom(osrm::storage::COMPACT_VERSION_ENTRY, version);
    writer.Close();
}
} // namespace

BOOST_AUTO_TEST_SUITE(compact_dataset)

BOOST_AUTO_TEST_CASE(legacy_and_missing_properties)
{
    DatasetDirectory directory;
    const auto base = directory.path / "data";
    BOOST_CHECK(!osrm::storage::isCompactDataset(base));
    {
        osrm::storage::tar::FileWriter writer(base.string() + ".osrm.properties",
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        writer.Close();
        writer.Close();
    }
    BOOST_CHECK(!osrm::storage::isCompactDataset(base));
    BOOST_CHECK_NO_THROW(osrm::storage::requireMutableDataset(base));
}

BOOST_AUTO_TEST_CASE(known_version_is_query_only)
{
    DatasetDirectory directory;
    const auto base = directory.path / "data";
    writeVersion(base, osrm::storage::COMPACT_DATASET_VERSION);
    BOOST_CHECK(osrm::storage::isCompactDataset(base));
    BOOST_CHECK_THROW(osrm::storage::requireMutableDataset(base), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(unknown_version_is_rejected)
{
    DatasetDirectory directory;
    const auto base = directory.path / "data";
    writeVersion(base, osrm::storage::COMPACT_DATASET_VERSION + 1);
    BOOST_CHECK_THROW(osrm::storage::isCompactDataset(base), osrm::util::exception);
    BOOST_CHECK_THROW(osrm::storage::requireMutableDataset(base), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(interrupted_conversion_is_rejected)
{
    DatasetDirectory directory;
    const auto base = directory.path / "data";
    std::ofstream marker(directory.path / osrm::storage::COMPACT_INCOMPLETE_FILE);
    marker.close();
    BOOST_CHECK_THROW(osrm::storage::isCompactDataset(base), osrm::util::exception);
    BOOST_CHECK_THROW(osrm::storage::requireMutableDataset(base), osrm::util::exception);
}

BOOST_AUTO_TEST_CASE(malformed_version_metadata_is_rejected)
{
    DatasetDirectory directory;
    const auto base = directory.path / "data";
    {
        osrm::storage::tar::FileWriter writer(base.string() + ".osrm.properties",
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        writer.WriteFrom(osrm::storage::COMPACT_VERSION_ENTRY,
                         osrm::storage::COMPACT_DATASET_VERSION);
        writer.Close();
    }
    BOOST_CHECK_THROW(osrm::storage::isCompactDataset(base), osrm::util::exception);
}

BOOST_AUTO_TEST_SUITE_END()
