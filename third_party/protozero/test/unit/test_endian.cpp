
#include <cstdint>
#include <limits>

#include <test.hpp>

#include <protozero/byteswap.hpp>

static int32_t check_swap_4(int32_t data) noexcept {
    protozero::detail::byteswap_inplace(&data);
    protozero::detail::byteswap_inplace(&data);
    return data;
}

static int64_t check_swap_8(int64_t data) noexcept {
    protozero::detail::byteswap_inplace(&data);
    protozero::detail::byteswap_inplace(&data);
    return data;
}

TEST_CASE("byte swapping") {
    REQUIRE(0 == check_swap_4(0));
    REQUIRE(1 == check_swap_4(1));
    REQUIRE(-1 == check_swap_4(-1));
    REQUIRE(395503 == check_swap_4(395503));
    REQUIRE(-804022 == check_swap_4(-804022));
    REQUIRE(std::numeric_limits<int32_t>::max() == check_swap_4(std::numeric_limits<int32_t>::max()));
    REQUIRE(std::numeric_limits<int32_t>::min() == check_swap_4(std::numeric_limits<int32_t>::min()));

    REQUIRE(0 == check_swap_8(0));
    REQUIRE(1 == check_swap_8(1));
    REQUIRE(-1 == check_swap_8(-1));
    REQUIRE(395503ll == check_swap_8(395503ll));
    REQUIRE(-804022ll == check_swap_8(-804022ll));
    REQUIRE(3280329805ll == check_swap_8(3280329805ll));
    REQUIRE(-2489204041ll == check_swap_8(-2489204041ll));
    REQUIRE(std::numeric_limits<int64_t>::max() == check_swap_8(std::numeric_limits<int64_t>::max()));
    REQUIRE(std::numeric_limits<int64_t>::min() == check_swap_8(std::numeric_limits<int64_t>::min()));
}

TEST_CASE("byte swap uint32_t") {
    uint32_t a = 17;
    protozero::detail::byteswap_inplace(&a);
    protozero::detail::byteswap_inplace(&a);

    REQUIRE(17 == a);
}

TEST_CASE("byte swap uint64_t") {
    uint64_t a = 347529808;
    protozero::detail::byteswap_inplace(&a);
    protozero::detail::byteswap_inplace(&a);

    REQUIRE(347529808 == a);
}

TEST_CASE("byte swap double") {
    double a = 1.1;
    protozero::detail::byteswap_inplace(&a);
    protozero::detail::byteswap_inplace(&a);

    REQUIRE(a == Approx(1.1));
}

TEST_CASE("byte swap float") {
    float a = 1.1f;
    protozero::detail::byteswap_inplace(&a);
    protozero::detail::byteswap_inplace(&a);

    REQUIRE(a == Approx(1.1f));
}

