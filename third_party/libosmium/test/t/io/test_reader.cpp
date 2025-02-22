#include "catch.hpp"

#include "utils.hpp"

#include <osmium/handler.hpp>
#include <osmium/io/any_compression.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/visitor.hpp>

#include <iterator>
#include <stdexcept>
#include <vector>

struct CountHandler : public osmium::handler::Handler {

    int count = 0;

    void node(const osmium::Node& /*node*/) {
        ++count;
    }

}; // class CountHandler

struct ZeroPositionNodeCountHandler : public osmium::handler::Handler {

    // number of nodes seen at zero position, or visible with undefined
    // location.
    int count = 0;
    int total_count = 0; // total number of nodes seen
    const osmium::Location zero = osmium::Location{static_cast<int32_t>(0), static_cast<int32_t>(0)};

    void node(const osmium::Node& node) {
        // no nodes in the history file have a zero location, and
        // no visible nodes should have an undefined location.
        if ((node.location() == zero) ||
            (node.visible() && !node.location())) {
            ++count;
        }
        ++total_count;
    }

}; // class ZeroPositionNodeCountHandler


TEST_CASE("Reader can be initialized with file") {
    const osmium::io::File file{with_data_dir("t/io/data.osm")};

    const int count = count_fds();
    osmium::io::Reader reader{file};
    osmium::handler::Handler handler;

    osmium::apply(reader, handler);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader can be initialized with string") {
    const int count = count_fds();

    osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
    osmium::handler::Handler handler;

    osmium::apply(reader, handler);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader can be initialized with user-provided pool") {
    const int count = count_fds();

    osmium::thread::Pool pool{4};
    const osmium::io::File file{with_data_dir("t/io/data.osm")};
    osmium::io::Reader reader{file, pool};
    osmium::handler::Handler handler;

    osmium::apply(reader, handler);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should throw after eof") {
    const int count = count_fds();

    const osmium::io::File file{with_data_dir("t/io/data.osm")};
    osmium::io::Reader reader{file};

    SECTION("Get header") {
        const auto header = reader.header();
        REQUIRE(header.get("generator") == "testdata");
        REQUIRE_FALSE(reader.eof());
    }

    REQUIRE_FALSE(reader.eof());

    while (const osmium::memory::Buffer buffer = reader.read()) {
    }

    REQUIRE(reader.eof());

    REQUIRE_THROWS_AS(reader.read(), osmium::io_error);

    reader.close();
    REQUIRE(reader.eof());
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should not hang when apply() is called twice on reader") {
    const int count = count_fds();

    const osmium::io::File file{with_data_dir("t/io/data.osm")};
    osmium::io::Reader reader{file};
    osmium::handler::Handler handler;

    osmium::apply(reader, handler);
    REQUIRE_THROWS_AS(osmium::apply(reader, handler), osmium::io_error);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should work with a buffer with uncompressed data") {
    const int count = count_fds();

    const int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm"));
    REQUIRE(fd >= 0);

    const size_t buffer_size = 1000;
    char buffer[buffer_size];
    const auto length = ::read(fd, buffer, buffer_size);
    REQUIRE(length > 0);
    osmium::io::detail::reliable_close(fd);

    const osmium::io::File file{buffer, static_cast<size_t>(length), "osm"};
    osmium::io::Reader reader{file};
    CountHandler handler;

    REQUIRE(handler.count == 0);
    osmium::apply(reader, handler);
    REQUIRE(handler.count == 1);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should work with a buffer with gzip-compressed data") {
    const int count = count_fds();

    const int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.gz"));
    REQUIRE(fd >= 0);

    const size_t buffer_size = 1000;
    char buffer[buffer_size];
    const auto length = ::read(fd, buffer, buffer_size);
    REQUIRE(length > 0);
    osmium::io::detail::reliable_close(fd);

    const osmium::io::File file{buffer, static_cast<size_t>(length), "osm.gz"};
    osmium::io::Reader reader{file};
    CountHandler handler;

    REQUIRE(handler.count == 0);
    osmium::apply(reader, handler);
    REQUIRE(handler.count == 1);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should work with a buffer with bzip2-compressed data") {
    const int count = count_fds();

    const int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.bz2"));
    REQUIRE(fd >= 0);

    const size_t buffer_size = 1000;
    char buffer[buffer_size];
    const auto length = ::read(fd, buffer, buffer_size);
    REQUIRE(length > 0);
    osmium::io::detail::reliable_close(fd);

    const osmium::io::File file{buffer, static_cast<size_t>(length), "osm.bz2"};
    osmium::io::Reader reader{file};
    CountHandler handler;

    REQUIRE(handler.count == 0);
    osmium::apply(reader, handler);
    REQUIRE(handler.count == 1);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should decode zero node positions in history (XML)") {
    const int count = count_fds();

    osmium::io::Reader reader{with_data_dir("t/io/deleted_nodes.osh"),
                              osmium::osm_entity_bits::node};
    ZeroPositionNodeCountHandler handler;

    REQUIRE(handler.count == 0);
    REQUIRE(handler.total_count == 0);

    osmium::apply(reader, handler);

    REQUIRE(handler.count == 0);
    REQUIRE(handler.total_count == 2);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should decode zero node positions in history (PBF)") {
    const int count = count_fds();

    osmium::io::Reader reader{with_data_dir("t/io/deleted_nodes.osh.pbf"),
                              osmium::osm_entity_bits::node};
    ZeroPositionNodeCountHandler handler;

    REQUIRE(handler.count == 0);
    REQUIRE(handler.total_count == 0);

    osmium::apply(reader, handler);

    REQUIRE(handler.count == 0);
    REQUIRE(handler.total_count == 2);

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should fail with nonexistent file") {
    const int count = count_fds();

    REQUIRE_THROWS(osmium::io::Reader{with_data_dir("t/io/nonexistent-file.osm")});

    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should fail with nonexistent file (gz)") {
    const int count = count_fds();

    REQUIRE_THROWS(osmium::io::Reader{with_data_dir("t/io/nonexistent-file.osm.gz")});

    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should fail with nonexistent file (pbf)") {
    const int count = count_fds();

    REQUIRE_THROWS(osmium::io::Reader{with_data_dir("t/io/nonexistent-file.osm.pbf")});

    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should work when there is an exception in main thread before getting header") {
    const int count = count_fds();

    try {
        const osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
        REQUIRE_FALSE(reader.eof());
        throw std::runtime_error{"foo"};
    } catch (...) {
    }

    REQUIRE(count == count_fds());
}

TEST_CASE("Reader should work when there is an exception in main thread while reading") {
    const int count = count_fds();

    try {
        osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
        REQUIRE_FALSE(reader.eof());
        auto header = reader.header();
        throw std::runtime_error{"foo"};
    } catch (...) {
    }

    REQUIRE(count == count_fds());
}

TEST_CASE("Applying rvalue handler on reader") {
    const int count = count_fds();

    osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
    struct NullHandler : public osmium::handler::Handler { };
    osmium::apply(reader, NullHandler{});

    reader.close();
    REQUIRE(count == count_fds());
}

TEST_CASE("Can call read() exactly once on Reader with entity_bits nothing") {
    const int count = count_fds();

    const osmium::io::File file{with_data_dir("t/io/data.osm")};
    osmium::io::Reader reader{file, osmium::osm_entity_bits::nothing};
    REQUIRE_FALSE(reader.eof());

    SECTION("Get header") {
        const auto header = reader.header();
        REQUIRE(header.get("generator") == "testdata");
        REQUIRE_FALSE(reader.eof());
    }

    const osmium::memory::Buffer buffer = reader.read();
    REQUIRE_FALSE(buffer);
    REQUIRE(reader.eof());
    REQUIRE_THROWS_AS(reader.read(), osmium::io_error);

    reader.close();
    REQUIRE(reader.eof());

    REQUIRE(count == count_fds());
}

TEST_CASE("Can not read after close") {
    const int count = count_fds();

    const osmium::io::File file{with_data_dir("t/io/data.osm")};
    osmium::io::Reader reader{file};

    SECTION("Get header") {
        const auto header = reader.header();
        REQUIRE(header.get("generator") == "testdata");
        REQUIRE_FALSE(reader.eof());
    }

    REQUIRE_FALSE(reader.eof());

    const osmium::memory::Buffer buffer = reader.read();
    REQUIRE(buffer);
    REQUIRE_FALSE(reader.eof());

    reader.close();
    REQUIRE(reader.eof());
    REQUIRE_THROWS_AS(reader.read(), osmium::io_error);

    REQUIRE(count == count_fds());
}

using object_counts = std::array<std::size_t, 3>;

std::vector<object_counts> count_objects_per_buffer(const char* filename, osmium::io::buffers_type btype) {
    const osmium::io::File file{with_data_dir(filename)};
    osmium::io::Reader reader{file, btype};

    std::vector<object_counts> counts;
    while (osmium::memory::Buffer buffer = reader.read()) {
        const auto rn = buffer.select<osmium::Node>();
        const auto rw = buffer.select<osmium::Way>();
        const auto rr = buffer.select<osmium::Relation>();
        counts.push_back(object_counts{static_cast<std::size_t>(std::distance(rn.begin(), rn.end())),
                                       static_cast<std::size_t>(std::distance(rw.begin(), rw.end())),
                                       static_cast<std::size_t>(std::distance(rr.begin(), rr.end()))});
    }

    return counts;
}

void check_buffer_counts(const std::string& filename, const std::vector<object_counts>& oc, osmium::io::buffers_type btype) {
    for (const auto* suffix : {".osm", ".osm.opl", ".osm.o5m"}) {
        const std::string fn = filename + suffix;
        const auto counts = count_objects_per_buffer(fn.c_str(), btype);
        REQUIRE(counts == oc);
    }
}

TEST_CASE("Reader with single object type per buffer") {
    check_buffer_counts("t/io/data-n5w1r3", {{5, 1, 3}}, osmium::io::buffers_type::any);
    check_buffer_counts("t/io/data-n5w1r3", {{5, 0, 0}, {0, 1, 0}, {0, 0, 3}}, osmium::io::buffers_type::single);
    check_buffer_counts("t/io/data-n0w1r3", {{0, 1, 0}, {0, 0, 3}}, osmium::io::buffers_type::single);
    check_buffer_counts("t/io/data-n5w0r3", {{5, 0, 0}, {0, 0, 3}}, osmium::io::buffers_type::single);
    check_buffer_counts("t/io/data-n5w1r0", {{5, 0, 0}, {0, 1, 0}}, osmium::io::buffers_type::single);
}

