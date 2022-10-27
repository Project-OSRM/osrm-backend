
#include <test.hpp>

static_assert(protozero::encode_zigzag32( 0L) == 0UL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag32(-1L) == 1UL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag32( 1L) == 2UL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag32(-2L) == 3UL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag32( 2L) == 4UL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag32( std::numeric_limits<int32_t>::max()) == 2 * static_cast<uint32_t>(std::numeric_limits<int32_t>::max()), "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag32(-std::numeric_limits<int32_t>::max()) == 2 * static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) - 1, "test constexpr zigzag functions");

static_assert(protozero::encode_zigzag64( 0LL) == 0ULL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag64(-1LL) == 1ULL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag64( 1LL) == 2ULL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag64(-2LL) == 3ULL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag64( 2LL) == 4ULL, "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag64( std::numeric_limits<int64_t>::max()) == 2 * static_cast<uint64_t>(std::numeric_limits<int64_t>::max()), "test constexpr zigzag functions");
static_assert(protozero::encode_zigzag64(-std::numeric_limits<int64_t>::max()) == 2 * static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) - 1, "test constexpr zigzag functions");

static_assert(protozero::decode_zigzag32(0UL) ==  0L, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag32(1UL) == -1L, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag32(2UL) ==  1L, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag32(3UL) == -2L, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag32(4UL) ==  2L, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag32(0xfffffffeUL) ==  0x7fffffffL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag32(0xfffffffdUL) == -0x7fffffffL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag32(2 * static_cast<uint32_t>(std::numeric_limits<int32_t>::max())    ) ==  std::numeric_limits<int32_t>::max(), "test constexpr zigzag functions");

// fails on Visual Studio 2017
//static_assert(protozero::decode_zigzag32(2 * static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) - 1) == -std::numeric_limits<int32_t>::max(), "test constexpr zigzag functions");

static_assert(protozero::decode_zigzag64(0ULL) ==  0LL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(1ULL) == -1LL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(2ULL) ==  1LL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(3ULL) == -2LL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(4ULL) ==  2LL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(0xfffffffffffffffeULL) ==  0x7fffffffffffffffLL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(0xfffffffffffffffdULL) == -0x7fffffffffffffffLL, "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(2 * static_cast<uint64_t>(std::numeric_limits<int64_t>::max())    ) ==  std::numeric_limits<int64_t>::max(), "test constexpr zigzag functions");
static_assert(protozero::decode_zigzag64(2 * static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) - 1) == -std::numeric_limits<int64_t>::max(), "test constexpr zigzag functions");

inline constexpr int32_t zz32(int32_t val) noexcept {
    return protozero::decode_zigzag32(protozero::encode_zigzag32(val));
}

inline constexpr int64_t zz64(int64_t val) noexcept {
    return protozero::decode_zigzag64(protozero::encode_zigzag64(val));
}

TEST_CASE("zigzag encode some 32 bit values") {
    REQUIRE(protozero::encode_zigzag32( 0L) == 0UL);
    REQUIRE(protozero::encode_zigzag32(-1L) == 1UL);
    REQUIRE(protozero::encode_zigzag32( 1L) == 2UL);
    REQUIRE(protozero::encode_zigzag32(-2L) == 3UL);
    REQUIRE(protozero::encode_zigzag32( 2L) == 4UL);
    REQUIRE(protozero::encode_zigzag32( std::numeric_limits<int32_t>::max()) == 2 * static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));
    REQUIRE(protozero::encode_zigzag32(-std::numeric_limits<int32_t>::max()) == 2 * static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) - 1);
}

TEST_CASE("zigzag encode some 64 bit values") {
    REQUIRE(protozero::encode_zigzag64( 0LL) == 0ULL);
    REQUIRE(protozero::encode_zigzag64(-1LL) == 1ULL);
    REQUIRE(protozero::encode_zigzag64( 1LL) == 2ULL);
    REQUIRE(protozero::encode_zigzag64(-2LL) == 3ULL);
    REQUIRE(protozero::encode_zigzag64( 2LL) == 4ULL);
    REQUIRE(protozero::encode_zigzag64( std::numeric_limits<int64_t>::max()) == 2 * static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
    REQUIRE(protozero::encode_zigzag64(-std::numeric_limits<int64_t>::max()) == 2 * static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) - 1);
}

TEST_CASE("zigzag and back - 32bit") {
    REQUIRE(zz32( 0L) ==  0L);
    REQUIRE(zz32( 1L) ==  1L);
    REQUIRE(zz32(-1L) == -1L);
    REQUIRE(zz32(std::numeric_limits<int32_t>::max()) == std::numeric_limits<int32_t>::max());
    REQUIRE(zz32(std::numeric_limits<int32_t>::min()) == std::numeric_limits<int32_t>::min());
}

TEST_CASE("zigzag and back - 64bit") {
    REQUIRE(zz64( 0LL) ==  0LL);
    REQUIRE(zz64( 1LL) ==  1LL);
    REQUIRE(zz64(-1LL) == -1LL);
    REQUIRE(zz64(std::numeric_limits<int64_t>::max()) == std::numeric_limits<int64_t>::max());
    REQUIRE(zz64(std::numeric_limits<int64_t>::min()) == std::numeric_limits<int64_t>::min());
}

