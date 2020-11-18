#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/compression.hpp>

#include <string>

TEST_CASE("Invalid file descriptor of uncompressed file") {
    osmium::io::NoDecompressor decomp{-1};
    REQUIRE_THROWS_AS(decomp.read(), const std::system_error&);
}

TEST_CASE("Non-open file descriptor of uncompressed file") {
    // 12345 is just a random file descriptor that should not be open
    osmium::io::NoDecompressor decomp{12345};
    REQUIRE_THROWS_AS(decomp.read(), const std::system_error&);
}

TEST_CASE("Empty uncompressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/empty_file");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    osmium::io::NoDecompressor decomp{fd};
    REQUIRE(decomp.read().empty());
    decomp.close();

    REQUIRE(count == count_fds());
}

TEST_CASE("Read uncompressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/data.txt");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    size_t size = 0;
    std::string all;
    {
        osmium::io::NoDecompressor decomp{fd};
        for (std::string data = decomp.read(); !data.empty(); data = decomp.read()) {
            size += data.size();
            all += data;
        }
        decomp.close();
    }

    REQUIRE(size >= 9);
    all.resize(8);
    REQUIRE("TESTDATA" == all);

    REQUIRE(count == count_fds());
}

TEST_CASE("Read uncompressed file without explicit close") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/data.txt");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    size_t size = 0;
    std::string all;
    {
        osmium::io::NoDecompressor decomp{fd};
        for (std::string data = decomp.read(); !data.empty(); data = decomp.read()) {
            size += data.size();
            all += data;
        }
    }

    REQUIRE(size >= 9);
    all.resize(8);
    REQUIRE("TESTDATA" == all);

    REQUIRE(count == count_fds());
}

TEST_CASE("Compressor: Invalid file descriptor for uncompressed file") {
    osmium::io::NoCompressor comp{-1, osmium::io::fsync::yes};
    REQUIRE_THROWS_AS(comp.write("foo"), const std::system_error&);
}

TEST_CASE("Compressor: Non-open file descriptor for uncompressed file") {
    // 12345 is just a random file descriptor that should not be open
    osmium::io::NoCompressor comp{12345, osmium::io::fsync::yes};
    REQUIRE_THROWS_AS(comp.write("foo"), const std::system_error&);
}

TEST_CASE("Write uncompressed file with explicit close") {
    const int count = count_fds();

    const std::string output_file = "test_uncompressed_out.txt";
    const int fd = osmium::io::detail::open_for_writing(output_file, osmium::io::overwrite::allow);
    REQUIRE(fd > 0);

    osmium::io::NoCompressor comp{fd, osmium::io::fsync::yes};
    comp.write("foo");
    comp.close();

    REQUIRE(count == count_fds());

    REQUIRE(osmium::file_size(output_file) == 3);
}

TEST_CASE("Write uncompressed file with implicit close") {
    const int count = count_fds();

    const std::string output_file = "test_uncompressed_out.txt";
    const int fd = osmium::io::detail::open_for_writing(output_file, osmium::io::overwrite::allow);
    REQUIRE(fd > 0);

    {
        osmium::io::NoCompressor comp{fd, osmium::io::fsync::yes};
        comp.write("foo");
    }
    REQUIRE(count == count_fds());

    REQUIRE(osmium::file_size(output_file) == 3);
}

