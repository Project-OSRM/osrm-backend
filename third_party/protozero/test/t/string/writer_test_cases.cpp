
#include <buffer.hpp>

#include "t/string/string_testcase.pb.h"

TEMPLATE_TEST_CASE("write string field and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestString::Test msg;

    SECTION("empty") {
        pw.add_string(1, "");

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.s().empty());
    }

    SECTION("one") {
        pw.add_string(1, "x");

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.s() == "x");
    }

    SECTION("string") {
        pw.add_string(1, "foobar");

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.s() == "foobar");
    }

}

