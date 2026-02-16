#include "storage/io_config.hpp"

#include <boost/test/unit_test.hpp>

#include <set>
#include <sstream>

BOOST_AUTO_TEST_SUITE(io_config)

using namespace osrm;
using namespace osrm::storage;

// Test helper class to access IOConfig functionality
struct TestConfig : IOConfig
{
    TestConfig(std::vector<std::filesystem::path> required,
               std::vector<std::filesystem::path> optional)
        : IOConfig(std::move(required), std::move(optional), {})
    {
    }
};

BOOST_AUTO_TEST_CASE(list_input_files_format)
{
    TestConfig config({".osrm.ebg", ".osrm.edges"}, {".osrm.hsgr", ".osrm.cells"});

    std::ostringstream output;
    config.ListInputFiles(output);

    std::string result = output.str();

    // Check that required files are listed with "required" prefix
    BOOST_CHECK(result.find("required .osrm.ebg") != std::string::npos);
    BOOST_CHECK(result.find("required .osrm.edges") != std::string::npos);

    // Check that optional files are listed with "optional" prefix
    BOOST_CHECK(result.find("optional .osrm.hsgr") != std::string::npos);
    BOOST_CHECK(result.find("optional .osrm.cells") != std::string::npos);

    // Check that each line ends with newline
    BOOST_CHECK(result.find("required .osrm.ebg\n") != std::string::npos);
    BOOST_CHECK(result.find("optional .osrm.hsgr\n") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(list_input_files_empty_string_skipped)
{
    // ExtractorConfig has an empty string as the first required input
    // (representing the OSM input file)
    TestConfig config({"", ".osrm.ebg"}, {".osrm.hsgr"});

    std::ostringstream output;
    config.ListInputFiles(output);

    std::string result = output.str();

    // Empty string should not appear in output
    BOOST_CHECK(result.find("required \n") == std::string::npos);

    // Other entries should still be present
    BOOST_CHECK(result.find("required .osrm.ebg") != std::string::npos);
    BOOST_CHECK(result.find("optional .osrm.hsgr") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(list_input_files_empty_lists)
{
    TestConfig config({}, {});

    std::ostringstream output;
    config.ListInputFiles(output);

    std::string result = output.str();

    // Output should be empty when there are no input files
    BOOST_CHECK(result.empty());
}

BOOST_AUTO_TEST_CASE(list_input_files_required_only)
{
    TestConfig config({".osrm.ebg", ".osrm.edges"}, {});

    std::ostringstream output;
    config.ListInputFiles(output);

    std::string result = output.str();

    BOOST_CHECK(result.find("required .osrm.ebg") != std::string::npos);
    BOOST_CHECK(result.find("required .osrm.edges") != std::string::npos);
    BOOST_CHECK(result.find("optional") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(list_input_files_optional_only)
{
    TestConfig config({}, {".osrm.hsgr", ".osrm.cells"});

    std::ostringstream output;
    config.ListInputFiles(output);

    std::string result = output.str();

    BOOST_CHECK(result.find("required") == std::string::npos);
    BOOST_CHECK(result.find("optional .osrm.hsgr") != std::string::npos);
    BOOST_CHECK(result.find("optional .osrm.cells") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(list_input_files_deduplication)
{
    TestConfig config1({".osrm.ebg", ".osrm.ebg_nodes", ".osrm.properties"}, {});
    TestConfig config2({".osrm.ebg", ".osrm.turn_weight_penalties", ".osrm.ebg_nodes"}, {});

    std::ostringstream output;
    std::set<std::string> seen;
    config1.ListInputFiles(output, seen);
    config2.ListInputFiles(output, seen);

    std::string result = output.str();

    // Each file should appear exactly once
    auto count = [&](const std::string &needle)
    {
        size_t n = 0;
        size_t pos = 0;
        while ((pos = result.find(needle, pos)) != std::string::npos)
        {
            ++n;
            pos += needle.size();
        }
        return n;
    };

    BOOST_CHECK_EQUAL(count("required .osrm.ebg\n"), 1u);
    BOOST_CHECK_EQUAL(count("required .osrm.ebg_nodes\n"), 1u);
    BOOST_CHECK_EQUAL(count("required .osrm.properties\n"), 1u);
    BOOST_CHECK_EQUAL(count("required .osrm.turn_weight_penalties\n"), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
