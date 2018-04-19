
#include <test.hpp>

#include "t/message/message_testcase.pb.h"

TEST_CASE("write message field and check with libprotobuf") {

    std::string buffer_test;
    protozero::pbf_writer pbf_test{buffer_test};

    SECTION("string") {
        std::string buffer_submessage;
        protozero::pbf_writer pbf_submessage{buffer_submessage};
        pbf_submessage.add_string(1, "foobar");

        pbf_test.add_message(1, buffer_submessage);
    }

    SECTION("string with subwriter") {
        protozero::pbf_writer pbf_submessage{pbf_test, 1};
        pbf_submessage.add_string(1, "foobar");
    }

    TestMessage::Test msg;
    msg.ParseFromString(buffer_test);
    REQUIRE(msg.submessage().s() == "foobar");

}

