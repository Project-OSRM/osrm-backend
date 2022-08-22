#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/compression.hpp>
#include <osmium/io/xml_input.hpp>

#include <stdexcept>
#include <string>
#include <utility>

// The MockDecompressor behaves like other Decompressor classes, but "invents"
// OSM data in XML format that can be read. Through a parameter to the
// constructor it can be instructed to throw an exception in specific parts
// of its code. This is then used to test the internals of the Reader.

class MockDecompressor final : public osmium::io::Decompressor {

    std::string m_fail_in;
    int m_read_count = 0;

public:

    explicit MockDecompressor(std::string fail_in) :
        m_fail_in(std::move(fail_in)) {
        if (m_fail_in == "constructor") {
            throw std::runtime_error{"error constructor"};
        }
    }

    MockDecompressor(const MockDecompressor&) = delete;
    MockDecompressor& operator=(const MockDecompressor&) = delete;

    MockDecompressor(MockDecompressor&&) = delete;
    MockDecompressor& operator=(MockDecompressor&&) = delete;

    ~MockDecompressor() noexcept override = default;

    static void add_node(std::string& s, int i) {
        s += "<node id='";
        s += std::to_string(i);
        s += "' version='1' timestamp='2014-01-01T00:00:00Z' uid='1' user='test' changeset='1' lon='1.02' lat='1.02'/>\n";
    }

    std::string read() override {
        std::string buffer;
        ++m_read_count;

        if (m_read_count == 1) {
            if (m_fail_in == "first read") {
                throw std::runtime_error{"error first read"};
            }
            buffer += "<?xml version='1.0' encoding='UTF-8'?>\n<osm version='0.6' generator='testdata'>\n";
            for (int i = 0; i < 1000; ++i) {
                add_node(buffer, i);
            }
        } else if (m_read_count == 2) {
            if (m_fail_in == "second read") {
                throw std::runtime_error{"error second read"};
            }
            for (int i = 1000; i < 2000; ++i) {
                add_node(buffer, i);
            }
        } else if (m_read_count == 3) {
            buffer += "</osm>";
        }

        return buffer;
    }

    void close() override {
        if (m_fail_in == "close") {
            throw std::runtime_error{"error close"};
        }
    }

}; // class MockDecompressor

TEST_CASE("Test Reader using MockDecompressor") {

    std::string fail_in;

    osmium::io::CompressionFactory::instance().register_compression(osmium::io::file_compression::gzip,
        [](int /*unused*/, osmium::io::fsync /*unused*/) { return nullptr; },
        [&](int /*unused*/) { return new MockDecompressor(fail_in); },
        [](const char* /*unused*/, size_t /*unused*/) { return nullptr; }
    );

    SECTION("fail in constructor") {
        fail_in = "constructor";

        try {
            osmium::io::Reader reader{with_data_dir("t/io/data.osm.gz")};
            REQUIRE(false);
        } catch (const std::runtime_error& e) {
            REQUIRE(std::string{e.what()} == "error constructor");
        }
    }

    SECTION("fail in first read") {
        fail_in = "first read";

        try {
            osmium::io::Reader reader{with_data_dir("t/io/data.osm.gz")};
            reader.read();
            REQUIRE(false);
        } catch (const std::runtime_error& e) {
            REQUIRE(std::string{e.what()} == "error first read");
        }
    }

    SECTION("fail in second read") {
        fail_in = "second read";

        try {
            osmium::io::Reader reader{with_data_dir("t/io/data.osm.gz")};
            reader.read();
            reader.read();
            REQUIRE(false);
        } catch (const std::runtime_error& e) {
            REQUIRE(std::string{e.what()} == "error second read");
        }
    }

    SECTION("fail in close") {
        fail_in = "close";

        try {
            osmium::io::Reader reader{with_data_dir("t/io/data.osm.gz")};
            reader.read();
            reader.read();
            reader.read();
            reader.close();
            REQUIRE(false);
        } catch (const std::runtime_error& e) {
            REQUIRE(std::string{e.what()} == "error close");
        }
    }

    SECTION("not failing") {
        fail_in = "not";

        osmium::io::Reader reader{with_data_dir("t/io/data.osm.gz")};
        reader.read();
        reader.close();
        REQUIRE(true);
    }

}

