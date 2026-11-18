#include "storage/compact_dataset.hpp"
#include "storage/storage.hpp"

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

BOOST_AUTO_TEST_CASE(query_archive_requires_matching_source_generations)
{
    DatasetDirectory directory;
    const auto base = directory.path / "data";
    const auto write_generation = [&](const std::string &suffix,
                                      const std::string &entry,
                                      std::uint64_t generation,
                                      std::uint32_t checksum = 42)
    {
        osrm::storage::tar::FileWriter writer(base.string() + suffix,
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        writer.WriteElementCount64(entry, 1);
        writer.WriteFrom(entry, generation);
        const auto checksum_entry =
            suffix == ".osrm.mldgr" ? "/mld/connectivity_checksum" : "/ch/connectivity_checksum";
        writer.WriteElementCount64(checksum_entry, 1);
        writer.WriteFrom(checksum_entry, checksum);
        writer.Close();
    };
    const auto write_query = [&](std::uint64_t mld_generation, std::uint64_t ch_generation)
    {
        osrm::storage::tar::FileWriter writer(base.string() + ".osrm.query",
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        for (const auto &[entry, value] :
             {std::pair{osrm::storage::COMPACT_VERSION_ENTRY,
                        osrm::storage::COMPACT_DATASET_VERSION},
              std::pair{osrm::storage::MLD_GENERATION_ENTRY, mld_generation},
              std::pair{osrm::storage::CH_GENERATION_ENTRY, ch_generation}})
        {
            writer.WriteElementCount64(entry, 1);
            writer.WriteFrom(entry, value);
        }
        for (const auto *entry : {"/mld/connectivity_checksum", "/ch/connectivity_checksum"})
        {
            writer.WriteElementCount64(entry, 1);
            writer.WriteFrom(entry, std::uint32_t{42});
        }
        writer.Close();
    };
    BOOST_CHECK(!osrm::storage::hasQueryData(base));
    write_query(1, 0);
    BOOST_CHECK(!osrm::storage::hasQueryData(base));
    write_generation(".osrm.mldgr", osrm::storage::MLD_GENERATION_ENTRY, 1);
    BOOST_CHECK(osrm::storage::hasQueryData(base));
    write_generation(".osrm.mldgr", osrm::storage::MLD_GENERATION_ENTRY, 2);
    BOOST_CHECK(!osrm::storage::hasQueryData(base));
    write_query(2, 0);
    BOOST_CHECK(osrm::storage::hasQueryData(base));
    write_generation(".osrm.hsgr", osrm::storage::CH_GENERATION_ENTRY, 1);
    BOOST_CHECK(!osrm::storage::hasQueryData(base));
    write_query(2, 1);
    BOOST_CHECK(osrm::storage::hasQueryData(base));
    write_generation(".osrm.hsgr", osrm::storage::CH_GENERATION_ENTRY, 2);
    BOOST_CHECK(!osrm::storage::hasQueryData(base));
    write_generation(".osrm.hsgr", osrm::storage::CH_GENERATION_ENTRY, 1, 43);
    BOOST_CHECK(!osrm::storage::hasQueryData(base));
    write_generation(".osrm.hsgr", osrm::storage::CH_GENERATION_ENTRY, 1);
    BOOST_CHECK(osrm::storage::hasQueryData(base));
    osrm::storage::invalidateQueryData(base);
    BOOST_CHECK(!std::filesystem::exists(base.string() + ".osrm.query"));
    BOOST_CHECK(std::filesystem::exists(base.string() + ".osrm.mldgr"));
    BOOST_CHECK(std::filesystem::exists(base.string() + ".osrm.hsgr"));
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

BOOST_AUTO_TEST_SUITE_END()
