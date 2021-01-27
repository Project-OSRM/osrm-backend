
#include <buffer.hpp>

#include "t/repeated/repeated_testcase.pb.h"

TEMPLATE_TEST_CASE("write repeated fields and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestRepeated::Test msg;

    SECTION("one") {
        pw.add_int32(1, 0L);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i().size() == 1);
        REQUIRE(msg.i(0) == 0L);
    }

    SECTION("many") {
        pw.add_int32(1, 0L);
        pw.add_int32(1, 1L);
        pw.add_int32(1, -1L);
        pw.add_int32(1, std::numeric_limits<int32_t>::max());
        pw.add_int32(1, std::numeric_limits<int32_t>::min());

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i().size() == 5);
        REQUIRE(msg.i(0) == 0L);
        REQUIRE(msg.i(1) == 1L);
        REQUIRE(msg.i(2) == -1L);
        REQUIRE(msg.i(3) == std::numeric_limits<int32_t>::max());
        REQUIRE(msg.i(4) == std::numeric_limits<int32_t>::min());
    }

}

