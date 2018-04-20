#include "catch.hpp"

#include <osmium/util/options.hpp>

TEST_CASE("Set a single option value from string") {
    osmium::Options o;

    o.set("foo", "bar");
    REQUIRE("bar" == o.get("foo"));
    REQUIRE(o.get("empty").empty());
    REQUIRE("default" == o.get("empty", "default"));

    REQUIRE_FALSE(o.is_true("foo"));
    REQUIRE_FALSE(o.is_true("empty"));

    REQUIRE(o.is_not_false("foo"));
    REQUIRE(o.is_not_false("empty"));

    REQUIRE(1 == o.size());
}

TEST_CASE("Set option values from booleans") {
    osmium::Options o;

    o.set("t", true);
    o.set("f", false);
    REQUIRE("true" == o.get("t"));
    REQUIRE("false" == o.get("f"));
    REQUIRE(o.get("empty").empty());

    REQUIRE(o.is_true("t"));
    REQUIRE_FALSE(o.is_true("f"));

    REQUIRE(o.is_not_false("t"));
    REQUIRE_FALSE(o.is_not_false("f"));

    REQUIRE(2 == o.size());
}

TEST_CASE("Set option value from string with equal sign") {
    osmium::Options o;

    o.set("foo=bar");
    REQUIRE("bar" == o.get("foo"));
    REQUIRE(1 == o.size());
}

TEST_CASE("Set option value from string without equal sign") {
    osmium::Options o;

    o.set("foo");
    REQUIRE("true" == o.get("foo"));

    REQUIRE(o.is_true("foo"));
    REQUIRE(o.is_not_false("foo"));

    REQUIRE(1 == o.size());
}

TEST_CASE("Options with initializer list") {
    osmium::Options o{{ "foo", "true" }, { "bar", "17" }};

    REQUIRE(o.get("foo") == "true");
    REQUIRE(o.get("bar") == "17");
    REQUIRE(o.is_true("foo"));
    REQUIRE_FALSE(o.is_true("bar"));
    REQUIRE(o.size() == 2);

    SECTION("Change existing value") {
        o.set("foo", "false");
        REQUIRE_FALSE(o.is_true("foo"));
    }

    SECTION("Add new value") {
        o.set("new", "something");
        REQUIRE_FALSE(o.is_true("new"));
        REQUIRE(o.get("new") == "something");
    }
}

TEST_CASE("Iterating over options") {
    /*not const*/ osmium::Options o{{ "foo", "true" }, { "bar", "17" }};

    auto it = o.begin();
    REQUIRE(it->first == "bar");
    REQUIRE(it->second == "17");
    ++it;
    REQUIRE(it->first == "foo");
    REQUIRE(it->second == "true");
    ++it;
    REQUIRE(it == o.end());
}

TEST_CASE("Const iterating over options") {
    const osmium::Options o{{ "foo", "true" }, { "bar", "17" }};

    SECTION("begin/end") {
        auto it = o.begin();
        REQUIRE(it->first == "bar");
        REQUIRE(it->second == "17");
        ++it;
        REQUIRE(it->first == "foo");
        REQUIRE(it->second == "true");
        ++it;
        REQUIRE(it == o.end());
    }
    SECTION("cbegin/cend") {
        auto it = o.cbegin();
        REQUIRE(it->first == "bar");
        REQUIRE(it->second == "17");
        ++it;
        REQUIRE(it->first == "foo");
        REQUIRE(it->second == "true");
        ++it;
        REQUIRE(it == o.cend());
    }
}

