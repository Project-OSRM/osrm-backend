
#include "catch.hpp"

#include <mapbox/variant.hpp>
#include <mapbox/variant_io.hpp>

#include <string>

struct some_visitor
{
    int var_;

    some_visitor(int init)
        : var_(init) {}

    int operator()(int val) const
    {
        return var_ + val;
    }

    int operator()(double val) const
    {
        return var_ + int(val);
    }

    int operator()(const std::string&) const
    {
        return 0;
    }
};

TEST_CASE("non-const visitor works on const variants", "[visitor][unary visitor]")
{
    using variant_type = const mapbox::util::variant<int, double, std::string>;
    variant_type var1(123);
    variant_type var2(3.2);
    variant_type var3("foo");
    REQUIRE(var1.get<int>() == 123);
    REQUIRE(var2.get<double>() == Approx(3.2));
    REQUIRE(var3.get<std::string>() == "foo");

    some_visitor visitor{1};

    REQUIRE(mapbox::util::apply_visitor(visitor, var1) == 124);
    REQUIRE(mapbox::util::apply_visitor(visitor, var2) == 4);
    REQUIRE(mapbox::util::apply_visitor(visitor, var3) == 0);
}

TEST_CASE("const visitor works on const variants", "[visitor][unary visitor]")
{
    using variant_type = const mapbox::util::variant<int, double, std::string>;
    variant_type var1(123);
    variant_type var2(3.2);
    variant_type var3("foo");
    REQUIRE(var1.get<int>() == 123);
    REQUIRE(var2.get<double>() == Approx(3.2));
    REQUIRE(var3.get<std::string>() == "foo");

    const some_visitor visitor{1};

    REQUIRE(mapbox::util::apply_visitor(visitor, var1) == 124);
    REQUIRE(mapbox::util::apply_visitor(visitor, var2) == 4);
    REQUIRE(mapbox::util::apply_visitor(visitor, var3) == 0);
}

TEST_CASE("rvalue visitor works on const variants", "[visitor][unary visitor]")
{
    using variant_type = const mapbox::util::variant<int, double, std::string>;
    variant_type var1(123);
    variant_type var2(3.2);
    variant_type var3("foo");
    REQUIRE(var1.get<int>() == 123);
    REQUIRE(var2.get<double>() == Approx(3.2));
    REQUIRE(var3.get<std::string>() == "foo");

    REQUIRE(mapbox::util::apply_visitor(some_visitor{1}, var1) == 124);
    REQUIRE(mapbox::util::apply_visitor(some_visitor{1}, var2) == 4);
    REQUIRE(mapbox::util::apply_visitor(some_visitor{1}, var3) == 0);
}

TEST_CASE("visitor works on rvalue variants", "[visitor][unary visitor]")
{
    using variant_type = const mapbox::util::variant<int, double, std::string>;

    REQUIRE(mapbox::util::apply_visitor(some_visitor{1}, variant_type{123}) == 124);
    REQUIRE(mapbox::util::apply_visitor(some_visitor{1}, variant_type{3.2}) == 4);
    REQUIRE(mapbox::util::apply_visitor(some_visitor{1}, variant_type{"foo"}) == 0);
}

struct total_sizeof
{
    total_sizeof() : total_(0) {}

    template <class Value>
    int operator()(const Value&) const
    {
        total_ += int(sizeof(Value));
        return total_;
    }

    int result() const
    {
        return total_;
    }

    mutable int total_;

}; // total_sizeof

TEST_CASE("changes in visitor should be visible", "[visitor][unary visitor]")
{
    using variant_type = mapbox::util::variant<int, std::string, double>;
    variant_type v;
    total_sizeof ts;
    v = 5.9;
    REQUIRE(mapbox::util::apply_visitor(ts, v) == sizeof(double));
    REQUIRE(ts.result() == sizeof(double));
}

TEST_CASE("changes in const visitor (with mutable internals) should be visible", "[visitor][unary visitor]")
{
    using variant_type = const mapbox::util::variant<int, std::string, double>;
    variant_type v{"foo"};
    const total_sizeof ts;
    REQUIRE(mapbox::util::apply_visitor(ts, v) == sizeof(std::string));
    REQUIRE(ts.result() == sizeof(std::string));
}
