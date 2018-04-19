
#include <test.hpp>

#include "t/int32/int32_testcase.pb.h"

TEST_CASE("write int32 field and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestInt32::Test msg;

    SECTION("zero") {
        pw.add_int32(1, 0L);

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == 0L);
    }

    SECTION("positive") {
        pw.add_int32(1, 1L);

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == 1L);
    }

    SECTION("negative") {
        pw.add_int32(1, -1L);

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == -1L);
    }

    SECTION("max") {
        pw.add_int32(1, std::numeric_limits<int32_t>::max());

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == std::numeric_limits<int32_t>::max());
    }

    SECTION("min") {
        pw.add_int32(1, std::numeric_limits<int32_t>::min());

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == std::numeric_limits<int32_t>::min());
    }

}

