
#include "catch.hpp"
#include "utils.hpp"

#include <stdexcept>
#include <string>

#include <osmium/io/compression.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/io/xml_output.hpp>

class MockCompressor : public osmium::io::Compressor {

    std::string m_fail_in;

public:

    explicit MockCompressor(const std::string& fail_in) :
        Compressor(osmium::io::fsync::no),
        m_fail_in(fail_in) {
        if (m_fail_in == "constructor") {
            throw std::logic_error{"constructor"};
        }
    }

    ~MockCompressor() noexcept final = default;

    void write(const std::string&) final {
        if (m_fail_in == "write") {
            throw std::logic_error{"write"};
        }
    }

    void close() final {
        if (m_fail_in == "close") {
            throw std::logic_error{"close"};
        }
    }

}; // class MockCompressor

TEST_CASE("Write with mock compressor") {

    std::string fail_in;

    osmium::io::CompressionFactory::instance().register_compression(osmium::io::file_compression::gzip,
        [&](int, osmium::io::fsync) { return new MockCompressor(fail_in); },
        [](int) { return nullptr; },
        [](const char*, size_t) { return nullptr; }
    );

    osmium::io::Header header;
    header.set("generator", "test_writer_with_mock_compression.cpp");

    osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
    osmium::memory::Buffer buffer = reader.read();
    REQUIRE(buffer);
    REQUIRE(buffer.committed() > 0);
    REQUIRE(buffer.select<osmium::OSMObject>().size() > 0);

    SECTION("fail on construction") {

        fail_in = "constructor";

        REQUIRE_THROWS_AS({
            osmium::io::Writer writer("test-writer-mock-fail-on-construction.osm.gz", header, osmium::io::overwrite::allow);
            writer(std::move(buffer));
            writer.close();
        }, std::logic_error);

    }

    SECTION("fail on write") {

        fail_in = "write";

        REQUIRE_THROWS_AS({
            osmium::io::Writer writer("test-writer-mock-fail-on-write.osm.gz", header, osmium::io::overwrite::allow);
            writer(std::move(buffer));
            writer.close();
        }, std::logic_error);

    }

    SECTION("fail on close") {

        fail_in = "close";

        REQUIRE_THROWS_AS({
            osmium::io::Writer writer("test-writer-mock-fail-on-close.osm.gz", header, osmium::io::overwrite::allow);
            writer(std::move(buffer));
            writer.close();
        }, std::logic_error);

    }

}

