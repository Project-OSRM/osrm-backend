
#include <array>

#include <test.hpp>

#include <protozero/types.hpp>

TEST_CASE("default constructed data_view") {
    const protozero::data_view view{};
    REQUIRE(view.data() == nullptr);
    REQUIRE(view.size() == 0); // NOLINT(readability-container-size-empty)
    REQUIRE(view.empty());
}

TEST_CASE("data_view from C string") {
    const protozero::data_view view{"foobar"};
    REQUIRE(view.data());
    REQUIRE(view.size() == 6);
    REQUIRE_FALSE(view.empty());
}

TEST_CASE("data_view from std::string") {
    const std::string str{"foobar"};
    const protozero::data_view view{str};
    REQUIRE(view.data());
    REQUIRE(view.size() == 6);
}

TEST_CASE("data_view from ptr, size") {
    const std::string str{"foobar"};
    const protozero::data_view view{str.data(), str.size()};
    REQUIRE(view.data());
    REQUIRE(view.size() == 6);
}

TEST_CASE("data_view from C array") {
    const char* str = "foobar";
    const protozero::data_view view{str};
    REQUIRE(view.data());
    REQUIRE(view.size() == 6);
}

TEST_CASE("data_view from std::array") {
    const std::array<char, 7> str{"foobar"};
    const protozero::data_view view{str.data(), 6};
    REQUIRE(view.data());
    REQUIRE(view.size() == 6);
}

TEST_CASE("convert data_view to std::string") {
    const protozero::data_view view{"foobar"};

    const std::string s = std::string(view);
    REQUIRE(s == "foobar");
    REQUIRE(std::string(view) == "foobar");
    REQUIRE(view.to_string() == "foobar");
}

#ifndef PROTOZERO_USE_VIEW
// This test only works with our own data_view implementation, because only
// that one contains the protozero_assert() which generates the exception.
TEST_CASE("converting default constructed data_view to string fails") {
    const protozero::data_view view{};
    REQUIRE_THROWS_AS(view.to_string(), const assert_error&);
}
#endif

TEST_CASE("swapping data_view") {
    protozero::data_view view1{"foo"};
    protozero::data_view view2{"bar"};

    REQUIRE(view1.to_string() == "foo");
    REQUIRE(view2.to_string() == "bar");

    using std::swap;
    swap(view1, view2);

    REQUIRE(view2.to_string() == "foo");
    REQUIRE(view1.to_string() == "bar");
}

TEST_CASE("comparing data_views") {
    const protozero::data_view v1{"foo"};
    const protozero::data_view v2{"bar"};
    const protozero::data_view v3{"foox"};
    const protozero::data_view v4{"foo"};
    const protozero::data_view v5{"fooooooo", 3};
    const protozero::data_view v6{"f\0o", 3};
    const protozero::data_view v7{"f\0obar", 3};

    REQUIRE_FALSE(v1 == v2);
    REQUIRE_FALSE(v1 == v3);
    REQUIRE(v1 == v4);
    REQUIRE(v1 == v5);
    REQUIRE_FALSE(v1 == v6);
    REQUIRE_FALSE(v1 == v7);
    REQUIRE_FALSE(v2 == v3);
    REQUIRE_FALSE(v2 == v4);
    REQUIRE_FALSE(v3 == v4);
    REQUIRE(v4 == v5);
    REQUIRE(v6 == v7);

    REQUIRE(v1 != v2);
    REQUIRE(v1 != v3);
    REQUIRE_FALSE(v1 != v4);
    REQUIRE_FALSE(v1 != v5);
    REQUIRE(v1 != v6);
    REQUIRE(v1 != v7);
    REQUIRE(v2 != v3);
    REQUIRE(v2 != v4);
    REQUIRE(v3 != v4);
    REQUIRE_FALSE(v4 != v5);
    REQUIRE_FALSE(v6 != v7);
}

TEST_CASE("ordering of data_views") {
    const protozero::data_view v1{"foo"};
    const protozero::data_view v2{"foo"};
    const protozero::data_view v3{"bar"};
    const protozero::data_view v4{"foox"};
    const protozero::data_view v5{"zzz"};

    REQUIRE(v1.compare(v1) == 0);
    REQUIRE(v1.compare(v2) == 0);
    REQUIRE(v1.compare(v3) > 0);
    REQUIRE(v1.compare(v4) < 0);
    REQUIRE(v1.compare(v5) < 0);

    REQUIRE(v2.compare(v1) == 0);
    REQUIRE(v2.compare(v2) == 0);
    REQUIRE(v2.compare(v3) > 0);
    REQUIRE(v2.compare(v4) < 0);
    REQUIRE(v2.compare(v5) < 0);

    REQUIRE(v3.compare(v1) < 0);
    REQUIRE(v3.compare(v2) < 0);
    REQUIRE(v3.compare(v3) == 0);
    REQUIRE(v3.compare(v4) < 0);
    REQUIRE(v3.compare(v5) < 0);

    REQUIRE(v4.compare(v1) > 0);
    REQUIRE(v4.compare(v2) > 0);
    REQUIRE(v4.compare(v3) > 0);
    REQUIRE(v4.compare(v4) == 0);
    REQUIRE(v4.compare(v5) < 0);

    REQUIRE(v5.compare(v1) > 0);
    REQUIRE(v5.compare(v2) > 0);
    REQUIRE(v5.compare(v3) > 0);
    REQUIRE(v5.compare(v4) > 0);
    REQUIRE(v5.compare(v5) == 0);

    REQUIRE(v1 == v2);
    REQUIRE(v1 <= v2);
    REQUIRE(v1 >= v2);
    REQUIRE(v1 < v4);
    REQUIRE(v3 < v1);
    REQUIRE(v3 <= v1);
    REQUIRE(v5 > v1);
    REQUIRE(v5 >= v1);
}

