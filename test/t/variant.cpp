#include "catch.hpp"

#include <mapbox/variant.hpp>
#include <mapbox/variant_io.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

// Hack to make nullptr work with Catch
namespace std {

template <class C, class T>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& os, std::nullptr_t)
{
    return os << (void*)nullptr;
}
}

TEST_CASE("variant can be moved into vector", "[variant]")
{
    using variant_type = mapbox::util::variant<bool, std::string>;
    variant_type v(std::string("test"));
    std::vector<variant_type> vec;
    vec.emplace_back(std::move(v));
    REQUIRE(v.get<std::string>() != std::string("test"));
    REQUIRE(vec.at(0).get<std::string>() == std::string("test"));
}

TEST_CASE("variant should support built-in types", "[variant]")
{
    SECTION("bool")
    {
        mapbox::util::variant<bool> v(true);
        REQUIRE(v.valid());
        REQUIRE(v.is<bool>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<bool>() == true);
        v.set<bool>(false);
        REQUIRE(v.get<bool>() == false);
        v = true;
        REQUIRE(v == mapbox::util::variant<bool>(true));
    }
    SECTION("nullptr")
    {
        using value_type = std::nullptr_t;
        mapbox::util::variant<value_type> v(nullptr);
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<value_type>() == nullptr);
        REQUIRE(v == mapbox::util::variant<value_type>(nullptr));
    }
    SECTION("unique_ptr")
    {
        using value_type = std::unique_ptr<std::string>;
        mapbox::util::variant<value_type> v(value_type(new std::string("hello")));
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(*v.get<value_type>().get() == *value_type(new std::string("hello")).get());
        REQUIRE(*v.get<value_type>() == "hello");
    }
    SECTION("string")
    {
        using value_type = std::string;
        mapbox::util::variant<value_type> v(value_type("hello"));
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<value_type>() == value_type("hello"));
        v.set<value_type>(value_type("there"));
        REQUIRE(v.get<value_type>() == value_type("there"));
        v = value_type("variant");
        REQUIRE(v == mapbox::util::variant<value_type>(value_type("variant")));
    }
    SECTION("size_t")
    {
        using value_type = std::size_t;
        mapbox::util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(value_type(0));
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == mapbox::util::variant<value_type>(value_type(1)));
    }
    SECTION("int8_t")
    {
        using value_type = std::int8_t;
        mapbox::util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == mapbox::util::variant<value_type>(value_type(1)));
    }
    SECTION("int16_t")
    {
        using value_type = std::int16_t;
        mapbox::util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == mapbox::util::variant<value_type>(value_type(1)));
    }
    SECTION("int32_t")
    {
        using value_type = std::int32_t;
        mapbox::util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == mapbox::util::variant<value_type>(value_type(1)));
    }
    SECTION("int64_t")
    {
        using value_type = std::int64_t;
        mapbox::util::variant<value_type> v(std::numeric_limits<value_type>::max());
        REQUIRE(v.valid());
        REQUIRE(v.is<value_type>());
        REQUIRE(v.which() == 0);
        REQUIRE(v.get<value_type>() == std::numeric_limits<value_type>::max());
        v.set<value_type>(0);
        REQUIRE(v.get<value_type>() == value_type(0));
        v = value_type(1);
        REQUIRE(v == mapbox::util::variant<value_type>(value_type(1)));
    }
}

struct MissionInteger
{
    using value_type = uint64_t;
    value_type val_;

  public:
    MissionInteger(uint64_t val) : val_(val) {}

    bool operator==(MissionInteger const& rhs) const
    {
        return (val_ == rhs.get());
    }

    uint64_t get() const
    {
        return val_;
    }
};

std::ostream& operator<<(std::ostream& os, MissionInteger const& rhs)
{
    os << rhs.get();
    return os;
}

TEST_CASE("variant should support custom types", "[variant]")
{
    // http://www.missionintegers.com/integer/34838300
    mapbox::util::variant<MissionInteger> v(MissionInteger(34838300));
    REQUIRE(v.valid());
    REQUIRE(v.is<MissionInteger>());
    REQUIRE(v.which() == 0);
    REQUIRE(v.get<MissionInteger>() == MissionInteger(34838300));
    REQUIRE(v.get<MissionInteger>().get() == MissionInteger::value_type(34838300));
    // TODO: should both of the set usages below compile?
    v.set<MissionInteger>(MissionInteger::value_type(0));
    v.set<MissionInteger>(MissionInteger(0));
    REQUIRE(v.get<MissionInteger>().get() == MissionInteger::value_type(0));
    v = MissionInteger(1);
    REQUIRE(v == mapbox::util::variant<MissionInteger>(MissionInteger(1)));
}

TEST_CASE("variant::which() returns zero based index of stored type", "[variant]")
{
    using variant_type = mapbox::util::variant<bool, std::string, std::uint64_t, std::int64_t, double, float>;
    // which() returns index in forward order
    REQUIRE(0 == variant_type(true).which());
    REQUIRE(1 == variant_type(std::string("test")).which());
    REQUIRE(2 == variant_type(std::uint64_t(0)).which());
    REQUIRE(3 == variant_type(std::int64_t(0)).which());
    REQUIRE(4 == variant_type(double(0.0)).which());
    REQUIRE(5 == variant_type(float(0.0)).which());
}

TEST_CASE("get with wrong type (here: double) should throw", "[variant]")
{
    using variant_type = mapbox::util::variant<int, double>;
    variant_type var = 5;
    REQUIRE(var.is<int>());
    REQUIRE_FALSE(var.is<double>());
    REQUIRE(var.get<int>() == 5);
    REQUIRE_THROWS_AS(var.get<double>(),
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("get with wrong type (here: int) should throw", "[variant]")
{
    using variant_type = mapbox::util::variant<int, double>;
    variant_type var = 5.0;
    REQUIRE(var.is<double>());
    REQUIRE_FALSE(var.is<int>());
    REQUIRE(var.get<double>() == 5.0);
    REQUIRE(mapbox::util::get<double>(var) == 5.0);
    REQUIRE_THROWS_AS(var.get<int>(),
                      mapbox::util::bad_variant_access&);
    REQUIRE_THROWS_AS(mapbox::util::get<int>(var),
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("get on const varint with wrong type (here: int) should throw", "[variant]")
{
    using variant_type = mapbox::util::variant<int, double>;
    const variant_type var = 5.0;
    REQUIRE(var.is<double>());
    REQUIRE_FALSE(var.is<int>());
    REQUIRE(var.get<double>() == 5.0);
    REQUIRE(mapbox::util::get<double>(var) == 5.0);
    REQUIRE_THROWS_AS(var.get<int>(),
                      mapbox::util::bad_variant_access&);
    REQUIRE_THROWS_AS(mapbox::util::get<int>(var),
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("get with any type should throw if not initialized", "[variant]")
{
    mapbox::util::variant<int, double> var{mapbox::util::no_init()};
    REQUIRE_THROWS_AS(var.get<int>(),
                      mapbox::util::bad_variant_access&);
    REQUIRE_THROWS_AS(var.get<double>(),
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("no_init variant can be copied and moved from", "[variant]")
{
    using variant_type = mapbox::util::variant<int, double>;

    variant_type v1{mapbox::util::no_init()};
    variant_type v2{42};
    variant_type v3{23};

    REQUIRE(v2.get<int>() == 42);
    v2 = v1;
    REQUIRE_THROWS_AS(v2.get<int>(),
                      mapbox::util::bad_variant_access&);

    REQUIRE(v3.get<int>() == 23);
    v3 = std::move(v1);
    REQUIRE_THROWS_AS(v3.get<int>(),
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("no_init variant can be copied and moved to", "[variant]")
{
    using variant_type = mapbox::util::variant<int, double>;

    variant_type v1{42};
    variant_type v2{mapbox::util::no_init()};
    variant_type v3{mapbox::util::no_init()};

    REQUIRE_THROWS_AS(v2.get<int>(),
                      mapbox::util::bad_variant_access&);

    REQUIRE(v1.get<int>() == 42);
    v2 = v1;
    REQUIRE(v2.get<int>() == 42);
    REQUIRE(v1.get<int>() == 42);

    REQUIRE_THROWS_AS(v3.get<int>(),
                      mapbox::util::bad_variant_access&);

    v3 = std::move(v1);
    REQUIRE(v3.get<int>() == 42);
}

TEST_CASE("implicit conversion", "[variant][implicit conversion]")
{
    using variant_type = mapbox::util::variant<int>;
    variant_type var(5.0); // converted to int
    REQUIRE(var.get<int>() == 5);
    var = 6.0; // works for operator=, too
    REQUIRE(var.get<int>() == 6);
}

TEST_CASE("implicit conversion to first type in variant type list", "[variant][implicit conversion]")
{
    using variant_type = mapbox::util::variant<long, char>;
    variant_type var = 5l; // converted to long
    REQUIRE(var.get<long>() == 5);
    REQUIRE_THROWS_AS(var.get<char>(),
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("implicit conversion to unsigned char", "[variant][implicit conversion]")
{
    using variant_type = mapbox::util::variant<unsigned char>;
    variant_type var = 100.0;
    CHECK(var.get<unsigned char>() == static_cast<unsigned char>(100.0));
    CHECK(var.get<unsigned char>() == static_cast<unsigned char>(static_cast<unsigned int>(100.0)));
}

struct dummy
{
};

TEST_CASE("implicit conversion to a suitable type", "[variant][implicit conversion]")
{
    using mapbox::util::variant;
    CHECK_NOTHROW((variant<dummy, float, std::string>(123)).get<float>());
    CHECK_NOTHROW((variant<dummy, float, std::string>("foo")).get<std::string>());
}

TEST_CASE("value_traits for non-convertible type", "[variant::detail]")
{
    namespace detail = mapbox::util::detail;
    using target_type = detail::value_traits<dummy, int>::target_type;
    CHECK((std::is_same<target_type, void>::value) == true);
}

TEST_CASE("Type indexing should work with variants with duplicated types", "[variant::detail]")
{
    // Index is in reverse order
    REQUIRE((mapbox::util::detail::value_traits<bool, bool, int, double, std::string>::index == 3));
    REQUIRE((mapbox::util::detail::value_traits<bool, bool, int, double, int>::index == 3));
    REQUIRE((mapbox::util::detail::value_traits<int, bool, int, double, std::string>::index == 2));
    REQUIRE((mapbox::util::detail::value_traits<int, bool, int, double, int>::index == 2));
    REQUIRE((mapbox::util::detail::value_traits<double, bool, int, double, std::string>::index == 1));
    REQUIRE((mapbox::util::detail::value_traits<bool, bool, int, bool, std::string>::index == 3));
    REQUIRE((mapbox::util::detail::value_traits<std::string, bool, int, double, std::string>::index == 0));
    REQUIRE((mapbox::util::detail::value_traits<dummy, bool, int, double, std::string>::index == mapbox::util::detail::invalid_value));
    REQUIRE((mapbox::util::detail::value_traits<std::vector<int>, bool, int, double, std::string>::index == mapbox::util::detail::invalid_value));
}

TEST_CASE("variant default constructor", "[variant][default constructor]")
{
    // By default variant is initialised with (default constructed) first type in template parameters pack
    // As a result first type in Types... must be default constructable
    // NOTE: index in reverse order -> index = N - 1
    using variant_type = mapbox::util::variant<int, double, std::string>;
    REQUIRE(variant_type{}.which() == 0);
    REQUIRE(variant_type{}.valid());
    REQUIRE_FALSE(variant_type{mapbox::util::no_init()}.valid());
}

TEST_CASE("variant printer", "[visitor][unary visitor][printer]")
{
    using variant_type = mapbox::util::variant<int, double, std::string>;
    std::vector<variant_type> var = {2.1, 123, "foo", 456};
    std::stringstream out;
    std::copy(var.begin(), var.end(), std::ostream_iterator<variant_type>(out, ","));
    out << var[2];
    REQUIRE(out.str() == "2.1,123,foo,456,foo");
}

TEST_CASE("swapping variants should do the right thing", "[variant]")
{
    using variant_type = mapbox::util::variant<int, double, std::string>;
    variant_type a = 7;
    variant_type b = 3;
    variant_type c = 3.141;
    variant_type d = "foo";
    variant_type e = "a long string that is longer than small string optimization";

    using std::swap;
    swap(a, b);
    REQUIRE(a.get<int>() == 3);
    REQUIRE(a.which() == 0);
    REQUIRE(b.get<int>() == 7);
    REQUIRE(b.which() == 0);

    swap(b, c);
    REQUIRE(b.get<double>() == Approx(3.141));
    REQUIRE(b.which() == 1);
    REQUIRE(c.get<int>() == 7);
    REQUIRE(c.which() == 0);

    swap(b, d);
    REQUIRE(b.get<std::string>() == "foo");
    REQUIRE(b.which() == 2);
    REQUIRE(d.get<double>() == Approx(3.141));
    REQUIRE(d.which() == 1);

    swap(b, e);
    REQUIRE(b.get<std::string>() == "a long string that is longer than small string optimization");
    REQUIRE(b.which() == 2);
    REQUIRE(e.get<std::string>() == "foo");
    REQUIRE(e.which() == 2);
}

TEST_CASE("variant should work with equality operators")
{
    using variant_type = mapbox::util::variant<int, std::string>;

    variant_type a{1};
    variant_type b{1};
    variant_type c{2};
    variant_type s{"foo"};

    REQUIRE(a == a);
    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    REQUIRE_FALSE(a == s);
    REQUIRE_FALSE(c == s);

    REQUIRE_FALSE(a != a);
    REQUIRE_FALSE(a != b);
    REQUIRE(a != c);
    REQUIRE(a != s);
    REQUIRE(c != s);
}

TEST_CASE("variant should work with comparison operators")
{
    using variant_type = mapbox::util::variant<int, std::string>;

    variant_type a{1};
    variant_type b{1};
    variant_type c{2};
    variant_type s{"foo"};
    variant_type t{"bar"};

    REQUIRE_FALSE(a < a);
    REQUIRE_FALSE(a < b);
    REQUIRE(a < c);
    REQUIRE(a < s);
    REQUIRE(c < s);
    REQUIRE(t < s);

    REQUIRE_FALSE(a > a);
    REQUIRE_FALSE(a > b);
    REQUIRE_FALSE(a > c);
    REQUIRE_FALSE(a > s);
    REQUIRE_FALSE(c > s);
    REQUIRE_FALSE(t > s);

    REQUIRE(a <= a);
    REQUIRE(a <= b);
    REQUIRE(a <= c);
    REQUIRE(a <= s);
    REQUIRE(c <= s);
    REQUIRE(t <= s);

    REQUIRE(a >= a);
    REQUIRE(a >= b);
    REQUIRE_FALSE(a >= c);
    REQUIRE_FALSE(a >= s);
    REQUIRE_FALSE(c >= s);
    REQUIRE_FALSE(t >= s);
}

TEST_CASE("storing reference wrappers works")
{
    using variant_type = mapbox::util::variant<std::reference_wrapper<int>, std::reference_wrapper<double>>;

    int a = 1;
    variant_type v{std::ref(a)};
    REQUIRE(v.get<int>() == 1);
    REQUIRE(mapbox::util::get<int>(v) == 1);
    REQUIRE_THROWS_AS(v.get<double>(),
                      mapbox::util::bad_variant_access&);
    REQUIRE_THROWS_AS(mapbox::util::get<double>(v),
                      mapbox::util::bad_variant_access&);
    a = 2;
    REQUIRE(v.get<int>() == 2);
    v.get<int>() = 3;
    REQUIRE(a == 3);

    double b = 3.141;
    v = std::ref(b);
    REQUIRE(v.get<double>() == Approx(3.141));
    REQUIRE(mapbox::util::get<double>(v) == Approx(3.141));
    REQUIRE_THROWS_AS(v.get<int>(),
                      mapbox::util::bad_variant_access&);
    REQUIRE_THROWS_AS(mapbox::util::get<int>(v),
                      mapbox::util::bad_variant_access&);
    b = 2.718;
    REQUIRE(v.get<double>() == Approx(2.718));
    a = 3;
    REQUIRE(v.get<double>() == Approx(2.718));
    v.get<double>() = 4.1;
    REQUIRE(b == Approx(4.1));

    REQUIRE_THROWS_AS(v.get<int>() = 4,
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("storing reference wrappers to consts works")
{
    using variant_type = mapbox::util::variant<std::reference_wrapper<int const>, std::reference_wrapper<double const>>;

    int a = 1;
    variant_type v{std::cref(a)};
    REQUIRE(v.get<int const>() == 1);
    REQUIRE(v.get<int>() == 1);
    REQUIRE(mapbox::util::get<int const>(v) == 1);
    REQUIRE(mapbox::util::get<int>(v) == 1);
    REQUIRE_THROWS_AS(v.get<double const>(),
                      mapbox::util::bad_variant_access&);
    REQUIRE_THROWS_AS(mapbox::util::get<double const>(v),
                      mapbox::util::bad_variant_access&);

    double b = 3.141;
    v = std::cref(b);
    REQUIRE(v.get<double const>() == Approx(3.141));
    REQUIRE(mapbox::util::get<double const>(v) == Approx(3.141));
    REQUIRE_THROWS_AS(v.get<int const>(),
                      mapbox::util::bad_variant_access&);
    REQUIRE_THROWS_AS(mapbox::util::get<int const>(v),
                      mapbox::util::bad_variant_access&);
}

TEST_CASE("recursive wrapper")
{
    using variant_type = mapbox::util::variant<mapbox::util::recursive_wrapper<int>>;
    variant_type v(1);
    REQUIRE(v.is<int>());
    REQUIRE(v.get<int>() == 1);
}


TEST_CASE("variant : direct_type helper should match T, references (T&)  and const references (T const&) to the original type T)")
{
    using value = mapbox::util::variant<bool, std::uint64_t>;

    std::uint64_t u(1234);
    REQUIRE(value(u).is<std::uint64_t>()); // matches T

    std::uint64_t& ur(u);
    REQUIRE(value(ur).is<std::uint64_t>()); // matches T&

    std::uint64_t const& ucr(u);
    REQUIRE(value(ucr).is<std::uint64_t>()); // matches T const&
}
