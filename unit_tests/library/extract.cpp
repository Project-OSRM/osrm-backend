#include <boost/test/unit_test.hpp>

#include "osrm/exception.hpp"
#include "osrm/extractor.hpp"
#include "osrm/extractor_config.hpp"

#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>

// Thread-safe streambuf wrapper. std::cerr's thread-safety guarantee depends on
// its original buffer's internal locking. When we replace it with a plain
// std::stringbuf for testing, concurrent TBB writes cause heap corruption.
// This wrapper serializes all writes to the underlying buffer.
class synchronized_streambuf : public std::streambuf
{
  public:
    explicit synchronized_streambuf(std::streambuf *wrapped) : wrapped_(wrapped) {}

  protected:
    int overflow(int c) override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (traits_type::eq_int_type(c, traits_type::eof()))
            return traits_type::not_eof(c);
        return wrapped_->sputc(traits_type::to_char_type(c));
    }

    std::streamsize xsputn(const char *s, std::streamsize n) override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return wrapped_->sputn(s, n);
    }

    int sync() override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return wrapped_->pubsync();
    }

  private:
    std::streambuf *wrapped_;
    std::mutex mtx_;
};

class redirect_stderr
{
  public:
    explicit redirect_stderr(std::streambuf *buf)
        : sync_buf_(buf), old_(std::cerr.rdbuf(&sync_buf_))
    {
    }
    ~redirect_stderr() { std::cerr.rdbuf(old_); }

  private:
    synchronized_streambuf sync_buf_;
    std::streambuf *old_;
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

BOOST_AUTO_TEST_CASE(test_extract_with_custom_output_path)
{
    osrm::ExtractorConfig config;
    config.input_path = OSRM_TEST_DATA_DIR "/monaco.osm.pbf";
    // Use custom output path instead of deriving from input
    config.UseDefaultOutputNames(OSRM_TEST_DATA_DIR "/monaco-custom-output");
    config.profile_path = OSRM_TEST_DATA_DIR "/../../profiles/car.lua";
    config.small_component_size = 1000;
    config.requested_num_threads = std::thread::hardware_concurrency();
    BOOST_CHECK_NO_THROW(osrm::extract(config));

    // Verify output files exist at custom path
    BOOST_CHECK(std::filesystem::exists(OSRM_TEST_DATA_DIR "/monaco-custom-output.osrm.names"));
    BOOST_CHECK(std::filesystem::exists(OSRM_TEST_DATA_DIR "/monaco-custom-output.osrm.ebg"));
    BOOST_CHECK(
        std::filesystem::exists(OSRM_TEST_DATA_DIR "/monaco-custom-output.osrm.properties"));
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
    BOOST_CHECK((output.str()).find("bad_setup.lua:6: attempt to compare number with nil") !=
                std::string::npos);
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
    BOOST_CHECK((output.str()).find("bad_way.lua:41: attempt to compare number with nil") !=
                std::string::npos);
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
    BOOST_CHECK((output.str()).find("bad_node.lua:36: attempt to compare number with nil") !=
                std::string::npos);
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
    BOOST_CHECK((output.str()).find("bad_segment.lua:132: attempt to compare number with nil") !=
                std::string::npos);
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
    BOOST_CHECK((output.str()).find("bad_turn.lua:122: attempt to compare number with nil") !=
                std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
