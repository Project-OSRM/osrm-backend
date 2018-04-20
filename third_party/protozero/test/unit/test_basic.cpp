
#include <test.hpp>

TEST_CASE("default constructed pbf_reader is okay") {
    protozero::pbf_reader item;

    REQUIRE(item.length() == 0);
    REQUIRE_FALSE(item); // test operator bool()
    REQUIRE_FALSE(item.next());
}

TEST_CASE("empty buffer in pbf_reader is okay") {
    const std::string buffer;
    protozero::pbf_reader item{buffer};

    REQUIRE(item.length() == 0);
    REQUIRE_FALSE(item); // test operator bool()
    REQUIRE_FALSE(item.next());
}

TEST_CASE("check every possible value for single byte in buffer") {
    char buffer;
    for (int i = 0; i <= 255; ++i) {
        buffer = static_cast<char>(i);
        protozero::pbf_reader item{&buffer, 1};

        REQUIRE(item.length() == 1);
        REQUIRE_FALSE(!item); // test operator bool()
        REQUIRE_THROWS((item.next(), item.skip()));
    }
}

TEST_CASE("next() should throw when illegal wire type is encountered") {
    const char buffer = 1u << 3u | 7u;

    protozero::pbf_reader item{&buffer, 1};
    REQUIRE_THROWS_AS(item.next(), const protozero::unknown_pbf_wire_type_exception&);
}

TEST_CASE("next() should throw when illegal tag is encountered") {
    std::string data;

    SECTION("tag 0") {
        protozero::write_varint(std::back_inserter(data), 0u << 3u | 1u);
    }

    SECTION("tag 19000") {
        protozero::write_varint(std::back_inserter(data), 19000u << 3u | 1u);
    }

    SECTION("tag 19001") {
        protozero::write_varint(std::back_inserter(data), 19001u << 3u | 1u);
    }

    SECTION("tag 19999") {
        protozero::write_varint(std::back_inserter(data), 19999u << 3u | 1u);
    }

    protozero::pbf_reader item{data};
    REQUIRE_THROWS_AS(item.next(), const protozero::invalid_tag_exception&);
}

TEST_CASE("next() works when a legal tag is encountered") {
    std::string data;

    SECTION("tag 1") {
        protozero::write_varint(std::back_inserter(data), 1u << 3u | 1u);
    }

    SECTION("tag 18999") {
        protozero::write_varint(std::back_inserter(data), 18999u << 3u | 1u);
    }

    SECTION("tag 20000") {
        protozero::write_varint(std::back_inserter(data), 20000u << 3u | 1u);
    }

    SECTION("tag 1^29 - 1") {
        protozero::write_varint(std::back_inserter(data), ((1u << 29u) - 1u) << 3u | 1u);
    }

    protozero::pbf_reader item{data};
    REQUIRE(item.next());
}

TEST_CASE("pbf_writer asserts on invalid tags") {
    std::string data;
    protozero::pbf_writer writer{data};

    REQUIRE_THROWS_AS(writer.add_int32(0, 123), const assert_error&);
    writer.add_int32(1, 123);
    writer.add_int32(2, 123);
    writer.add_int32(18999, 123);
    REQUIRE_THROWS_AS(writer.add_int32(19000, 123), const assert_error&);
    REQUIRE_THROWS_AS(writer.add_int32(19001, 123), const assert_error&);
    REQUIRE_THROWS_AS(writer.add_int32(19999, 123), const assert_error&);
    writer.add_int32(20000, 123);
    writer.add_int32((1u << 29u) - 1u, 123);
    REQUIRE_THROWS_AS(writer.add_int32(1u << 29u, 123), const assert_error&);
}

