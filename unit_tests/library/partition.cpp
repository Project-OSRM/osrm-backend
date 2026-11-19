#include <boost/test/unit_test.hpp>

#include "osrm/partitioner.hpp"
#include "osrm/partitioner_config.hpp"

#include "../common/temporary_file.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>

#include <thread>

BOOST_AUTO_TEST_SUITE(library_partition)

BOOST_AUTO_TEST_CASE(test_partition_with_invalid_config)
{
    using namespace osrm;

    osrm::PartitionerConfig config;
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_THROW(osrm::partition(config),
                      std::exception); // including osrm::util::exception, etc.
}

BOOST_AUTO_TEST_CASE(redirected_partition_preserves_source_dataset)
{
    const auto root = std::filesystem::temp_directory_path() / random_string(16);
    const auto source = root / "source";
    const auto destination = root / "output" / "monaco";
    std::filesystem::create_directories(source);
    std::filesystem::create_directory(destination.parent_path());
    const auto read = [](const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::vector<char>(std::istreambuf_iterator<char>(stream), {});
    };
    std::map<std::filesystem::path, std::vector<char>> original;
    for (const auto &entry : std::filesystem::directory_iterator(OSRM_TEST_DATA_DIR "/mld"))
    {
        if (!entry.path().filename().string().starts_with("monaco.osrm."))
            continue;
        const auto copy = source / entry.path().filename();
        std::filesystem::copy_file(entry.path(), copy);
        original.emplace(copy, read(copy));
    }
    for (const auto *suffix : {".mldgr", ".cell_metrics", ".hsgr"})
        std::ofstream(destination.string() + ".osrm" + suffix) << "stale";
    osrm::PartitionerConfig config;
    config.UseDefaultOutputNames(source / "monaco.osrm");
    config.output_path = destination;
    config.requested_num_threads = 2;
    BOOST_CHECK_NO_THROW(osrm::partition(config));
    for (const auto &[path, bytes] : original)
        BOOST_CHECK(read(path) == bytes);
    for (const auto *suffix : {".mldgr", ".cell_metrics", ".hsgr"})
        BOOST_CHECK(!std::filesystem::exists(destination.string() + ".osrm" + suffix));
    BOOST_CHECK(std::filesystem::exists(destination.string() + ".osrm.partition"));
    BOOST_CHECK(std::filesystem::exists(destination.string() + ".osrm.cells"));
    std::filesystem::remove_all(root);
}

BOOST_AUTO_TEST_SUITE_END()
