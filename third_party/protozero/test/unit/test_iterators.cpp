
#include <test.hpp>

TEST_CASE("default constructed varint_iterators are equal") {
    protozero::const_varint_iterator<uint32_t> a{};
    protozero::const_varint_iterator<uint32_t> b{};

    protozero::iterator_range<protozero::const_varint_iterator<uint32_t>> r{};

    REQUIRE(a == a);
    REQUIRE(a == b);
    REQUIRE(a == r.begin());
    REQUIRE(a == r.end());
    REQUIRE(r.empty());
    REQUIRE(r.size() == 0); // NOLINT(readability-container-size-empty)
    REQUIRE(r.begin() == r.end());
}

