#include "catch.hpp"

#include <osmium/io/compression.hpp>

TEST_CASE("Create compressor using factory") {
    const auto& factory = osmium::io::CompressionFactory::instance();
    REQUIRE(factory.create_compressor(osmium::io::file_compression::none, -1, osmium::io::fsync::no));
}

TEST_CASE("Create decompressor using factory") {
    const auto& factory = osmium::io::CompressionFactory::instance();
    REQUIRE(factory.create_decompressor(osmium::io::file_compression::none, nullptr, 0));
}

TEST_CASE("Compression factory fails on undefined compression") {
    const auto& factory = osmium::io::CompressionFactory::instance();
    REQUIRE_THROWS_AS(factory.create_compressor(osmium::io::file_compression::gzip, -1, osmium::io::fsync::no),
                      const osmium::unsupported_file_format_error&);
    REQUIRE_THROWS_WITH(factory.create_compressor(osmium::io::file_compression::gzip, -1, osmium::io::fsync::no),
                        "Support for compression 'gzip' not compiled into this binary");
}

