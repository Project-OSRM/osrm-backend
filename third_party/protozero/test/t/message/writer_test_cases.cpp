
#include <buffer.hpp>

#include "t/message/message_testcase.pb.h"

TEMPLATE_TEST_CASE("write message field and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    SECTION("string") {
        std::string buffer_submessage;
        protozero::pbf_writer pbf_submessage{buffer_submessage};
        pbf_submessage.add_string(1, "foobar");

        pw.add_message(1, buffer_submessage);
    }

    SECTION("string with subwriter") {
        typename TestType::writer_type pbf_submessage{pw, 1};
        pbf_submessage.add_string(1, "foobar");
    }

    TestMessage::Test msg;
    msg.ParseFromArray(buffer.data(), buffer.size());
    REQUIRE(msg.submessage().s() == "foobar");

}

