#include <boost/test/unit_test.hpp>

#include "osrm/extractor.hpp"
#include "osrm/extractor_config.hpp"

#include <thread>

BOOST_AUTO_TEST_SUITE(library_extract)

BOOST_AUTO_TEST_CASE(test_extract_with_invalid_config)
{
    osrm::ExtractorConfig config;
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_THROW(osrm::extract(config),
                      std::exception); // including osrm::util::exception, osmium::io_error, etc.
}

BOOST_AUTO_TEST_CASE(test_extract_with_valid_config)
{
    osrm::ExtractorConfig config;
    config.input_path = OSRM_TEST_DATA_DIR "/monaco.osm.pbf";
    config.UseDefaultOutputNames(OSRM_TEST_DATA_DIR "/monaco.osm.pbf");
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_NO_THROW(osrm::extract(config));
}

BOOST_AUTO_TEST_SUITE_END()
