
#include <test.hpp>

#include "t/nested/nested_testcase.pb.h"

TEST_CASE("write nested message fields and check with libprotobuf") {

    std::string buffer_test;
    protozero::pbf_writer pbf_test{buffer_test};

    SECTION("string") {
        std::string buffer_subsub;
        protozero::pbf_writer pbf_subsub{buffer_subsub};
        pbf_subsub.add_string(1, "foobar");
        pbf_subsub.add_int32(2, 99);

        std::string buffer_sub;
        protozero::pbf_writer pbf_sub{buffer_sub};
        pbf_sub.add_string(1, buffer_subsub);
        pbf_sub.add_int32(2, 88);

        pbf_test.add_message(1, buffer_sub);
    }

    SECTION("with subwriter") {
        protozero::pbf_writer pbf_sub{pbf_test, 1};
        {
            protozero::pbf_writer pbf_subsub(pbf_sub, 1);
            pbf_subsub.add_string(1, "foobar");
            pbf_subsub.add_int32(2, 99);
        }
        pbf_sub.add_int32(2, 88);
    }

    pbf_test.add_int32(2, 77);

    TestNested::Test msg;
    msg.ParseFromString(buffer_test);

    REQUIRE(msg.i() == 77);
    REQUIRE(msg.sub().i() == 88);
    REQUIRE(msg.sub().subsub().i() == 99);
    REQUIRE(msg.sub().subsub().s() == "foobar");

}

