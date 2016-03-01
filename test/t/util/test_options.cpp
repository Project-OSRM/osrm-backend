#include "catch.hpp"

#include <iterator>

#include <osmium/util/options.hpp>

TEST_CASE("Options") {

    osmium::util::Options o;

    SECTION("set a single value from string") {
        o.set("foo", "bar");
        REQUIRE("bar" == o.get("foo"));
        REQUIRE("" == o.get("empty"));
        REQUIRE("default" == o.get("empty", "default"));

        REQUIRE(!o.is_true("foo"));
        REQUIRE(!o.is_true("empty"));

        REQUIRE(o.is_not_false("foo"));
        REQUIRE(o.is_not_false("empty"));

        REQUIRE(1 == o.size());
    }

    SECTION("set values from booleans") {
        o.set("t", true);
        o.set("f", false);
        REQUIRE("true" == o.get("t"));
        REQUIRE("false" == o.get("f"));
        REQUIRE("" == o.get("empty"));

        REQUIRE(o.is_true("t"));
        REQUIRE(!o.is_true("f"));

        REQUIRE(o.is_not_false("t"));
        REQUIRE(!o.is_not_false("f"));

        REQUIRE(2 == o.size());
    }

    SECTION("set value from string with equal sign") {
        o.set("foo=bar");
        REQUIRE("bar" == o.get("foo"));
        REQUIRE(1 == o.size());
    }

    SECTION("set value from string without equal sign") {
        o.set("foo");
        REQUIRE("true" == o.get("foo"));

        REQUIRE(o.is_true("foo"));
        REQUIRE(o.is_not_false("foo"));

        REQUIRE(1 == o.size());
    }

}

TEST_CASE("Options with initializer list") {

    osmium::util::Options o{ { "foo", "true" }, { "bar", "17" } };

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

