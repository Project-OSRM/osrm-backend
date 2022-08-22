
#include <buffer.hpp>

#include "t/enum/enum_testcase.pb.h"

TEMPLATE_TEST_CASE("write enum field and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestEnum::Test msg;

    SECTION("zero") {
        pw.add_enum(1, 0L);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.color() == TestEnum::Color::BLACK);
    }

    SECTION("positive") {
        pw.add_enum(1, 3L);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.color() == TestEnum::Color::BLUE);
    }

    SECTION("negative") {
        pw.add_enum(1, -1L);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.color() == TestEnum::Color::NEG);
    }

    SECTION("max") {
        pw.add_enum(1, std::numeric_limits<int32_t>::max() - 1);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.color() == TestEnum::Color::MAX);
    }

    SECTION("min") {
        pw.add_enum(1, std::numeric_limits<int32_t>::min() + 1);

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.color() == TestEnum::Color::MIN);
    }

}

