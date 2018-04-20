
#include <test.hpp>

#include "t/enum/enum_testcase.pb.h"

TEST_CASE("write enum field and check with libprotobuf") {

    std::string buffer;
    protozero::pbf_writer pw{buffer};

    TestEnum::Test msg;

    SECTION("zero") {
        pw.add_enum(1, 0L);

        msg.ParseFromString(buffer);

        REQUIRE(msg.color() == TestEnum::Color::BLACK);
    }

    SECTION("positive") {
        pw.add_enum(1, 3L);

        msg.ParseFromString(buffer);

        REQUIRE(msg.color() == TestEnum::Color::BLUE);
    }

    SECTION("negative") {
        pw.add_enum(1, -1L);

        msg.ParseFromString(buffer);

        REQUIRE(msg.color() == TestEnum::Color::NEG);
    }

    SECTION("max") {
        pw.add_enum(1, std::numeric_limits<int32_t>::max() - 1);

        msg.ParseFromString(buffer);

        REQUIRE(msg.color() == TestEnum::Color::MAX);
    }

    SECTION("min") {
        pw.add_enum(1, std::numeric_limits<int32_t>::min() + 1);

        msg.ParseFromString(buffer);

        REQUIRE(msg.color() == TestEnum::Color::MIN);
    }

}

