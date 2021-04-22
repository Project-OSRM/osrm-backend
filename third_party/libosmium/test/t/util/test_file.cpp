#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/file.hpp>

#include <stdexcept>
#include <string>

TEST_CASE("file_size(int) and file_offset() of known file") {
    std::string file_name{with_data_dir("t/util/known_file_size")};
    const int fd = osmium::io::detail::open_for_reading(file_name);
    REQUIRE(fd > 0);
    REQUIRE(osmium::file_size(fd) == 22);
    REQUIRE(osmium::file_offset(fd) == 0);
    REQUIRE_FALSE(osmium::isatty(fd));
}

TEST_CASE("file_size(std::string) of known file") {
    std::string file_name{with_data_dir("t/util/known_file_size")};
    REQUIRE(osmium::file_size(file_name) == 22);
}

TEST_CASE("file_size(const char*) of known file") {
    std::string file_name{with_data_dir("t/util/known_file_size")};
    REQUIRE(osmium::file_size(file_name.c_str()) == 22);
}

TEST_CASE("file_size() with illegal fd should throw") {
    REQUIRE_THROWS_AS(osmium::file_size(-1), const std::system_error&);
}

TEST_CASE("file_size() with unused fd should throw") {
    // its unlikely that fd 1000 is open...
    REQUIRE_THROWS_AS(osmium::file_size(1000), const std::system_error&);
}

TEST_CASE("file_size() of unknown file should throw") {
    REQUIRE_THROWS_AS(osmium::file_size("unknown file"), const std::system_error&);
}

TEST_CASE("resize_file() with illegal fd should throw") {
    REQUIRE_THROWS_AS(osmium::resize_file(-1, 10), const std::system_error&);
}

TEST_CASE("resize_file() with unused fd should throw") {
    // its unlikely that fd 1000 is open...
    REQUIRE_THROWS_AS(osmium::resize_file(1000, 10), const std::system_error&);
}

TEST_CASE("get_pagesize()") {
    REQUIRE(osmium::get_pagesize() > 0);
}

