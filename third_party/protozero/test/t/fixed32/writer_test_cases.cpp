
#include <test.hpp>

#include "t/fixed32/fixed32_testcase.pb.h"

TEST_CASE("write fixed32 field and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestFixed32::Test msg;

    SECTION("zero") {
        pw.add_fixed32(1, 0);

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == 0);
    }

    SECTION("max") {
        pw.add_fixed32(1, std::numeric_limits<uint32_t>::max());

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == std::numeric_limits<uint32_t>::max());
    }

    SECTION("min") {
        pw.add_fixed32(1, std::numeric_limits<uint32_t>::min());

        msg.ParseFromString(buffer);

        REQUIRE(msg.i() == std::numeric_limits<uint32_t>::min());
    }

}

