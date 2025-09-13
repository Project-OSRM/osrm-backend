
#include <buffer.hpp>

#include "t/fixed32/fixed32_testcase.pb.h"

TEMPLATE_TEST_CASE("write fixed32 field and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestFixed32::Test msg;

    SECTION("zero") {
        pw.add_fixed32(1, 0);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == 0);
    }

    SECTION("max") {
        pw.add_fixed32(1, std::numeric_limits<uint32_t>::max());

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == std::numeric_limits<uint32_t>::max());
    }

    SECTION("min") {
        pw.add_fixed32(1, std::numeric_limits<uint32_t>::min());

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i() == std::numeric_limits<uint32_t>::min());
    }

}

