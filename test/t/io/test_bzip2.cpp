#include "catch.hpp"

#include "utils.hpp"

#include <osmium/io/bzip2_compression.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

TEST_CASE("Read bzip2-compressed file") {
    const std::string input_file = with_data_dir("t/io/data_bzip2.txt.bz2");

    const int fd = ::open(input_file.c_str(), O_RDONLY);
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

    REQUIRE(9 == size);
    REQUIRE("TESTDATA\n" == all);
}

