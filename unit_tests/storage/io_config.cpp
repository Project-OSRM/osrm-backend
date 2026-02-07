#include "storage/io_config.hpp"

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE_END()
