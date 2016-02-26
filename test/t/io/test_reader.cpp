#include "catch.hpp"
#include "utils.hpp"

#include <osmium/handler.hpp>
#include <osmium/io/any_compression.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/memory/buffer.hpp>

struct CountHandler : public osmium::handler::Handler {

    int count = 0;

    void node(osmium::Node&) {
        ++count;
    }

}; // class CountHandler

struct ZeroPositionNodeCountHandler : public osmium::handler::Handler {

    // number of nodes seen at zero position, or visible with undefined
    // location.
    int count = 0;
    int total_count = 0; // total number of nodes seen
    const osmium::Location zero = osmium::Location(int32_t(0), int32_t(0));

    void node(osmium::Node &n) {
        // no nodes in the history file have a zero location, and
        // no visible nodes should have an undefined location.
        if ((n.location() == zero) ||
            (n.visible() && !n.location())) {
            ++count;
        }
        ++total_count;
    }

}; // class ZeroPositionNodeCountHandler


TEST_CASE("Reader") {

    SECTION("reader can be initialized with file") {
        osmium::io::File file(with_data_dir("t/io/data.osm"));
        osmium::io::Reader reader(file);
        osmium::handler::Handler handler;

        osmium::apply(reader, handler);
    }

    SECTION("reader can be initialized with string") {
        osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
        osmium::handler::Handler handler;

        osmium::apply(reader, handler);
    }

    SECTION("should throw after eof") {
        osmium::io::File file(with_data_dir("t/io/data.osm"));
        osmium::io::Reader reader(file);

        REQUIRE(!reader.eof());

        while (osmium::memory::Buffer buffer = reader.read()) {
        }

        REQUIRE(reader.eof());

        REQUIRE_THROWS_AS({
            reader.read();
        }, osmium::io_error);
    }

    SECTION("should not hang when apply() is called twice on reader") {
        osmium::io::File file(with_data_dir("t/io/data.osm"));
        osmium::io::Reader reader(file);
        osmium::handler::Handler handler;

        osmium::apply(reader, handler);
        REQUIRE_THROWS_AS({
            osmium::apply(reader, handler);
        }, osmium::io_error);
    }

    SECTION("should work with a buffer with uncompressed data") {
        int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm"));
        REQUIRE(fd >= 0);

        const size_t buffer_size = 1000;
        char buffer[buffer_size];
        auto length = ::read(fd, buffer, buffer_size);
        REQUIRE(length > 0);

        osmium::io::File file(buffer, static_cast<size_t>(length), "osm");
        osmium::io::Reader reader(file);
        CountHandler handler;

        REQUIRE(handler.count == 0);
        osmium::apply(reader, handler);
        REQUIRE(handler.count == 1);
    }

    SECTION("should work with a buffer with gzip-compressed data") {
        int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.gz"));
        REQUIRE(fd >= 0);

        const size_t buffer_size = 1000;
        char buffer[buffer_size];
        auto length = ::read(fd, buffer, buffer_size);
        REQUIRE(length > 0);

        osmium::io::File file(buffer, static_cast<size_t>(length), "osm.gz");
        osmium::io::Reader reader(file);
        CountHandler handler;

        REQUIRE(handler.count == 0);
        osmium::apply(reader, handler);
        REQUIRE(handler.count == 1);
    }

    SECTION("should work with a buffer with bzip2-compressed data") {
        int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.bz2"));
        REQUIRE(fd >= 0);

        const size_t buffer_size = 1000;
        char buffer[buffer_size];
        auto length = ::read(fd, buffer, buffer_size);
        REQUIRE(length > 0);

        osmium::io::File file(buffer, static_cast<size_t>(length), "osm.bz2");
        osmium::io::Reader reader(file);
        CountHandler handler;

        REQUIRE(handler.count == 0);
        osmium::apply(reader, handler);
        REQUIRE(handler.count == 1);
    }

    SECTION("should decode zero node positions in history (XML)") {
        osmium::io::Reader reader(with_data_dir("t/io/deleted_nodes.osh"),
                                  osmium::osm_entity_bits::node);
        ZeroPositionNodeCountHandler handler;

        REQUIRE(handler.count == 0);
        REQUIRE(handler.total_count == 0);

        osmium::apply(reader, handler);

        REQUIRE(handler.count == 0);
        REQUIRE(handler.total_count == 2);
    }

    SECTION("should decode zero node positions in history (PBF)") {
        osmium::io::Reader reader(with_data_dir("t/io/deleted_nodes.osh.pbf"),
                                  osmium::osm_entity_bits::node);
        ZeroPositionNodeCountHandler handler;

        REQUIRE(handler.count == 0);
        REQUIRE(handler.total_count == 0);

        osmium::apply(reader, handler);

        REQUIRE(handler.count == 0);
        REQUIRE(handler.total_count == 2);
    }

}

TEST_CASE("Reader failure modes") {

    SECTION("should fail with nonexistent file") {
        REQUIRE_THROWS({
            osmium::io::Reader reader(with_data_dir("t/io/nonexistent-file.osm"));
        });
    }

    SECTION("should fail with nonexistent file (gz)") {
        REQUIRE_THROWS({
            osmium::io::Reader reader(with_data_dir("t/io/nonexistent-file.osm.gz"));
        });
    }

    SECTION("should fail with nonexistent file (pbf)") {
        REQUIRE_THROWS({
            osmium::io::Reader reader(with_data_dir("t/io/nonexistent-file.osm.pbf"));
        });
    }

    SECTION("should work when there is an exception in main thread before getting header") {
        try {
            osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
            REQUIRE(!reader.eof());
            throw std::runtime_error("foo");
        } catch (...) {
        }

    }

    SECTION("should work when there is an exception in main thread while reading") {
        try {
            osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
            REQUIRE(!reader.eof());
            auto header = reader.header();
            throw std::runtime_error("foo");
        } catch (...) {
        }

    }

}

