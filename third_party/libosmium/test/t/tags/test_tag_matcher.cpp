#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/tags/matcher.hpp>

#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

static_assert(std::is_default_constructible<osmium::TagMatcher>::value, "TagMatcher should be default constructible");
static_assert(std::is_copy_constructible<osmium::TagMatcher>::value, "TagMatcher should be copy constructible");
static_assert(std::is_move_constructible<osmium::TagMatcher>::value, "TagMatcher should be move constructible");
static_assert(std::is_copy_assignable<osmium::TagMatcher>::value, "TagMatcher should be copyable");
static_assert(std::is_move_assignable<osmium::TagMatcher>::value, "TagMatcher should be moveable");

TEST_CASE("Tag matcher") {
    osmium::memory::Buffer buffer{10240};

    const auto pos = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "highway", "primary" },
            { "name", "Main Street" },
            { "source", "GPS" }
    }));
    const osmium::TagList& tag_list = buffer.get<osmium::TagList>(pos);

    SECTION("Matching nothing (default constructor)") {
        osmium::TagMatcher m{};
        REQUIRE_FALSE(m(tag_list));

        REQUIRE_FALSE(m(*tag_list.begin()));
    }

    SECTION("Matching nothing (bool)") {
        osmium::TagMatcher m{false};
        REQUIRE_FALSE(m(tag_list));

        REQUIRE_FALSE(m(*tag_list.begin()));
    }

    SECTION("Matching everything") {
        osmium::TagMatcher m{true};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE(m(*std::next(tag_list.begin())));
        REQUIRE(m(*std::next(std::next(tag_list.begin()))));
    }

    SECTION("Matching keys only") {
        osmium::TagMatcher m{osmium::StringMatcher::equal{"highway"}};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE_FALSE(m(*std::next(tag_list.begin())));
    }

    SECTION("Matching keys only with shortcut const char*") {
        osmium::TagMatcher m{"highway"};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE_FALSE(m(*std::next(tag_list.begin())));
    }

    SECTION("Matching keys only with shortcut std::string") {
        std::string s{"highway"};
        osmium::TagMatcher m{s};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE_FALSE(m(*std::next(tag_list.begin())));
    }

    SECTION("Matching key and value") {
        osmium::TagMatcher m{osmium::StringMatcher::equal{"highway"},
                             osmium::StringMatcher::equal{"primary"}};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE_FALSE(m(*std::next(tag_list.begin())));
    }

    SECTION("Matching key and value with shortcut") {
        osmium::TagMatcher m{"highway", "primary", false};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE_FALSE(m(*std::next(tag_list.begin())));
    }

    SECTION("Matching key and value") {
        osmium::TagMatcher m{osmium::StringMatcher::equal{"highway"},
                             osmium::StringMatcher::equal{"secondary"}};
        REQUIRE_FALSE(m(tag_list));
    }

    SECTION("Matching key and value inverted") {
        osmium::TagMatcher m{osmium::StringMatcher::equal{"highway"},
                             osmium::StringMatcher::equal{"secondary"},
                             true};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE_FALSE(m(*std::next(tag_list.begin())));
    }

    SECTION("Matching key and value list") {
        osmium::TagMatcher m{osmium::StringMatcher::equal{"highway"},
                             osmium::StringMatcher::list{{"primary", "secondary"}}};
        REQUIRE(m(tag_list));

        REQUIRE(m(*tag_list.begin()));
        REQUIRE_FALSE(m(*std::next(tag_list.begin())));
    }
}

TEST_CASE("Copy and move tag matcher") {
    osmium::TagMatcher m1{"highway"};
    osmium::TagMatcher c1{true};
    osmium::TagMatcher c2{false};

    auto m2 = m1;

    REQUIRE(m2("highway", "residential"));
    REQUIRE_FALSE(m2("name", "High Street"));

    c1 = m1;
    REQUIRE(c1("highway", "residential"));
    REQUIRE_FALSE(c1("name", "High Street"));

    auto m3 = std::move(m2);

    REQUIRE(m3("highway", "residential"));
    REQUIRE_FALSE(m3("name", "High Street"));

    c1 = std::move(c2);
    REQUIRE_FALSE(c1("highway", "residential"));
    REQUIRE_FALSE(c1("name", "High Street"));
}

