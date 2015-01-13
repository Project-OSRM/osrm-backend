#include "catch.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <osmium/io/bzip2_compression.hpp>

TEST_CASE("Bzip2") {

SECTION("read_compressed_file") {
    int fd = ::open("t/io/data_bzip2.txt.bz2", O_RDONLY);
    REQUIRE(fd > 0);

    size_t size = 0;
    std::string all;
    {
        osmium::io::Bzip2Decompressor decomp(fd);
        for (std::string data = decomp.read(); !data.empty(); data = decomp.read()) {
            size += data.size();
            all += data;
        }
    }

    REQUIRE(9 == size);
    REQUIRE("TESTDATA\n" == all);
}

}

