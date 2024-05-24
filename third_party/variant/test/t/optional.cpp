#include "catch.hpp"

#include <mapbox/optional.hpp>

struct dummy
{
    dummy(int _m_1, int _m_2) : m_1(_m_1), m_2(_m_2) {}
    int m_1;
    int m_2;
};

TEST_CASE("optional can be instantiated with a POD type", "[optional]")
{
    mapbox::util::optional<int> dbl_opt;

    REQUIRE(!dbl_opt);
    dbl_opt = 3;
    REQUIRE(dbl_opt);

    REQUIRE(dbl_opt.get() == 3);
    REQUIRE(*dbl_opt == 3);
}

TEST_CASE("copy c'tor", "[optional]")
{
    mapbox::util::optional<int> dbl_opt;

    REQUIRE(!dbl_opt);
    dbl_opt = 3;
    REQUIRE(dbl_opt);

    mapbox::util::optional<int> other = dbl_opt;

    REQUIRE(other.get() == 3);
    REQUIRE(*other == 3);
}

TEST_CASE("const operator*, const get()", "[optional]")
{
    const mapbox::util::optional<int> dbl_opt = 3;

    REQUIRE(dbl_opt);

    auto pi1 = dbl_opt.get();
    auto pi2 = *dbl_opt;

    REQUIRE(pi1 == 3);
    REQUIRE(pi2 == 3);
}

TEST_CASE("non-const operator*, non-const get()", "[optional]")
{
    mapbox::util::optional<int> dbl_opt = 3;

    REQUIRE(dbl_opt);

    auto pi1 = dbl_opt.get();
    auto pi2 = *dbl_opt;

    REQUIRE(pi1 == 3);
    REQUIRE(pi2 == 3);
}

TEST_CASE("emplace initialization, reset", "[optional]")
{
    mapbox::util::optional<dummy> dummy_opt;
    REQUIRE(!dummy_opt);

    // rvalues, baby!
    dummy_opt.emplace(1, 2);
    REQUIRE(dummy_opt);
    REQUIRE(dummy_opt.get().m_1 == 1);
    REQUIRE((*dummy_opt).m_2 == 2);

    dummy_opt.reset();
    REQUIRE(!dummy_opt);
}

TEST_CASE("assignment", "[optional]")
{
    mapbox::util::optional<int> a;
    mapbox::util::optional<int> b;

    a = 1;
    b = 3;
    REQUIRE(a.get() == 1);
    REQUIRE(b.get() == 3);
    b = a;
    REQUIRE(a.get() == b.get());
    REQUIRE(b.get() == 1);
}

TEST_CASE("self assignment", "[optional]")
{
    mapbox::util::optional<int> a;

    a = 1;
    REQUIRE(a.get() == 1);
#if !defined(__clang__)
    a = a;
    REQUIRE(a.get() == 1);
#endif
}
