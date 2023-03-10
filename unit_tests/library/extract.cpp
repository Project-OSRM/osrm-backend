#include <boost/test/unit_test.hpp>

#include "osrm/exception.hpp"
#include "osrm/extractor.hpp"
#include "osrm/extractor_config.hpp"

#include <boost/algorithm/string.hpp>
#include <thread>

// utility class to redirect stderr so we can test it
// inspired by https://stackoverflow.com/questions/5405016
class redirect_stderr
{
    // constructor: accept a pointer to a buffer where stderr will be redirected
  public:
    redirect_stderr(std::streambuf *buf)
        // store the original buffer for later (original buffer returned by rdbuf)
        : old(std::cerr.rdbuf(buf))
    {
    }

    // destructor: restore the original cerr, regardless of how this class gets destroyed
    ~redirect_stderr() { std::cerr.rdbuf(old); }

    // place to store the buffer
  private:
    std::streambuf *old;
};

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
    config.profile_path = OSRM_TEST_DATA_DIR "/../../profiles/car.lua";
    config.small_component_size = 1000;
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_NO_THROW(osrm::extract(config));
}

BOOST_AUTO_TEST_CASE(test_setup_runtime_error)
{
    osrm::ExtractorConfig config;
    config.input_path = OSRM_TEST_DATA_DIR "/monaco.osm.pbf";
    config.UseDefaultOutputNames(OSRM_TEST_DATA_DIR "/monaco.osm.pbf");
    config.profile_path = OSRM_TEST_DATA_DIR "/profiles/bad_setup.lua";
    config.small_component_size = 1000;
    config.requested_num_threads = std::thread::hardware_concurrency();

    std::stringstream output;

    {
        redirect_stderr redir(output.rdbuf());
        BOOST_CHECK_THROW(osrm::extract(config), osrm::util::exception);
    }

    // We just look for the line number, file name, and error message. This avoids portability
    // issues since the output contains the full path to the file, which may change between systems
    BOOST_CHECK(boost::algorithm::contains(output.str(),
                                           "bad_setup.lua:6: attempt to compare number with nil"));
}

BOOST_AUTO_TEST_CASE(test_way_runtime_error)
{
    osrm::ExtractorConfig config;
    config.input_path = OSRM_TEST_DATA_DIR "/monaco.osm.pbf";
    config.UseDefaultOutputNames(OSRM_TEST_DATA_DIR "/monaco.osm.pbf");
    config.profile_path = OSRM_TEST_DATA_DIR "/profiles/bad_way.lua";
    config.small_component_size = 1000;
    config.requested_num_threads = std::thread::hardware_concurrency();

    std::stringstream output;

    {
        redirect_stderr redir(output.rdbuf());
        BOOST_CHECK_THROW(osrm::extract(config), osrm::util::exception);
    }

    // We just look for the line number, file name, and error message. This avoids portability
    // issues since the output contains the full path to the file, which may change between systems
    BOOST_CHECK(boost::algorithm::contains(output.str(),
                                           "bad_way.lua:41: attempt to compare number with nil"));
}

BOOST_AUTO_TEST_CASE(test_node_runtime_error)
{
    osrm::ExtractorConfig config;
    config.input_path = OSRM_TEST_DATA_DIR "/monaco.osm.pbf";
    config.UseDefaultOutputNames(OSRM_TEST_DATA_DIR "/monaco.osm.pbf");
    config.profile_path = OSRM_TEST_DATA_DIR "/profiles/bad_node.lua";
    config.small_component_size = 1000;
    config.requested_num_threads = std::thread::hardware_concurrency();

    std::stringstream output;

    {
        redirect_stderr redir(output.rdbuf());
        BOOST_CHECK_THROW(osrm::extract(config), osrm::util::exception);
    }

    // We just look for the line number, file name, and error message. This avoids portability
    // issues since the output contains the full path to the file, which may change between systems
    BOOST_CHECK(boost::algorithm::contains(output.str(),
                                           "bad_node.lua:36: attempt to compare number with nil"));
}

BOOST_AUTO_TEST_CASE(test_segment_runtime_error)
{
    osrm::ExtractorConfig config;
    config.input_path = OSRM_TEST_DATA_DIR "/monaco.osm.pbf";
    config.UseDefaultOutputNames(OSRM_TEST_DATA_DIR "/monaco.osm.pbf");
    config.profile_path = OSRM_TEST_DATA_DIR "/profiles/bad_segment.lua";
    config.small_component_size = 1000;
    config.requested_num_threads = std::thread::hardware_concurrency();

    std::stringstream output;

    {
        redirect_stderr redir(output.rdbuf());
        BOOST_CHECK_THROW(osrm::extract(config), osrm::util::exception);
    }

    // We just look for the line number, file name, and error message. This avoids portability
    // issues since the output contains the full path to the file, which may change between systems
    BOOST_CHECK(boost::algorithm::contains(
        output.str(), "bad_segment.lua:132: attempt to compare number with nil"));
}

BOOST_AUTO_TEST_CASE(test_turn_runtime_error)
{
    osrm::ExtractorConfig config;
    config.input_path = OSRM_TEST_DATA_DIR "/monaco.osm.pbf";
    config.UseDefaultOutputNames(OSRM_TEST_DATA_DIR "/monaco.osm.pbf");
    config.profile_path = OSRM_TEST_DATA_DIR "/profiles/bad_turn.lua";
    config.small_component_size = 1000;
    config.requested_num_threads = std::thread::hardware_concurrency();

    std::stringstream output;

    {
        redirect_stderr redir(output.rdbuf());
        BOOST_CHECK_THROW(osrm::extract(config), osrm::util::exception);
    }

    // We just look for the line number, file name, and error message. This avoids portability
    // issues since the output contains the full path to the file, which may change between systems
    BOOST_CHECK(boost::algorithm::contains(output.str(),
                                           "bad_turn.lua:122: attempt to compare number with nil"));
}

BOOST_AUTO_TEST_SUITE_END()
