#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/tags/tags_filter.hpp>

#include <functional>

TEST_CASE("Tags filter") {
    osmium::memory::Buffer buffer{10240};

    const auto pos1 = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "highway", "primary" },
            { "name", "Main Street" },
            { "source", "GPS" }
    }));
    const auto pos2 = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "amenity", "restaurant" },
            { "name", "The Golden Goose" }
    }));
    const osmium::TagList& tag_list1 = buffer.get<osmium::TagList>(pos1);
    const osmium::TagList& tag_list2 = buffer.get<osmium::TagList>(pos2);

    SECTION("Filter based on key only: okay") {
        osmium::TagsFilter filter;
        filter.add_rule(true, osmium::TagMatcher{osmium::StringMatcher::equal{"highway"}});
        filter.add_rule(true, osmium::TagMatcher{osmium::StringMatcher::equal{"amenity"}});
        REQUIRE(filter(*tag_list1.begin()));
        REQUIRE(filter(*tag_list2.begin()));
        REQUIRE_FALSE(filter(*std::next(tag_list1.begin())));
        REQUIRE_FALSE(filter(*std::next(tag_list2.begin())));
    }

    SECTION("Filter based string: shortcut") {
        osmium::TagsFilter filter;
        filter.add_rule(true, "highway");
        filter.add_rule(true, "amenity", "restaurant");
        REQUIRE(filter(*tag_list1.begin()));
        REQUIRE(filter(*tag_list2.begin()));
        REQUIRE_FALSE(filter(*std::next(tag_list1.begin())));
        REQUIRE_FALSE(filter(*std::next(tag_list2.begin())));
    }

    SECTION("Filter based on key only: fail") {
        osmium::TagsFilter filter;
        filter.add_rule(true, osmium::StringMatcher::equal{"foo"});
        filter.add_rule(true, osmium::StringMatcher::equal{"bar"});
        REQUIRE_FALSE(filter(*tag_list1.begin()));
        REQUIRE_FALSE(filter(*tag_list2.begin()));
        REQUIRE_FALSE(filter(*std::next(tag_list1.begin())));
        REQUIRE_FALSE(filter(*std::next(tag_list2.begin())));
    }

    SECTION("KeyFilter iterator filters tags") {
        osmium::TagsFilter filter;
        filter.add_rule(true, osmium::StringMatcher::equal{"highway"})
              .add_rule(true, osmium::StringMatcher::equal{"source"});

        osmium::TagsFilter::iterator it{std::cref(filter), tag_list1.begin(),
                                                           tag_list1.end()};

        const osmium::TagsFilter::iterator end{std::cref(filter), tag_list1.end(),
                                                                  tag_list1.end()};

        REQUIRE(2 == std::distance(it, end));

        REQUIRE(it != end);
        REQUIRE(std::string("highway") == it->key());
        REQUIRE(std::string("primary") == it->value());
        ++it;
        REQUIRE(std::string("source") == it->key());
        REQUIRE(std::string("GPS") == it->value());
        REQUIRE(++it == end);
    }

}

