
#include <string>

#include <test.hpp> // IWYU pragma: keep

#include "t/bool/bool_testcase.pb.h"

TEST_CASE("write bool field and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestBoolean::Test msg;

    SECTION("false") {
        pw.add_bool(1, false);

        msg.ParseFromString(buffer);

        REQUIRE_FALSE(msg.b());
    }

    SECTION("true") {
        pw.add_bool(1, true);

        msg.ParseFromString(buffer);

        REQUIRE(msg.b());
    }

}

