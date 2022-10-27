#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/reader.hpp>

TEST_CASE("Reader throws on unsupported file format") {
    REQUIRE_THROWS_AS(osmium::io::Reader{with_data_dir("t/io/data.osm")}, osmium::unsupported_file_format_error);
}

