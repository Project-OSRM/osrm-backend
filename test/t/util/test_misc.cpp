#include "catch.hpp"

#include <osmium/util/misc.hpp>

template <typename T>
void test_conv() {
    REQUIRE(osmium::detail::str_to_int<T>("0") == 0);
    REQUIRE(osmium::detail::str_to_int<T>("1") == 1);
    REQUIRE(osmium::detail::str_to_int<T>("9") == 9);
    REQUIRE(osmium::detail::str_to_int<T>("10") == 10);
    REQUIRE(osmium::detail::str_to_int<T>("44") == 44);
    REQUIRE(osmium::detail::str_to_int<T>("45") == 45);
    REQUIRE(osmium::detail::str_to_int<T>("  123") == 123);

    REQUIRE(osmium::detail::str_to_int<T>("-1") == 0);
    REQUIRE(osmium::detail::str_to_int<T>("-100") == 0);
    REQUIRE(osmium::detail::str_to_int<T>("1 ") == 0);
    REQUIRE(osmium::detail::str_to_int<T>("x1") == 0);
    REQUIRE(osmium::detail::str_to_int<T>("1x") == 0);
    REQUIRE(osmium::detail::str_to_int<T>("2 3") == 0);
    REQUIRE(osmium::detail::str_to_int<T>("9999999999999999999999") == 0);
}

TEST_CASE("string to integer conversion") {
    test_conv<int>();
    test_conv<unsigned int>();

    test_conv<int8_t>();
    REQUIRE(osmium::detail::str_to_int<int8_t>("126") == 126);
    REQUIRE(osmium::detail::str_to_int<int8_t>("127") == 0);
    REQUIRE(osmium::detail::str_to_int<int8_t>("128") == 0);

    test_conv<uint8_t>();
    REQUIRE(osmium::detail::str_to_int<uint8_t>("254") == 254);
    REQUIRE(osmium::detail::str_to_int<uint8_t>("255") == 0);
    REQUIRE(osmium::detail::str_to_int<uint8_t>("256") == 0);

    test_conv<int16_t>();
    REQUIRE(osmium::detail::str_to_int<int16_t>("32766") == 32766);
    REQUIRE(osmium::detail::str_to_int<int16_t>("32767") == 0);
    REQUIRE(osmium::detail::str_to_int<int16_t>("32768") == 0);

    test_conv<uint16_t>();
    REQUIRE(osmium::detail::str_to_int<uint16_t>("65534") == 65534);
    REQUIRE(osmium::detail::str_to_int<uint16_t>("65535") == 0);
    REQUIRE(osmium::detail::str_to_int<uint16_t>("65536") == 0);

    test_conv<int32_t>();
    REQUIRE(osmium::detail::str_to_int<int32_t>("2147483646") == 2147483646ll);
    REQUIRE(osmium::detail::str_to_int<int32_t>("2147483647") == 0);
    REQUIRE(osmium::detail::str_to_int<int32_t>("2147483648") == 0);

    test_conv<uint32_t>();
    REQUIRE(osmium::detail::str_to_int<uint32_t>("4294967294") == 4294967294ull);
    REQUIRE(osmium::detail::str_to_int<uint32_t>("4294967295") == 0);
    REQUIRE(osmium::detail::str_to_int<uint32_t>("4294967296") == 0);

    test_conv<int64_t>();
    REQUIRE(osmium::detail::str_to_int<int64_t>("9223372036854775806") == 9223372036854775806ll);
    REQUIRE(osmium::detail::str_to_int<int64_t>("9223372036854775807") == 0);
    REQUIRE(osmium::detail::str_to_int<int64_t>("9223372036854775808") == 0);

    test_conv<uint64_t>();
    REQUIRE(osmium::detail::str_to_int<uint64_t>("9223372036854775806") == 9223372036854775806ull);
    REQUIRE(osmium::detail::str_to_int<uint64_t>("9223372036854775807") == 0);
    REQUIRE(osmium::detail::str_to_int<uint64_t>("9223372036854775808") == 0);
}

