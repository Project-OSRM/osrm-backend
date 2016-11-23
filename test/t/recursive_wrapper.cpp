
#include "catch.hpp"

#include <mapbox/recursive_wrapper.hpp>

#include <type_traits>
#include <utility>

using rwi = mapbox::util::recursive_wrapper<int>;
using rwp = mapbox::util::recursive_wrapper<std::pair<int, int>>;

static_assert(std::is_same<rwi::type, int>::value, "type check failed");

TEST_CASE("recursive wrapper of int")
{

    SECTION("construct with value")
    {
        rwi a{7};

        REQUIRE(a.get() == 7);
        REQUIRE(*a.get_pointer() == 7);

        a = 8;
        REQUIRE(a.get() == 8);

        rwi b{a};
        REQUIRE(b.get() == 8);

        rwi c;
        c = b;
        REQUIRE(b.get() == 8);
        REQUIRE(c.get() == 8);

        c = 9;
        REQUIRE(c.get() == 9);

        int x = 10;
        c = x;
        REQUIRE(c.get() == 10);

        b = std::move(c);
        REQUIRE(b.get() == 10);
    }

    SECTION("construct with const reference")
    {
        int i = 7;
        rwi a{i};

        REQUIRE(a.get() == 7);
    }

    SECTION("implicit conversion to reference of underlying type")
    {

        SECTION("const")
        {
            rwi const a{7};
            REQUIRE(a.get() == 7);
            REQUIRE(*a.get_pointer() == 7);

            rwi::type const& underlying = a;
            REQUIRE(underlying == 7);
        }

        SECTION("non const")
        {
            rwi a{7};
            REQUIRE(a.get() == 7);
            REQUIRE(*a.get_pointer() == 7);

            rwi::type& underlying = a;
            REQUIRE(underlying == 7);
            a = 8;
            REQUIRE(underlying == 8);
        }
    }
}

TEST_CASE("move of recursive wrapper")
{
    rwi a{1};

    SECTION("move constructor")
    {
        rwi b{std::move(a)};
        REQUIRE(b.get() == 1);
    }

    SECTION("operator= on rvalue")
    {
        rwi b{2};
        b = std::move(a);
        REQUIRE(b.get() == 1);
    }
}

TEST_CASE("swap")
{
    rwi a{1};
    rwi b{2};

    REQUIRE(a.get() == 1);
    REQUIRE(b.get() == 2);

    using std::swap;
    swap(a, b);

    REQUIRE(a.get() == 2);
    REQUIRE(b.get() == 1);
}

TEST_CASE("recursive wrapper of pair<int, int>")
{

    SECTION("default constructed")
    {
        rwp a;
        REQUIRE(a.get().first == 0);
        REQUIRE(a.get().second == 0);
    }

    SECTION("construct with value")
    {
        rwp a{std::make_pair(1, 2)};

        REQUIRE(a.get().first == 1);
        REQUIRE(a.get().second == 2);

        REQUIRE(a.get_pointer()->first == 1);
        REQUIRE(a.get_pointer()->second == 2);

        a = {3, 4};
        REQUIRE(a.get().first == 3);
        REQUIRE(a.get().second == 4);

        rwp b{a};
        REQUIRE(b.get().first == 3);
        REQUIRE(b.get().second == 4);

        rwp c;
        c = b;
        REQUIRE(b.get().first == 3);
        REQUIRE(b.get().second == 4);
        REQUIRE(c.get().first == 3);
        REQUIRE(c.get().second == 4);

        c = {5, 6};
        REQUIRE(c.get().first == 5);
        REQUIRE(c.get().second == 6);

        b = std::move(c);
        REQUIRE(b.get().first == 5);
        REQUIRE(b.get().second == 6);
        //        REQUIRE(c.get_pointer() == nullptr);
    }
}
