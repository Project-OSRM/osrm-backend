
#include <test.hpp>

#include "t/bytes/bytes_testcase.pb.h"

TEST_CASE("write bytes field and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestBytes::Test msg;

    SECTION("empty") {
        pw.add_string(1, "");

        msg.ParseFromString(buffer);

        REQUIRE(msg.s().empty());
    }

    SECTION("one") {
        pw.add_string(1, "x");

        msg.ParseFromString(buffer);

        REQUIRE(msg.s() == "x");
    }

    SECTION("string") {
        pw.add_string(1, "foobar");

        msg.ParseFromString(buffer);

        REQUIRE(msg.s() == "foobar");
    }

    SECTION("binary") {
        std::string data;
        data.append(1, char(1));
        data.append(1, char(2));
        data.append(1, char(3));

        pw.add_string(1, data);

        msg.ParseFromString(buffer);

        REQUIRE(msg.s().size() == 3);
        REQUIRE(msg.s()[1] == char(2));
        REQUIRE(msg.s()[2] == char(3));
        REQUIRE(msg.s()[2] == char(3));
    }

}

