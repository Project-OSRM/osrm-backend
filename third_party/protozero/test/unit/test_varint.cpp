
#include <test.hpp>

TEST_CASE("max varint length") {
    REQUIRE(protozero::max_varint_length == 10);
}

TEST_CASE("varint") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};

    SECTION("encode/decode int32") {
        pw.add_int32(1, 17);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());

        SECTION("get") {
            REQUIRE(17 == item.get_int32());
        }

        SECTION("skip") {
            item.skip();
        }

        REQUIRE_FALSE(item.next());
    }

    SECTION("encode/decode uint32") {
        pw.add_uint32(1, 17U);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());

        SECTION("get") {
            REQUIRE(17U == item.get_uint32());
        }

        SECTION("skip") {
            item.skip();
        }

        REQUIRE_FALSE(item.next());
    }

    SECTION("encode/decode uint64") {
        pw.add_uint64(1, (1ULL << 40U));
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());

        SECTION("get") {
            REQUIRE((1ULL << 40U) == item.get_uint64());
        }

        SECTION("skip") {
            item.skip();
        }

        REQUIRE_FALSE(item.next());
    }

    SECTION("short buffer while parsing varint") {
        pw.add_uint64(1, (1ULL << 40U));
        buffer.resize(buffer.size() - 1); // "remove" last byte from buffer
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());

        SECTION("get") {
            REQUIRE_THROWS_AS(item.get_uint64(), protozero::end_of_buffer_exception);
        }

        SECTION("skip") {
            REQUIRE_THROWS_AS(item.skip(), protozero::end_of_buffer_exception);
        }
    }

    SECTION("data corruption in buffer while parsing varint)") {
        pw.add_uint64(1, (1ULL << 20U));
        buffer[buffer.size() - 1] += static_cast<char>(0x80); // pretend the varint goes on
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());

        SECTION("get") {
            REQUIRE_THROWS_AS(item.get_uint64(), protozero::end_of_buffer_exception);
        }

        SECTION("skip") {
            REQUIRE_THROWS_AS(item.skip(), protozero::end_of_buffer_exception);
        }
    }

    SECTION("data corruption in buffer while parsing varint (max length varint)") {
        pw.add_uint64(1, std::numeric_limits<uint64_t>::max());
        buffer[buffer.size() - 1] += static_cast<char>(0x80); // pretend the varint goes on
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());

        SECTION("get") {
            REQUIRE_THROWS_AS(item.get_uint64(), protozero::varint_too_long_exception);
        }

        SECTION("skip") {
            REQUIRE_THROWS_AS(item.skip(), protozero::varint_too_long_exception);
        }
    }
}

TEST_CASE("10-byte varint") {
    std::string buffer;
    protozero::pbf_writer pw{buffer};
    pw.add_uint64(1, 1);
    buffer.back() = static_cast<char>(0xffU);
    for (int i = 0; i < 9; ++i) {
        buffer.push_back(static_cast<char>(0xffU));
    }
    buffer.push_back(0x02);

    protozero::pbf_reader item{buffer};
    REQUIRE(item.next());
    REQUIRE_THROWS_AS(item.get_uint64(), protozero::varint_too_long_exception);
}

TEST_CASE("lots of varints back and forth") {
    std::string buffer;

    for (uint32_t n = 0; n < 70000; ++n) {
        protozero::pbf_writer pw{buffer};
        pw.add_uint32(1, n);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());
        REQUIRE(n == item.get_uint32());
        REQUIRE_FALSE(item.next());
        buffer.clear();
    }

    for (int32_t n = -70000; n < 70000; ++n) {
        protozero::pbf_writer pw{buffer};
        pw.add_int32(1, n);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());
        REQUIRE(n == item.get_int32());
        REQUIRE_FALSE(item.next());
        buffer.clear();
    }

    for (int32_t n = -70000; n < 70000; ++n) {
        protozero::pbf_writer pw{buffer};
        pw.add_sint32(1, n);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());
        REQUIRE(n == item.get_sint32());
        REQUIRE_FALSE(item.next());
        buffer.clear();
    }

    for (uint32_t i = 0; i < 63; ++i) {
        const auto n = static_cast<int64_t>(1ULL << i);
        protozero::pbf_writer pw{buffer};
        pw.add_int64(1, n);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());
        REQUIRE(n == item.get_int64());
        REQUIRE_FALSE(item.next());
        buffer.clear();
    }

    for (uint32_t i = 0; i < 63; ++i) {
        const int64_t n = - static_cast<int64_t>(1ULL << i);
        protozero::pbf_writer pw{buffer};
        pw.add_int64(1, n);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());
        REQUIRE(n == item.get_int64());
        REQUIRE_FALSE(item.next());
        buffer.clear();
    }

    for (uint32_t i = 0; i < 64; ++i) {
        const uint64_t n = 1ULL << i;
        protozero::pbf_writer pw{buffer};
        pw.add_uint64(1, n);
        protozero::pbf_reader item{buffer};
        REQUIRE(item.next());
        REQUIRE(n == item.get_uint64());
        REQUIRE_FALSE(item.next());
        buffer.clear();
    }
}

TEST_CASE("skip_varint with empty buffer throws") {
    const char* buffer = "";
    REQUIRE_THROWS_AS(protozero::skip_varint(&buffer, buffer), protozero::end_of_buffer_exception);
}

TEST_CASE("call skip_varint with every possible value for single byte in buffer") {
    char buffer[1];
    for (int i = 0; i <= 127; ++i) {
        buffer[0] = static_cast<char>(i);
        const char* b = buffer;
        protozero::skip_varint(&b, buffer + 1);
        REQUIRE(b == buffer + 1);
    }
    for (int i = 128; i <= 255; ++i) {
        buffer[0] = static_cast<char>(i);
        const char* b = buffer;
        REQUIRE_THROWS_AS(protozero::skip_varint(&b, buffer + 1), protozero::end_of_buffer_exception);
    }
}

TEST_CASE("decode_varint with empty buffer throws") {
    const char* buffer = "";
    REQUIRE_THROWS_AS(protozero::decode_varint(&buffer, buffer), protozero::end_of_buffer_exception);
}

TEST_CASE("call decode_varint with every possible value for single byte in buffer") {
    char buffer[1];
    for (unsigned int i = 0; i <= 127; ++i) {
        REQUIRE(protozero::length_of_varint(i) == 1);
        buffer[0] = static_cast<char>(i);
        const char* b = buffer;
        REQUIRE(protozero::decode_varint(&b, buffer + 1) == i);
        REQUIRE(b == buffer + 1);
    }
    for (unsigned int i = 128; i <= 255; ++i) {
        REQUIRE(protozero::length_of_varint(i) == 2);
        buffer[0] = static_cast<char>(i);
        const char* b = buffer;
        REQUIRE_THROWS_AS(protozero::decode_varint(&b, buffer + 1), protozero::end_of_buffer_exception);
    }
}

TEST_CASE("check lengths of varint") {
    REQUIRE(protozero::length_of_varint(0) == 1);
    REQUIRE(protozero::length_of_varint(127) == 1);
    REQUIRE(protozero::length_of_varint(128) == 2);
    REQUIRE(protozero::length_of_varint(16383) == 2);
    REQUIRE(protozero::length_of_varint(16384) == 3);
    REQUIRE(protozero::length_of_varint(2097151) == 3);
    REQUIRE(protozero::length_of_varint(2097152) == 4);
    REQUIRE(protozero::length_of_varint(0xffffffffULL) == 5);
    REQUIRE(protozero::length_of_varint(0xffffffffffffffffULL) == 10);
}

