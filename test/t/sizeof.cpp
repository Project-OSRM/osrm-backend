#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "catch.hpp"

#include <mapbox/variant.hpp>
#include <mapbox/variant_io.hpp>

struct some_struct
{
    int a;
    bool b;
    std::string c;
};

using variant_internal_index_type = mapbox::util::type_index_t;

TEST_CASE("size of variants")
{
    constexpr const auto min_overhead = sizeof(variant_internal_index_type);

    using namespace std; // workaround for bug in GCC <= 4.8 where max_align_t is not in std
    constexpr const auto max_overhead = alignof(max_align_t) + min_overhead;

    using v1 = mapbox::util::variant<int>;
    using v2 = mapbox::util::variant<int, bool, int64_t>;
    using v3 = mapbox::util::variant<int, std::string>;
    using v4 = mapbox::util::variant<std::string, std::string>;
    using v5 = mapbox::util::variant<some_struct>;

    constexpr const auto si = sizeof(int);
    constexpr const auto sb = sizeof(bool);
    constexpr const auto si64 = sizeof(int64_t);
    constexpr const auto sd = sizeof(double);
    constexpr const auto sstr = sizeof(std::string);
    constexpr const auto spi = sizeof(std::pair<int, int>);
    constexpr const auto ss = sizeof(some_struct);

    REQUIRE(sizeof(v1) <= max_overhead + si);
    REQUIRE(sizeof(v2) <= max_overhead + std::max({si, sb, si64}));
    REQUIRE(sizeof(v3) <= max_overhead + std::max({si, sstr}));
    REQUIRE(sizeof(v4) <= max_overhead + sstr);
    REQUIRE(sizeof(v5) <= max_overhead + ss);

    REQUIRE(sizeof(v1) >= min_overhead + si);
    REQUIRE(sizeof(v2) >= min_overhead + std::max({si, sb, si64}));
    REQUIRE(sizeof(v3) >= min_overhead + std::max({si, sstr}));
    REQUIRE(sizeof(v4) >= min_overhead + sstr);
    REQUIRE(sizeof(v5) >= min_overhead + ss);
}
