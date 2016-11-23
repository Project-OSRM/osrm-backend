
#include <type_traits>

#include "catch.hpp"

#include <mapbox/variant_io.hpp>

struct add_visitor
{
    add_visitor() {}

    template <typename A, typename B>
    double operator()(A a, B b) const
    {
        return a + b;
    }
};

TEST_CASE("const binary visitor works on const variants" NAME_EXT, "[visitor][binary visitor]")
{
    const variant_type a{7};
    const variant_type b = 3;
    const variant_type c{7.1};
    const variant_type d = 2.9;

    const add_visitor v;

    REQUIRE(mapbox::util::apply_visitor(v, a, b) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, d) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, a, c) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, a, d) == Approx(9.9));

    REQUIRE(mapbox::util::apply_visitor(v, b, a) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, d, c) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, a) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, d, a) == Approx(9.9));
}

TEST_CASE("non-const binary visitor works on const variants" NAME_EXT, "[visitor][binary visitor]")
{
    const variant_type a = 7;
    const variant_type b = 3;
    const variant_type c = 7.1;
    const variant_type d = 2.9;

    add_visitor v;

    REQUIRE(mapbox::util::apply_visitor(v, a, b) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, d) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, a, c) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, a, d) == Approx(9.9));

    REQUIRE(mapbox::util::apply_visitor(v, b, a) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, d, c) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, a) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, d, a) == Approx(9.9));
}

TEST_CASE("const binary visitor works on non-const variants" NAME_EXT, "[visitor][binary visitor]")
{
    variant_type a = 7;
    variant_type b = 3;
    variant_type c = 7.1;
    variant_type d = 2.9;

    const add_visitor v;

    REQUIRE(mapbox::util::apply_visitor(v, a, b) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, d) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, a, c) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, a, d) == Approx(9.9));

    REQUIRE(mapbox::util::apply_visitor(v, b, a) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, d, c) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, a) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, d, a) == Approx(9.9));
}

TEST_CASE("non-const binary visitor works on non-const variants" NAME_EXT, "[visitor][binary visitor]")
{
    variant_type a = 7;
    variant_type b = 3;
    variant_type c = 7.1;
    variant_type d = 2.9;

    add_visitor v;

    REQUIRE(mapbox::util::apply_visitor(v, a, b) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, d) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, a, c) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, a, d) == Approx(9.9));

    REQUIRE(mapbox::util::apply_visitor(v, b, a) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, d, c) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(v, c, a) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(v, d, a) == Approx(9.9));
}

TEST_CASE("rvalue binary visitor works on const variants" NAME_EXT, "[visitor][binary visitor]")
{
    const variant_type a = 7;
    const variant_type b = 3;
    const variant_type c = 7.1;
    const variant_type d = 2.9;

    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, a, b) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, c, d) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, a, c) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, a, d) == Approx(9.9));

    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, b, a) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, d, c) == Approx(10));
    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, c, a) == Approx(14.1));
    REQUIRE(mapbox::util::apply_visitor(add_visitor{}, d, a) == Approx(9.9));
}

struct sum_mul_visitor
{
    double sum;

    sum_mul_visitor() : sum(0.0) {}

    template <typename A, typename B>
    double operator()(A a, B b)
    {
        double m = a * b;
        sum += m;
        return m;
    }
};

TEST_CASE("mutable binary visitor works" NAME_EXT, "[visitor][binary visitor]")
{
    const variant_type a = 2;
    const variant_type b = 3;
    const variant_type c = 0.1;
    const variant_type d = 0.2;

    sum_mul_visitor v;

    REQUIRE(mapbox::util::apply_visitor(v, a, b) == Approx(6));
    REQUIRE(mapbox::util::apply_visitor(v, c, d) == Approx(0.02));
    REQUIRE(mapbox::util::apply_visitor(v, a, c) == Approx(0.2));
    REQUIRE(mapbox::util::apply_visitor(v, a, d) == Approx(0.4));

    REQUIRE(v.sum == Approx(6.62));

    REQUIRE(mapbox::util::apply_visitor(v, b, a) == Approx(6));
    REQUIRE(mapbox::util::apply_visitor(v, d, c) == Approx(0.02));
    REQUIRE(mapbox::util::apply_visitor(v, c, a) == Approx(0.2));
    REQUIRE(mapbox::util::apply_visitor(v, d, a) == Approx(0.4));
}

struct swap_visitor
{
    swap_visitor(){};

    template <typename A, typename B>
    void operator()(A& a, B& b) const
    {
        using T = typename std::common_type<A, B>::type;
        T tmp = a;
        a = b;
        b = tmp;
    }
};

TEST_CASE("static mutating visitor on mutable variants works" NAME_EXT, "[visitor][binary visitor]")
{
    variant_type a = 2;
    variant_type b = 3;
    variant_type c = 0.1;
    variant_type d = 0.2;

    const swap_visitor v;

    SECTION("swap a and b")
    {
        mapbox::util::apply_visitor(v, a, b);
        REQUIRE(a.get<int>() == 3);
        REQUIRE(b.get<int>() == 2);
    }

    SECTION("swap c and d")
    {
        mapbox::util::apply_visitor(v, c, d);
        REQUIRE(c.get<double>() == Approx(0.2));
        REQUIRE(d.get<double>() == Approx(0.1));
    }

    SECTION("swap a and c")
    {
        mapbox::util::apply_visitor(v, a, c);
        REQUIRE(a.get<int>() == 0);
        REQUIRE(c.get<double>() == Approx(2.0));
    }

    SECTION("swap c and a")
    {
        mapbox::util::apply_visitor(v, c, a);
        REQUIRE(a.get<int>() == 0);
        REQUIRE(c.get<double>() == Approx(2.0));
    }
}
