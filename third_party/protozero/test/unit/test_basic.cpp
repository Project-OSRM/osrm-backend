
#include <test.hpp>

#include <type_traits>

template <typename T>
struct movable_not_copyable {
    constexpr static bool value = !std::is_copy_constructible<T>::value &&
                                  !std::is_copy_assignable<T>::value    &&
                                   std::is_nothrow_move_constructible<T>::value &&
                                   std::is_nothrow_move_assignable<T>::value;
};

static_assert(movable_not_copyable<protozero::pbf_writer>::value, "pbf_writer should be nothrow movable, but not copyable");

enum class dummy : protozero::pbf_tag_type {};
static_assert(movable_not_copyable<protozero::pbf_builder<dummy>>::value, "pbf_builder should be nothrow movable, but not copyable");

static_assert(movable_not_copyable<protozero::packed_field_bool>::value, "packed_field_bool should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_enum>::value, "packed_field_enum should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_int32>::value, "packed_field_int32 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_sint32>::value, "packed_field_sint32 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_uint32>::value, "packed_field_uint32 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_int64>::value, "packed_field_int64 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_sint64>::value, "packed_field_sint64 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_uint64>::value, "packed_field_uint64 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_fixed32>::value, "packed_field_fixed32 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_sfixed32>::value, "packed_field_sfixed32 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_fixed64>::value, "packed_field_fixed64 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_sfixed64>::value, "packed_field_sfixed64 should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_float>::value, "packed_field_float should be nothrow movable, but not copyable");
static_assert(movable_not_copyable<protozero::packed_field_double>::value, "packed_field_double should be nothrow movable, but not copyable");

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
    char buffer[1];
    for (int i = 0; i <= 255; ++i) {
        buffer[0] = static_cast<char>(i);
        protozero::pbf_reader item{buffer, 1};

        REQUIRE(item.length() == 1);
        REQUIRE_FALSE(!item); // test operator bool()
        REQUIRE_THROWS((item.next(), item.skip()));
    }
}

TEST_CASE("next() should throw when illegal wire type is encountered") {
    const char buffer[1] = {1U << 3U | 7U};

    protozero::pbf_reader item{buffer, 1};
    REQUIRE_THROWS_AS(item.next(), protozero::unknown_pbf_wire_type_exception);
}

TEST_CASE("next() should throw when illegal tag 0 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, 0U << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE_THROWS_AS(item.next(), protozero::invalid_tag_exception);
}

TEST_CASE("next() should throw when illegal tag 19000 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, 19000U << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE_THROWS_AS(item.next(), protozero::invalid_tag_exception);
}

TEST_CASE("next() should throw when illegal tag 19001 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, 19001U << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE_THROWS_AS(item.next(), protozero::invalid_tag_exception);
}

TEST_CASE("next() should throw when illegal tag 19999 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, 19999U << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE_THROWS_AS(item.next(), protozero::invalid_tag_exception);
}

TEST_CASE("next() works when legal tag 1 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, 1U << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE(item.next());
}

TEST_CASE("next() works when legal tag 18999 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, 18999U << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE(item.next());
}

TEST_CASE("next() works when legal tag 20000 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, 20000U << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE(item.next());
}

TEST_CASE("next() works when legal tag 1^29 - 1 is encountered") {
    std::string data;
    protozero::add_varint_to_buffer(&data, ((1UL << 29U) - 1U) << 3U | 1U);
    protozero::pbf_reader item{data};
    REQUIRE(item.next());
}

TEST_CASE("pbf_writer asserts on invalid tags") {
    std::string data;
    protozero::pbf_writer writer{data};

    REQUIRE_THROWS_AS(writer.add_int32(0, 123), assert_error);
    writer.add_int32(1, 123);
    writer.add_int32(2, 123);
    writer.add_int32(18999, 123);
    REQUIRE_THROWS_AS(writer.add_int32(19000, 123), assert_error);
    REQUIRE_THROWS_AS(writer.add_int32(19001, 123), assert_error);
    REQUIRE_THROWS_AS(writer.add_int32(19999, 123), assert_error);
    writer.add_int32(20000, 123);
    writer.add_int32((1U << 29U) - 1U, 123);
    REQUIRE_THROWS_AS(writer.add_int32(1U << 29U, 123), assert_error);
}

