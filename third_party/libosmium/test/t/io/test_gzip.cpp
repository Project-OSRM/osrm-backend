#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/io/gzip_compression.hpp>

#include <string>

TEST_CASE("Invalid file descriptor of gzip-compressed file") {
    REQUIRE_THROWS_AS(osmium::io::GzipDecompressor{-1}, const osmium::gzip_error&);
}

TEST_CASE("Non-open file descriptor of gzip-compressed file") {
    // 12345 is just a random file descriptor that should not be open
    osmium::io::GzipDecompressor decomp{12345};
    REQUIRE_THROWS_AS(decomp.read(), const osmium::gzip_error&);
}

TEST_CASE("Empty gzip-compressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/empty_file");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    osmium::io::GzipDecompressor decomp{fd};
    REQUIRE(decomp.read().empty());
    decomp.close();

    REQUIRE(count == count_fds());
}

TEST_CASE("Read gzip-compressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/data_gzip.txt.gz");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    size_t size = 0;
    std::string all;
    {
        osmium::io::GzipDecompressor decomp{fd};
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

TEST_CASE("Read gzip-compressed file without explicit close") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/data_gzip.txt.gz");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    size_t size = 0;
    std::string all;
    {
        osmium::io::GzipDecompressor decomp{fd};
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

TEST_CASE("Corrupted gzip-compressed file") {
    const int count = count_fds();

    const std::string input_file = with_data_dir("t/io/corrupt_data_gzip.txt.gz");
    const int fd = osmium::io::detail::open_for_reading(input_file);
    REQUIRE(fd > 0);

    osmium::io::GzipDecompressor decomp{fd};
    decomp.read();
    REQUIRE_THROWS_AS(decomp.close(), const osmium::gzip_error&);

    REQUIRE(count == count_fds());
}

TEST_CASE("Compressor: Invalid file descriptor for gzip-compressed file") {
    REQUIRE_THROWS_AS(osmium::io::GzipCompressor(-1, osmium::io::fsync::yes), const std::system_error&);
}

TEST_CASE("Compressor: Non-open file descriptor for gzip-compressed file") {
    // 12345 is just a random file descriptor that should not be open
    REQUIRE_THROWS_AS(osmium::io::GzipCompressor(12345, osmium::io::fsync::yes), const std::system_error&);
}

TEST_CASE("Write gzip-compressed file with explicit close") {
    const int count = count_fds();

    const std::string output_file = "test_gzip_out.txt.gz";
    const int fd = osmium::io::detail::open_for_writing(output_file, osmium::io::overwrite::allow);
    REQUIRE(fd > 0);

    osmium::io::GzipCompressor comp{fd, osmium::io::fsync::yes};
    comp.write("foo");
    comp.close();

    REQUIRE(count == count_fds());

    REQUIRE(osmium::file_size(output_file) > 10);
}

TEST_CASE("Write gzip-compressed file with implicit close") {
    const int count = count_fds();

    const std::string output_file = "test_gzip_out.txt.gz";
    const int fd = osmium::io::detail::open_for_writing(output_file, osmium::io::overwrite::allow);
    REQUIRE(fd > 0);

    {
        osmium::io::GzipCompressor comp{fd, osmium::io::fsync::yes};
        comp.write("foo");
    }
    REQUIRE(count == count_fds());

    REQUIRE(osmium::file_size(output_file) > 10);
}

