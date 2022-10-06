
#include <buffer.hpp>

#include "t/int32/int32_testcase.pb.h"

TEMPLATE_TEST_CASE("write int32 field and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestInt32::Test msg;

    SECTION("zero") {
        pw.add_int32(1, 0L);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == 0L);
    }

    SECTION("positive") {
        pw.add_int32(1, 1L);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == 1L);
    }

    SECTION("negative") {
        pw.add_int32(1, -1L);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == -1L);
    }

    SECTION("max") {
        pw.add_int32(1, std::numeric_limits<int32_t>::max());

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == std::numeric_limits<int32_t>::max());
    }

    SECTION("min") {
        pw.add_int32(1, std::numeric_limits<int32_t>::min());

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == std::numeric_limits<int32_t>::min());
    }

}

