
#include <test.hpp>

#include "t/repeated_packed_fixed32/repeated_packed_fixed32_testcase.pb.h"

TEST_CASE("write repeated packed fixed32 field and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestRepeatedPackedFixed32::Test msg;

    SECTION("empty") {
        uint32_t data[] = { 17UL };
        pw.add_packed_fixed32(1, std::begin(data), std::begin(data) /* !!!! */);
    }

    SECTION("one") {
        uint32_t data[] = { 17UL };
        pw.add_packed_fixed32(1, std::begin(data), std::end(data));

        msg.ParseFromString(buffer);

        REQUIRE(msg.i().size() == 1);
        REQUIRE(msg.i(0) == 17UL);
    }

    SECTION("many") {
        uint32_t data[] = { 17UL, 0UL, 1UL, std::numeric_limits<uint32_t>::max() };
        pw.add_packed_fixed32(1, std::begin(data), std::end(data));

        msg.ParseFromString(buffer);

        REQUIRE(msg.i().size() == 4);
        REQUIRE(msg.i(0) == 17UL);
        REQUIRE(msg.i(1) ==  0UL);
        REQUIRE(msg.i(2) ==  1UL);
        REQUIRE(msg.i(3) == std::numeric_limits<uint32_t>::max());
    }
}

TEST_CASE("write from different types of iterators and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestRepeatedPackedFixed32::Test msg;

    SECTION("from uint16_t") {
        uint16_t data[] = { 1, 4, 9, 16, 25 };

        pw.add_packed_fixed32(1, std::begin(data), std::end(data));
    }

    SECTION("from string") {
        std::string data = "1 4 9 16 25";
        std::stringstream sdata(data);

        std::istream_iterator<uint32_t> eod;
        std::istream_iterator<uint32_t> it(sdata);

        pw.add_packed_fixed32(1, it, eod);
    }

    msg.ParseFromString(buffer);

    REQUIRE(msg.i().size() == 5);
    REQUIRE(msg.i(0) ==  1);
    REQUIRE(msg.i(1) ==  4);
    REQUIRE(msg.i(2) ==  9);
    REQUIRE(msg.i(3) == 16);
    REQUIRE(msg.i(4) == 25);
}

