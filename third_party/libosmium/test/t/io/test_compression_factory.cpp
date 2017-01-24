
#include "catch.hpp"

#include <osmium/io/compression.hpp>

TEST_CASE("Compression factory") {
    const auto& factory = osmium::io::CompressionFactory::instance();

    SECTION("compressor") {
        REQUIRE(factory.create_compressor(osmium::io::file_compression::none, -1, osmium::io::fsync::no));
    }

    SECTION("decompressor") {
        REQUIRE(factory.create_decompressor(osmium::io::file_compression::none, nullptr, 0));
    }

    SECTION("fail on undefined compression") {
        REQUIRE_THROWS_AS({
            factory.create_compressor(osmium::io::file_compression::gzip, -1, osmium::io::fsync::no);
        }, osmium::unsupported_file_format_error);
        REQUIRE_THROWS_WITH({
            factory.create_compressor(osmium::io::file_compression::gzip, -1, osmium::io::fsync::no);
        }, "Support for compression 'gzip' not compiled into this binary");
    }

}

