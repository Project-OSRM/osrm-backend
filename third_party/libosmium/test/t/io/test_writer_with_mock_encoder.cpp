#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/compression.hpp>
#include <osmium/io/detail/output_format.hpp>
#include <osmium/io/detail/queue_util.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/io/xml_input.hpp>

#include <stdexcept>
#include <string>
#include <utility>

class MockOutputFormat : public osmium::io::detail::OutputFormat {

    std::string m_fail_in;

public:

    MockOutputFormat(osmium::thread::Pool& pool, const osmium::io::File& /*file*/, osmium::io::detail::future_string_queue_type& output_queue, std::string fail_in) :
        OutputFormat(pool, output_queue),
        m_fail_in(std::move(fail_in)) {
    }

    void write_header(const osmium::io::Header& /*header*/) final {
        if (m_fail_in == "header") {
            throw std::logic_error{"header"};
        }
        send_to_output_queue(std::string{"header"});
    }

    void write_buffer(osmium::memory::Buffer&& /*buffer*/) final {
        if (m_fail_in == "write") {
            throw std::logic_error{"write"};
        }
        send_to_output_queue(std::string{"write"});
    }

    void write_end() final {
        if (m_fail_in == "write_end") {
            throw std::logic_error{"write_end"};
        }
        send_to_output_queue(std::string{"end"});
    }

}; // class MockOutputFormat

TEST_CASE("Test Writer with MockOutputFormat") {

    std::string fail_in;

    osmium::io::detail::OutputFormatFactory::instance().register_output_format(
        osmium::io::file_format::xml,
        [&](osmium::thread::Pool& pool, const osmium::io::File& file, osmium::io::detail::future_string_queue_type& output_queue) {
            return new MockOutputFormat{pool, file, output_queue, fail_in};
    });

    osmium::io::Header header;
    header.set("generator", "test_writer_with_mock_encoder.cpp");

    osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
    osmium::memory::Buffer buffer = reader.read();
    REQUIRE(buffer);
    REQUIRE(buffer.committed() > 0);
    REQUIRE_FALSE(buffer.select<osmium::OSMObject>().empty());
    REQUIRE(buffer.select<osmium::OSMObject>().size() > 0); // NOLINT(readability-container-size-empty)

    SECTION("error in header") {

        fail_in = "header";

        REQUIRE_THROWS_AS([&](){
            osmium::io::Writer writer("test-writer-mock-fail-on-construction.osm", header, osmium::io::overwrite::allow);
            writer(std::move(buffer));
            writer.close();
        }(), std::logic_error);

    }

    SECTION("error in write") {

        fail_in = "write";

        REQUIRE_THROWS_AS([&](){
            osmium::io::Writer writer("test-writer-mock-fail-on-construction.osm", header, osmium::io::overwrite::allow);
            writer(std::move(buffer));
            writer.close();
        }(), std::logic_error);

    }

    SECTION("error in write_end") {

        fail_in = "write_end";

        REQUIRE_THROWS_AS([&](){
            osmium::io::Writer writer("test-writer-mock-fail-on-construction.osm", header, osmium::io::overwrite::allow);
            writer(std::move(buffer));
            writer.close();
        }(), std::logic_error);

    }

}

