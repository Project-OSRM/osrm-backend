
#include <buffer.hpp>

#include "t/double/double_testcase.pb.h"

TEMPLATE_TEST_CASE("write double field and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestDouble::Test msg;

    SECTION("zero") {
        pw.add_double(1, 0.0);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.x() == Approx(0.0));
    }

    SECTION("positive") {
        pw.add_double(1, 4.893);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.x() == Approx(4.893));
    }

    SECTION("negative") {
        pw.add_double(1, -9232.33);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.x() == Approx(-9232.33));
    }

}

