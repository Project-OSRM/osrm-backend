
#include <buffer.hpp>

#include <array>
#include <sstream>

#include "t/repeated_packed_fixed32/repeated_packed_fixed32_testcase.pb.h"

TEMPLATE_TEST_CASE("write repeated packed fixed32 field and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestRepeatedPackedFixed32::Test msg;

    SECTION("empty") {
        const std::array<uint32_t, 1> data = {{ 17UL }};
        pw.add_packed_fixed32(1, std::begin(data), std::begin(data) /* !!!! */);
    }

    SECTION("one") {
        const std::array<uint32_t, 1> data = {{ 17UL }};
        pw.add_packed_fixed32(1, std::begin(data), std::end(data));

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i().size() == 1);
        REQUIRE(msg.i(0) == 17UL);
    }

    SECTION("many") {
        const std::array<uint32_t, 4> data = {{ 17UL, 0UL, 1UL, std::numeric_limits<uint32_t>::max() }};
        pw.add_packed_fixed32(1, std::begin(data), std::end(data));

        msg.ParseFromArray(buffer.data(), buffer.size());

        REQUIRE(msg.i().size() == 4);
        REQUIRE(msg.i(0) == 17UL);
        REQUIRE(msg.i(1) ==  0UL);
        REQUIRE(msg.i(2) ==  1UL);
        REQUIRE(msg.i(3) == std::numeric_limits<uint32_t>::max());
    }
}

TEMPLATE_TEST_CASE("write from different types of iterators and check with libprotobuf", "",
    buffer_test_string, buffer_test_vector, buffer_test_array, buffer_test_external) {

    TestType buffer;
    typename TestType::writer_type pw{buffer.buffer()};

    TestRepeatedPackedFixed32::Test msg;

    SECTION("from uint16_t") {
        const std::array<uint16_t, 5> data = {{ 1, 4, 9, 16, 25 }};

        pw.template add_packed_fixed<uint32_t>(1, std::begin(data), std::end(data));
    }

    SECTION("from string") {
        const std::string data = "1 4 9 16 25";
        std::stringstream sdata(data);

        const std::istream_iterator<uint32_t> eod;
        const std::istream_iterator<uint32_t> it(sdata);

        pw.template add_packed_fixed<uint32_t>(1, it, eod);
    }

    msg.ParseFromArray(buffer.data(), buffer.size());

    REQUIRE(msg.i().size() == 5);
    REQUIRE(msg.i(0) ==  1);
    REQUIRE(msg.i(1) ==  4);
    REQUIRE(msg.i(2) ==  9);
    REQUIRE(msg.i(3) == 16);
    REQUIRE(msg.i(4) == 25);
}

