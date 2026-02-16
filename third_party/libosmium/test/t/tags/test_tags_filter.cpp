#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/tags/tags_filter.hpp>
#include <osmium/util/string_matcher.hpp>

#include <functional>
#include <iterator>
#include <string>

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
        REQUIRE(std::string{"highway"} == it->key());
        REQUIRE(std::string{"primary"} == it->value());
        ++it;
        REQUIRE(std::string{"source"} == it->key());
        REQUIRE(std::string{"GPS"} == it->value());
        REQUIRE(++it == end);
    }

}

namespace {

    struct result_type {

        int v = 0;
        bool b = false;

        result_type() noexcept = default;

        result_type(int v_, bool b_) noexcept :
            v(v_),
            b(b_) {
        }

        explicit operator bool() const noexcept {
            return b;
        }

    }; // struct result_type

    bool operator==(const result_type& lhs, const result_type& rhs) noexcept {
        return lhs.v == rhs.v && lhs.b == rhs.b;
    }

} // anonymous namespace

TEST_CASE("TagsFilterBase") {
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
        osmium::TagsFilterBase<result_type> filter;
        filter.add_rule(result_type{1, true}, osmium::TagMatcher{osmium::StringMatcher::equal{"highway"}});
        filter.add_rule(result_type{2, true}, osmium::TagMatcher{osmium::StringMatcher::equal{"amenity"}});
        REQUIRE(filter(*tag_list1.begin()) == result_type(1, true));
        REQUIRE(filter(*tag_list2.begin()) == result_type(2, true));
        REQUIRE(filter(*std::next(tag_list1.begin())) == result_type(0, false));
        REQUIRE(filter(*std::next(tag_list2.begin())) == result_type(0, false));
    }

    SECTION("Filter based string: shortcut") {
        osmium::TagsFilterBase<result_type> filter;
        filter.add_rule({3, true}, "highway");
        filter.add_rule({4, true}, "amenity", "restaurant");
        REQUIRE(filter(*tag_list1.begin()) == result_type(3, true));
        REQUIRE(filter(*tag_list2.begin()) == result_type(4, true));
        REQUIRE(filter(*std::next(tag_list1.begin())) == result_type(0, false));
        REQUIRE(filter(*std::next(tag_list2.begin())) == result_type(0, false));
    }

    SECTION("Filter based on key only: fail") {
        osmium::TagsFilterBase<result_type> filter;
        filter.add_rule({5, true}, osmium::StringMatcher::equal{"foo"});
        filter.add_rule({6, true}, osmium::StringMatcher::equal{"bar"});
        REQUIRE(filter(*tag_list1.begin()) == result_type(0, false));
        REQUIRE(filter(*tag_list2.begin()) == result_type(0, false));
        REQUIRE(filter(*std::next(tag_list1.begin())) == result_type(0, false));
        REQUIRE(filter(*std::next(tag_list2.begin())) == result_type(0, false));
    }

    SECTION("KeyFilter iterator filters tags") {
        osmium::TagsFilterBase<result_type> filter;
        filter.add_rule({7, true}, osmium::StringMatcher::equal{"highway"})
              .add_rule({8, true}, osmium::StringMatcher::equal{"source"});

        using iterator = osmium::TagsFilterBase<result_type>::iterator;

        iterator it{std::cref(filter), tag_list1.begin(), tag_list1.end()};

        const iterator end{std::cref(filter), tag_list1.end(), tag_list1.end()};

        REQUIRE(2 == std::distance(it, end));

        REQUIRE(it != end);
        REQUIRE(std::string{"highway"} == it->key());
        REQUIRE(std::string{"primary"} == it->value());
        ++it;
        REQUIRE(std::string{"source"} == it->key());
        REQUIRE(std::string{"GPS"} == it->value());
        REQUIRE(++it == end);
    }

}

TEST_CASE("compare_tags") {
    osmium::memory::Buffer buffer{10240};

    const auto pos1 = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "highway", "primary" },
            { "name", "Main Street" },
            { "source", "GPS" }
    }));
    const auto pos2 = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "highway", "primary" },
            { "name", "Main Street" },
    }));
    const auto pos3 = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "attribution", "something" },
            { "highway", "primary" },
            { "name", "Main Street" },
    }));
    const auto pos4 = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "highway", "primary" },
            { "junk", "foo" },
            { "name", "Main Street" },
    }));
    const auto pos5 = osmium::builder::add_tag_list(buffer,
        osmium::builder::attr::_tags({
            { "amenity", "restaurant" },
            { "name", "The Golden Goose" }
    }));

    const osmium::TagList& tag_list1 = buffer.get<osmium::TagList>(pos1);
    const osmium::TagList& tag_list2 = buffer.get<osmium::TagList>(pos2);
    const osmium::TagList& tag_list3 = buffer.get<osmium::TagList>(pos3);
    const osmium::TagList& tag_list4 = buffer.get<osmium::TagList>(pos4);
    const osmium::TagList& tag_list5 = buffer.get<osmium::TagList>(pos5);

    const osmium::TagsFilter filter_empty{true};

    REQUIRE(compare_tags(tag_list1, tag_list1, filter_empty));
    REQUIRE(compare_tags(tag_list2, tag_list2, filter_empty));
    REQUIRE(compare_tags(tag_list3, tag_list3, filter_empty));
    REQUIRE(compare_tags(tag_list4, tag_list4, filter_empty));
    REQUIRE(compare_tags(tag_list5, tag_list5, filter_empty));

    REQUIRE_FALSE(compare_tags(tag_list1, tag_list2, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list1, tag_list3, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list1, tag_list4, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list1, tag_list5, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list2, tag_list3, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list2, tag_list4, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list2, tag_list5, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list3, tag_list4, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list3, tag_list5, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list4, tag_list5, filter_empty));

    REQUIRE_FALSE(compare_tags(tag_list2, tag_list1, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list3, tag_list1, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list4, tag_list1, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list1, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list3, tag_list2, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list4, tag_list2, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list2, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list4, tag_list3, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list3, filter_empty));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list4, filter_empty));

    osmium::TagsFilter filter{true};
    filter.add_rule(false, "attribution");
    filter.add_rule(false, "source");
    filter.add_rule(false, "junk");

    REQUIRE(compare_tags(tag_list1, tag_list1, filter));
    REQUIRE(compare_tags(tag_list2, tag_list2, filter));
    REQUIRE(compare_tags(tag_list3, tag_list3, filter));
    REQUIRE(compare_tags(tag_list4, tag_list4, filter));
    REQUIRE(compare_tags(tag_list5, tag_list5, filter));

    REQUIRE(compare_tags(tag_list1, tag_list2, filter));
    REQUIRE(compare_tags(tag_list1, tag_list3, filter));
    REQUIRE(compare_tags(tag_list1, tag_list4, filter));
    REQUIRE_FALSE(compare_tags(tag_list1, tag_list5, filter));
    REQUIRE(compare_tags(tag_list2, tag_list3, filter));
    REQUIRE(compare_tags(tag_list2, tag_list4, filter));
    REQUIRE_FALSE(compare_tags(tag_list2, tag_list5, filter));
    REQUIRE(compare_tags(tag_list3, tag_list4, filter));
    REQUIRE_FALSE(compare_tags(tag_list3, tag_list5, filter));
    REQUIRE_FALSE(compare_tags(tag_list4, tag_list5, filter));

    REQUIRE(compare_tags(tag_list2, tag_list1, filter));
    REQUIRE(compare_tags(tag_list3, tag_list1, filter));
    REQUIRE(compare_tags(tag_list4, tag_list1, filter));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list1, filter));
    REQUIRE(compare_tags(tag_list3, tag_list2, filter));
    REQUIRE(compare_tags(tag_list4, tag_list2, filter));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list2, filter));
    REQUIRE(compare_tags(tag_list4, tag_list3, filter));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list3, filter));
    REQUIRE_FALSE(compare_tags(tag_list5, tag_list4, filter));
}

