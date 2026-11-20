#include "extractor/serialization.hpp"
#include "storage/storage.hpp"
#include "storage/view_factory.hpp"
#include "util/mmap_file.hpp"

#include "../common/temporary_file.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace
{
const std::string prefix = "/common/intersection_bearings";

void checkPackedIDs(const std::filesystem::path &path)
{
    osrm::storage::tar::FileReader reader(path, osrm::storage::tar::FileReader::VerifyFingerprint);
    BOOST_CHECK(!reader.HasEntry(prefix + "/node_to_class_id"));
    std::vector<std::uint16_t> ids;
    osrm::storage::serialization::read(reader, prefix + "/node_to_class_id16", ids);
    const std::vector<std::uint16_t> expected{0, std::numeric_limits<std::uint16_t>::max()};
    BOOST_CHECK_EQUAL_COLLECTIONS(ids.begin(), ids.end(), expected.begin(), expected.end());
}
} // namespace

BOOST_AUTO_TEST_SUITE(intersection_bearings_archive)

BOOST_AUTO_TEST_CASE(round_trip_packed_ids)
{
    TemporaryFile source;
    osrm::util::guidance::BearingClass bearing;
    bearing.add(45);
    const osrm::extractor::IntersectionBearingsContainer original{
        {0, std::numeric_limits<BearingClassID>::max()}, {bearing}};
    {
        osrm::storage::tar::FileWriter writer(source.path,
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        osrm::extractor::serialization::write(writer, prefix, original);
    }
    checkPackedIDs(source.path);

    osrm::extractor::IntersectionBearingsContainer restored;
    {
        osrm::storage::tar::FileReader reader(source.path,
                                              osrm::storage::tar::FileReader::VerifyFingerprint);
        osrm::extractor::serialization::read(reader, prefix, restored);
    }
    BOOST_CHECK(restored.GetBearingClass(0) == bearing);
    TemporaryFile copy;
    {
        osrm::storage::tar::FileWriter writer(copy.path,
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        osrm::extractor::serialization::write(writer, prefix, restored);
    }
    checkPackedIDs(copy.path);

    auto layout = std::make_unique<osrm::storage::TarDataLayout>();
    osrm::storage::populateLayoutFromFile(source.path, *layout);
    boost::iostreams::mapped_file_source mapped;
    const auto bytes = osrm::util::mmapFile<char>(source.path, mapped);
    std::vector<osrm::storage::SharedDataIndex::AllocatedRegion> regions;
    regions.push_back({const_cast<char *>(bytes.data()), std::move(layout)});
    const osrm::storage::SharedDataIndex index{std::move(regions)};
    const auto view = osrm::storage::make_intersection_bearings_view(index, prefix);
    BOOST_CHECK(view.GetBearingClass(0) == bearing);
    {
        osrm::storage::tar::FileWriter writer(copy.path,
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        osrm::extractor::serialization::write(writer, prefix, view);
    }
    checkPackedIDs(copy.path);
}

BOOST_AUTO_TEST_CASE(round_trip_wide_ids)
{
    TemporaryFile file;
    const BearingClassID first_wide_id = std::numeric_limits<std::uint16_t>::max();
    std::vector<osrm::util::guidance::BearingClass> classes(first_wide_id + 1);
    classes.back().add(90);
    const osrm::extractor::IntersectionBearingsContainer original{{first_wide_id}, classes};
    {
        osrm::storage::tar::FileWriter writer(file.path,
                                              osrm::storage::tar::FileWriter::GenerateFingerprint);
        osrm::extractor::serialization::write(writer, prefix, original);
    }
    osrm::storage::tar::FileReader reader(file.path,
                                          osrm::storage::tar::FileReader::VerifyFingerprint);
    BOOST_CHECK(!reader.HasEntry(prefix + "/node_to_class_id16"));
    osrm::extractor::IntersectionBearingsContainer restored;
    osrm::extractor::serialization::read(reader, prefix, restored);
    BOOST_CHECK(restored.GetBearingClass(0) == classes.back());
}

BOOST_AUTO_TEST_SUITE_END()
