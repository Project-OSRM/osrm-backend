#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/file.hpp>

/**
 * Can read and seek around in files.
 */
TEST_CASE("Seek and read in files") {
    /* gzipped data contains very few repetitions in the binary file format,
     * which makes it easy to identify any problems. */
    const int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.gz"));
    struct seek_expectation {
        size_t offset;
        unsigned char eight_bytes[8];
    };
    const seek_expectation expectations[] = {
        { 0x00, {0x1f, 0x8b, 0x08, 0x08, 0x19, 0x4a, 0x18, 0x54} },
        { 0x00, {0x1f, 0x8b, 0x08, 0x08, 0x19, 0x4a, 0x18, 0x54} }, /* repeat / jump back */
        { 0x21, {0x56, 0xc6, 0x18, 0xc3, 0xea, 0x6d, 0x4f, 0xe0} }, /* unaligned */
        { 0xb3, {0xcd, 0x0a, 0xe7, 0x8f, 0xde, 0x00, 0x00, 0x00} }, /* close to end */
        { 0x21, {0x56, 0xc6, 0x18, 0xc3, 0xea, 0x6d, 0x4f, 0xe0} }, /* "long" backward jump */
    };
    for (const auto& expect : expectations) {
        char actual_eight_bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        osmium::util::file_seek(fd, expect.offset);
        const bool did_actually_read = osmium::io::detail::read_exactly(fd, &actual_eight_bytes[0], 8);
        REQUIRE(did_actually_read);
        for (int i = 0; i < 8; ++i) {
            REQUIRE(expect.eight_bytes[i] == static_cast<unsigned char>(actual_eight_bytes[i]));
        }
    }
}

TEST_CASE("Seek close to end of file") {
    /* gzipped data contains very few repetitions in the binary file format,
     * which makes it easy to identify any problems. */
    const int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.gz"));
    REQUIRE(osmium::util::file_size(with_data_dir("t/io/data.osm.gz")) == 187);
    char actual_eight_bytes[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    osmium::util::file_seek(fd, 186);
    const auto actually_read = osmium::io::detail::reliable_read(fd, &actual_eight_bytes[0], 8);
    REQUIRE(actually_read == 1);
    REQUIRE(actual_eight_bytes[0] == 0);
    REQUIRE(actual_eight_bytes[1] == 1);
}

TEST_CASE("Seek to exact end of file") {
    /* gzipped data contains very few repetitions in the binary file format,
     * which makes it easy to identify any problems. */
    const int fd = osmium::io::detail::open_for_reading(with_data_dir("t/io/data.osm.gz"));
    REQUIRE(osmium::util::file_size(with_data_dir("t/io/data.osm.gz")) == 187);
    char actual_eight_bytes[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    osmium::util::file_seek(fd, 187);
    const auto actually_read = osmium::io::detail::reliable_read(fd, &actual_eight_bytes[0], 8);
    REQUIRE(actually_read == 0);
    REQUIRE(actual_eight_bytes[0] == 1);
    REQUIRE(actual_eight_bytes[1] == 1);
}
