#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/any_compression.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/memory/buffer.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

static osmium::memory::Buffer get_buffer() {
    osmium::io::Reader reader{with_data_dir("t/io/data.osm")};
    osmium::memory::Buffer buffer = reader.read();
    REQUIRE(buffer);
    REQUIRE(buffer.committed() > 0);

    return buffer;
}

static osmium::memory::Buffer get_and_check_buffer() {
    auto buffer = get_buffer();

    const auto num = std::distance(buffer.select<osmium::OSMObject>().cbegin(), buffer.select<osmium::OSMObject>().cend());
    REQUIRE(num > 0);
    REQUIRE(buffer.select<osmium::OSMObject>().cbegin()->id() == 1);

    return buffer;
}

TEST_CASE("Writer: Empty writes") {
    const int count = count_fds();

    std::string filename;

    SECTION("Empty buffer") {
        filename = "test-writer-out-empty-buffer.osm";
        osmium::io::Writer writer{filename, osmium::io::overwrite::allow};
        osmium::memory::Buffer empty_buffer{1024};
        writer(std::move(empty_buffer));
        writer.close();
    }

    SECTION("Invalid buffer") {
        filename = "test-writer-out-invalid-buffer.osm";
        osmium::io::Writer writer{filename, osmium::io::overwrite::allow};
        osmium::memory::Buffer invalid_buffer;
        writer(std::move(invalid_buffer));
        writer.close();
    }

    osmium::io::Reader reader_check{filename};
    osmium::memory::Buffer buffer_check = reader_check.read();
    REQUIRE_FALSE(buffer_check);

    REQUIRE(count == count_fds());
}

TEST_CASE("Writer: Successful writes writing buffer") {
    const int count = count_fds();

    auto buffer = get_buffer();

    const auto num = std::distance(buffer.select<osmium::OSMObject>().cbegin(), buffer.select<osmium::OSMObject>().cend());
    REQUIRE(num > 0);
    REQUIRE(buffer.select<osmium::OSMObject>().cbegin()->id() == 1);

    std::string filename = "test-writer-out-buffer.osm";
    osmium::io::Writer writer{filename, osmium::io::Header{}, osmium::io::overwrite::allow};
    writer(std::move(buffer));
    writer.close();

    REQUIRE(count == count_fds());

    REQUIRE_THROWS_AS(writer(osmium::memory::Buffer{}), const osmium::io_error&);

    osmium::io::Reader reader_check{filename};
    const osmium::memory::Buffer buffer_check = reader_check.read();
    REQUIRE(buffer_check);
    REQUIRE(buffer_check.committed() > 0);
    REQUIRE(buffer_check.select<osmium::OSMObject>().size() == num);
    REQUIRE(buffer_check.select<osmium::OSMObject>().cbegin()->id() == 1);
}

TEST_CASE("Writer: Successful writes writing items") {
    const int count = count_fds();

    auto buffer = get_buffer();

    const auto num = std::distance(buffer.select<osmium::OSMObject>().cbegin(), buffer.select<osmium::OSMObject>().cend());
    REQUIRE(num > 0);
    REQUIRE(buffer.select<osmium::OSMObject>().cbegin()->id() == 1);

    std::string filename = "test-writer-out-item.osm";
    osmium::io::Writer writer{filename, osmium::io::overwrite::allow};
    for (const auto& item : buffer) {
        writer(item);
    }
    writer.close();

    REQUIRE(count == count_fds());

    osmium::io::Reader reader_check{filename};
    const osmium::memory::Buffer buffer_check = reader_check.read();
    REQUIRE(buffer_check);
    REQUIRE(buffer_check.committed() > 0);
    REQUIRE(buffer_check.select<osmium::OSMObject>().size() == num);
    REQUIRE(buffer_check.select<osmium::OSMObject>().cbegin()->id() == 1);
}

TEST_CASE("Writer: Successful writes using output iterator") {
    const int count = count_fds();

    auto buffer = get_buffer();

    const auto num = std::distance(buffer.select<osmium::OSMObject>().cbegin(), buffer.select<osmium::OSMObject>().cend());
    REQUIRE(num > 0);
    REQUIRE(buffer.select<osmium::OSMObject>().cbegin()->id() == 1);

    std::string filename = "test-writer-out-iterator.osm";
    osmium::io::Writer writer{filename, osmium::io::overwrite::allow};
    auto it = osmium::io::make_output_iterator(writer);
    std::copy(buffer.cbegin(), buffer.cend(), it);
    writer.close();

    REQUIRE(count == count_fds());

    osmium::io::Reader reader_check{filename};
    const osmium::memory::Buffer buffer_check = reader_check.read();
    REQUIRE(buffer_check);
    REQUIRE(buffer_check.committed() > 0);
    REQUIRE(buffer_check.select<osmium::OSMObject>().size() == num);
    REQUIRE(buffer_check.select<osmium::OSMObject>().cbegin()->id() == 1);
}

TEST_CASE("Writer: Interrupted writer after open") {
    const int count = count_fds();

    auto buffer = get_and_check_buffer();

    bool error = false;
    try {
        osmium::io::Writer writer{"test-writer-out-fail1.osm", osmium::io::overwrite::allow};
        throw std::runtime_error{"some error"};
    } catch (...) {
        error = true;
    }

    REQUIRE(error);

    REQUIRE(count == count_fds());
}

TEST_CASE("Writer: Interrupted writer after write") {
    const int count = count_fds();

    auto buffer = get_and_check_buffer();

    bool error = false;
    try {
        osmium::io::Writer writer{"test-writer-out-fail2.osm", osmium::io::overwrite::allow};
        writer(std::move(buffer));
        throw std::runtime_error{"some error"};
    } catch (...) {
        error = true;
    }

    REQUIRE(error);

    REQUIRE(count == count_fds());
}

TEST_CASE("Writer with user-provided pool with default number of threads") {
    const int count = count_fds();

    auto buffer = get_buffer();
    osmium::thread::Pool pool;
    osmium::io::Writer writer{"test-writer-pool-with-default-threads.osm", pool, osmium::io::overwrite::allow};
    writer(std::move(buffer));
    writer.close();

    REQUIRE(count == count_fds());
}

TEST_CASE("Writer with user-provided pool with negative number of threads") {
    const int count = count_fds();

    auto buffer = get_buffer();
    osmium::thread::Pool pool{-2};
    osmium::io::Writer writer{"test-writer-pool-with-negative-threads.osm", pool, osmium::io::overwrite::allow};
    writer(std::move(buffer));
    writer.close();

    REQUIRE(count == count_fds());
}

TEST_CASE("Writer with user-provided pool with outlier negative number of threads") {
    const int count = count_fds();

    auto buffer = get_buffer();
    osmium::thread::Pool pool{-1000};
    osmium::io::Writer writer{"test-writer-pool-with-outlier-negative-threads.osm", osmium::io::overwrite::allow, pool};
    writer(std::move(buffer));
    writer.close();

    REQUIRE(count == count_fds());
}

TEST_CASE("Writer with user-provided pool with outlier positive number of threads") {
    const int count = count_fds();

    auto buffer = get_buffer();
    osmium::thread::Pool pool{1000};
    osmium::io::Writer writer{"test-writer-pool-with-outlier-positive-threads.osm", osmium::io::overwrite::allow, pool};
    writer(std::move(buffer));
    writer.close();

    REQUIRE(count == count_fds());
}

