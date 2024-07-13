#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/bzip2_compression.hpp>
#include <osmium/io/detail/read_write.hpp>

#include <string>

static void read_from_decompressor(int fd) {
    osmium::io::Bzip2Decompressor decomp{fd};
    decomp.read();
    decomp.close();
}

TEST_CASE("Invalid file descriptor of bzip2-compressed file") {
    REQUIRE_THROWS(read_from_decompressor(-1));
}

TEST_CASE("Non-open file descriptor of bzip2-compressed file") {
    // 12345 is just a random file descriptor that should not be open
    REQUIRE_THROWS(read_from_decompressor(12345));
}

TEST_CASE("Empty bzip2-compressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/empty_file");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    osmium::io::Bzip2Decompressor decomp{fd};
    REQUIRE_THROWS_AS(decomp.read(), osmium::bzip2_error);
    decomp.close();

    REQUIRE(count == count_fds());
}

TEST_CASE("Read bzip2-compressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/data_bzip2.txt.bz2");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    size_t size = 0;
    std::string all;
    {
        osmium::io::Bzip2Decompressor decomp{fd};
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

TEST_CASE("Read bzip2-compressed file without explicit close") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/data_bzip2.txt.bz2");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    size_t size = 0;
    std::string all;
    {
        osmium::io::Bzip2Decompressor decomp{fd};
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

TEST_CASE("Corrupted bzip2-compressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/corrupt_data_bzip2.txt.bz2");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    osmium::io::Bzip2Decompressor decomp{fd};
    REQUIRE_THROWS_AS(decomp.read(), osmium::bzip2_error);
    decomp.close();

    REQUIRE(count == count_fds());
}

static void write_to_compressor(int fd) {
    osmium::io::Bzip2Compressor comp{fd, osmium::io::fsync::yes};
    comp.write("foo");
    comp.close();
}

TEST_CASE("Compressor: Invalid file descriptor for bzip2-compressed file") {
    REQUIRE_THROWS(write_to_compressor(-1));
}

TEST_CASE("Compressor: Non-open file descriptor for bzip2-compressed file") {
    // 12345 is just a random file descriptor that should not be open
    REQUIRE_THROWS(write_to_compressor(12345));
}

TEST_CASE("Write bzip2-compressed file with explicit close") {
    const int count = count_fds();

    const std::string output_file = "test_gzip_out.txt.bz2";
    const int fd = osmium::io::detail::open_for_writing(output_file, osmium::io::overwrite::allow);
    REQUIRE(fd > 0);

    osmium::io::Bzip2Compressor comp{fd, osmium::io::fsync::yes};
    comp.write("foo");
    comp.close();

    REQUIRE(count == count_fds());

    REQUIRE(osmium::file_size(output_file) > 10);
}

TEST_CASE("Write bzip2-compressed file with implicit close") {
    const int count = count_fds();

    const std::string output_file = "test_gzip_out.txt.bz2";
    const int fd = osmium::io::detail::open_for_writing(output_file, osmium::io::overwrite::allow);
    REQUIRE(fd > 0);

    {
        osmium::io::Bzip2Compressor comp{fd, osmium::io::fsync::yes};
        comp.write("foo");
    }
    REQUIRE(count == count_fds());

    REQUIRE(osmium::file_size(output_file) > 10);
}

