#include "storage/compact_dataset.hpp"
#include "storage/storage.hpp"
#include "storage/view_factory.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <fstream>
#include <memory>
#include <string_view>
#include <vector>

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

BOOST_AUTO_TEST_CASE(version_metadata_does_not_change_query_layout)
{
    DatasetDirectory directory;
    const auto legacy = directory.path / "legacy.osrm.properties";
    const auto compact = directory.path / "compact.osrm.properties";
    const std::uint64_t query_value = 1234;
    for (const auto &path : {legacy, compact})
    {
        osrm::storage::tar::FileWriter writer(path,
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        writer.WriteElementCount64("/common/query_value", 1);
        writer.WriteFrom("/common/query_value", query_value);
        if (path == compact)
        {
            writer.WriteElementCount64(osrm::storage::COMPACT_VERSION_ENTRY, 1);
            writer.WriteFrom(osrm::storage::COMPACT_VERSION_ENTRY,
                             osrm::storage::COMPACT_DATASET_VERSION);
        }
        writer.Close();
    }
    osrm::storage::ContiguousDataLayout before, after;
    osrm::storage::populateLayoutFromFile(legacy, before);
    osrm::storage::populateLayoutFromFile(compact, after);
    BOOST_CHECK(before.HasBlock("/common/query_value"));
    BOOST_CHECK(after.HasBlock("/common/query_value"));
    BOOST_CHECK(!after.HasBlock(osrm::storage::COMPACT_VERSION_ENTRY));
    BOOST_CHECK_EQUAL(before.GetSizeOfLayout(), after.GetSizeOfLayout());
    BOOST_CHECK(osrm::storage::isCompactDataset(directory.path / "compact"));
}

BOOST_AUTO_TEST_CASE(rejects_invalid_spatial_geometry_metadata)
{
    auto layout = std::make_unique<osrm::storage::ContiguousDataLayout>();
    const auto count_block = "/common/nbn_data/coordinates_spatial/count";
    layout->SetBlock(count_block, {1, sizeof(std::uint64_t)});
    layout->SetBlock("/common/segment_data/index", {0, 0});
    std::vector<char> buffer(layout->GetSizeOfLayout());
    std::vector<osrm::storage::SharedDataIndex::AllocatedRegion> regions;
    regions.push_back({buffer.data(), std::move(layout)});
    const osrm::storage::SharedDataIndex index(std::move(regions));

    const auto reject = [](std::string_view message)
    { return [message](const osrm::util::exception &error) { return error.what() == message; }; };
    auto &count = *index.GetBlockPtr<std::uint64_t>(count_block);
    for (const std::uint64_t invalid_count : {std::uint64_t{0}, std::uint64_t{SPECIAL_NODEID}})
    {
        count = invalid_count;
        BOOST_CHECK_EXCEPTION(
            osrm::storage::make_spatial_coordinate_view(index, "/common/nbn_data/coordinates"),
            osrm::util::exception,
            reject("Invalid packed node coordinate count"));
    }
    BOOST_CHECK_EXCEPTION(osrm::storage::make_segment_data_view(index, "/common/segment_data"),
                          osrm::util::exception,
                          reject("Missing segment geometry index"));
}

BOOST_AUTO_TEST_SUITE_END()
