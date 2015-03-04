/* The code in this file is released into the Public Domain. */

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

#include <osmium/io/xml_input.hpp>
#include <osmium/io/gzip_compression.hpp>
#include <osmium/visitor.hpp>

std::string filename(const char* test_id, const char* suffix = "osm") {
    const char* testdir = getenv("TESTDIR");
    if (!testdir) {
        std::cerr << "You have to set TESTDIR environment variable before running testdata-xml\n";
        exit(2);
    }

    std::string f;
    f += testdir;
    f += "/";
    f += test_id;
    f += "/data.";
    f += suffix;
    return f;
}

struct header_buffer_type {
    osmium::io::Header header;
    osmium::memory::Buffer buffer;
};

// =============================================

// The following helper functions are used to call different parts of the
// Osmium internals used to read and parse XML files. This way those parts
// can be tested individually. These function can not be used in normal
// operations, because they make certain assumptions, for instance that
// file contents fit into small buffers.

std::string read_file(const char* test_id) {
    int fd = osmium::io::detail::open_for_reading(filename(test_id));
    assert(fd >= 0);

    std::string input(10000, '\0');
    auto n = ::read(fd, reinterpret_cast<unsigned char*>(const_cast<char*>(input.data())), 10000);
    assert(n >= 0);
    input.resize(static_cast<std::string::size_type>(n));

    close(fd);

    return input;
}

std::string read_gz_file(const char* test_id, const char* suffix) {
    int fd = osmium::io::detail::open_for_reading(filename(test_id, suffix));
    assert(fd >= 0);

    osmium::io::GzipDecompressor gzip_decompressor(fd);
    std::string input = gzip_decompressor.read();
    gzip_decompressor.close();

    return input;
}


header_buffer_type parse_xml(std::string input) {
    osmium::thread::Queue<std::string> input_queue;
    osmium::thread::Queue<osmium::memory::Buffer> output_queue;
    std::promise<osmium::io::Header> header_promise;
    std::atomic<bool> done {false};
    input_queue.push(input);
    input_queue.push(std::string()); // EOF marker

    osmium::io::detail::XMLParser parser(input_queue, output_queue, header_promise, osmium::osm_entity_bits::all, done);
    parser();

    header_buffer_type result;
    result.header = header_promise.get_future().get();
    output_queue.wait_and_pop(result.buffer);

    if (result.buffer) {
        osmium::memory::Buffer buffer;
        output_queue.wait_and_pop(buffer);
        assert(!buffer);
    }

    return result;
}

header_buffer_type read_xml(const char* test_id) {
    std::string input = read_file(test_id);
    return parse_xml(input);
}

// =============================================

TEST_CASE("Reading OSM XML 100") {

    SECTION("Direct") {
        header_buffer_type r = read_xml("100-correct_but_no_data");

        REQUIRE(r.header.get("generator") == "testdata");
        REQUIRE(0 == r.buffer.committed());
        REQUIRE(! r.buffer);
    }

    SECTION("Using Reader") {
        osmium::io::Reader reader(filename("100-correct_but_no_data"));

        osmium::io::Header header = reader.header();
        REQUIRE(header.get("generator") == "testdata");

        osmium::memory::Buffer buffer = reader.read();
        REQUIRE(0 == buffer.committed());
        REQUIRE(! buffer);
        reader.close();
    }

    SECTION("Using Reader asking for header only") {
        osmium::io::Reader reader(filename("100-correct_but_no_data"), osmium::osm_entity_bits::nothing);

        osmium::io::Header header = reader.header();
        REQUIRE(header.get("generator") == "testdata");
        reader.close();
    }

}

// =============================================

TEST_CASE("Reading OSM XML 101") {

    SECTION("Direct") {
        REQUIRE_THROWS_AS(read_xml("101-missing_version"), osmium::format_version_error);
        try {
            read_xml("101-missing_version");
        } catch (osmium::format_version_error& e) {
            REQUIRE(e.version.empty());
        }
    }

    SECTION("Using Reader") {
        REQUIRE_THROWS_AS({
            osmium::io::Reader reader(filename("101-missing_version"));
            osmium::io::Header header = reader.header();
            osmium::memory::Buffer buffer = reader.read();
            reader.close();
        }, osmium::format_version_error);
    }

}

// =============================================

TEST_CASE("Reading OSM XML 102") {

    SECTION("Direct") {
        REQUIRE_THROWS_AS(read_xml("102-wrong_version"), osmium::format_version_error);
        try {
            read_xml("102-wrong_version");
        } catch (osmium::format_version_error& e) {
            REQUIRE(e.version == "0.1");
        }
    }

    SECTION("Using Reader") {
        REQUIRE_THROWS_AS({
            osmium::io::Reader reader(filename("102-wrong_version"));

            osmium::io::Header header = reader.header();
            osmium::memory::Buffer buffer = reader.read();
            reader.close();
        }, osmium::format_version_error);
    }

}

// =============================================

TEST_CASE("Reading OSM XML 103") {

    SECTION("Direct") {
        REQUIRE_THROWS_AS(read_xml("103-old_version"), osmium::format_version_error);
        try {
            read_xml("103-old_version");
        } catch (osmium::format_version_error& e) {
            REQUIRE(e.version == "0.5");
        }
    }

    SECTION("Using Reader") {
        REQUIRE_THROWS_AS({
            osmium::io::Reader reader(filename("103-old_version"));
            osmium::io::Header header = reader.header();
            osmium::memory::Buffer buffer = reader.read();
            reader.close();
        }, osmium::format_version_error);
    }

}

// =============================================

TEST_CASE("Reading OSM XML 104") {

    SECTION("Direct") {
        REQUIRE_THROWS_AS(read_xml("104-empty_file"), osmium::xml_error);
        try {
            read_xml("104-empty_file");
        } catch (osmium::xml_error& e) {
            REQUIRE(e.line == 1);
            REQUIRE(e.column == 0);
        }
    }

    SECTION("Using Reader") {
        REQUIRE_THROWS_AS({
            osmium::io::Reader reader(filename("104-empty_file"));
            osmium::io::Header header = reader.header();
            osmium::memory::Buffer buffer = reader.read();
            reader.close();
        }, osmium::xml_error);
    }
}

// =============================================

TEST_CASE("Reading OSM XML 105") {

    SECTION("Direct") {
        REQUIRE_THROWS_AS(read_xml("105-incomplete_xml_file"), osmium::xml_error);
    }

    SECTION("Using Reader") {
        REQUIRE_THROWS_AS({
            osmium::io::Reader reader(filename("105-incomplete_xml_file"));
            osmium::io::Header header = reader.header();
            osmium::memory::Buffer buffer = reader.read();
            reader.close();
        }, osmium::xml_error);
    }

}

// =============================================

TEST_CASE("Reading OSM XML 120") {

    SECTION("Direct") {
        std::string data = read_gz_file("120-correct_gzip_file_without_data", "osm.gz");

        REQUIRE(data.size() == 102);

        header_buffer_type r = parse_xml(data);
        REQUIRE(r.header.get("generator") == "testdata");
        REQUIRE(0 == r.buffer.committed());
        REQUIRE(! r.buffer);
    }

    SECTION("Using Reader") {
        osmium::io::Reader reader(filename("120-correct_gzip_file_without_data", "osm.gz"));

        osmium::io::Header header = reader.header();
        REQUIRE(header.get("generator") == "testdata");

        osmium::memory::Buffer buffer = reader.read();
        REQUIRE(0 == buffer.committed());
        REQUIRE(! buffer);
        reader.close();
    }

}

// =============================================

TEST_CASE("Reading OSM XML 121") {

    SECTION("Direct") {
        REQUIRE_THROWS_AS( {
            read_gz_file("121-truncated_gzip_file", "osm.gz");
        }, osmium::gzip_error);
    }

#if 0
    SECTION("Using Reader") {
        REQUIRE_THROWS_AS({
            osmium::io::Reader reader(filename("121-truncated_gzip_file", "osm.gz"));
            osmium::io::Header header = reader.header();
            osmium::memory::Buffer buffer = reader.read();
            reader.close();
        }, osmium::gzip_error);
    }
#endif

}

// =============================================

TEST_CASE("Reading OSM XML 200") {

    SECTION("Direct") {
        header_buffer_type r = read_xml("200-nodes");

        REQUIRE(r.header.get("generator") == "testdata");
        REQUIRE(r.buffer.committed() > 0);
        REQUIRE(r.buffer.get<osmium::memory::Item>(0).type() == osmium::item_type::node);
        REQUIRE(r.buffer.get<osmium::Node>(0).id() == 36966060);
        REQUIRE(std::distance(r.buffer.begin(), r.buffer.end()) == 3);
    }

    SECTION("Using Reader") {
        osmium::io::Reader reader(filename("200-nodes"));

        osmium::io::Header header = reader.header();
        REQUIRE(header.get("generator") == "testdata");

        osmium::memory::Buffer buffer = reader.read();
        REQUIRE(buffer.committed() > 0);
        REQUIRE(buffer.get<osmium::memory::Item>(0).type() == osmium::item_type::node);
        REQUIRE(buffer.get<osmium::Node>(0).id() == 36966060);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 3);
        reader.close();
    }

    SECTION("Using Reader asking for nodes") {
        osmium::io::Reader reader(filename("200-nodes"), osmium::osm_entity_bits::node);

        osmium::io::Header header = reader.header();
        REQUIRE(header.get("generator") == "testdata");

        osmium::memory::Buffer buffer = reader.read();
        REQUIRE(buffer.committed() > 0);
        REQUIRE(buffer.get<osmium::memory::Item>(0).type() == osmium::item_type::node);
        REQUIRE(buffer.get<osmium::Node>(0).id() == 36966060);
        REQUIRE(std::distance(buffer.begin(), buffer.end()) == 3);
        reader.close();
    }

    SECTION("Using Reader asking for header only") {
        osmium::io::Reader reader(filename("200-nodes"), osmium::osm_entity_bits::nothing);

        osmium::io::Header header = reader.header();
        REQUIRE(header.get("generator") == "testdata");

        osmium::memory::Buffer buffer = reader.read();
        REQUIRE(0 == buffer.committed());
        REQUIRE(! buffer);
        reader.close();
    }

    SECTION("Using Reader asking for ways") {
        osmium::io::Reader reader(filename("200-nodes"), osmium::osm_entity_bits::way);

        osmium::io::Header header = reader.header();
        REQUIRE(header.get("generator") == "testdata");

        osmium::memory::Buffer buffer = reader.read();
        REQUIRE(0 == buffer.committed());
        REQUIRE(! buffer);
        reader.close();
    }

}

