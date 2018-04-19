
#include <test.hpp>

#include "t/double/double_testcase.pb.h"

TEST_CASE("write double field and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestDouble::Test msg;

    SECTION("zero") {
        pw.add_double(1, 0.0);

        msg.ParseFromString(buffer);

        REQUIRE(msg.x() == Approx(0.0));
    }

    SECTION("positive") {
        pw.add_double(1, 4.893);

        msg.ParseFromString(buffer);

        REQUIRE(msg.x() == Approx(4.893));
    }

    SECTION("negative") {
        pw.add_double(1, -9232.33);

        msg.ParseFromString(buffer);

        REQUIRE(msg.x() == Approx(-9232.33));
    }

}

