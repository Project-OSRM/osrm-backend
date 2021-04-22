#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/tags/filter.hpp>
#include <osmium/tags/regex_filter.hpp>
#include <osmium/tags/taglist.hpp>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <regex>
#include <utility>
#include <vector>

template <class TFilter>
void check_filter(const osmium::TagList& tag_list,
                  const TFilter filter,
                  const std::vector<bool>& reference) {
    REQUIRE(tag_list.size() == reference.size());
    auto t_it = tag_list.begin();
    for (auto it = reference.begin(); it != reference.end(); ++t_it, ++it) {
        REQUIRE(filter(*t_it) == *it);
    }

    typename TFilter::iterator fi_begin{filter, tag_list.begin(), tag_list.end()};
    typename TFilter::iterator fi_end{filter, tag_list.end(), tag_list.end()};

    REQUIRE(std::distance(fi_begin, fi_end) == std::count(reference.begin(), reference.end(), true));
}

const osmium::TagList& make_tag_list(osmium::memory::Buffer& buffer,
                                     const std::initializer_list<std::pair<const char*, const char*>>& tags) {
    const auto pos = osmium::builder::add_tag_list(buffer, osmium::builder::attr::_tags(tags));
    return buffer.get<osmium::TagList>(pos);
}


TEST_CASE("KeyFilter") {
    osmium::memory::Buffer buffer{10240};
    osmium::tags::KeyFilter filter{false};

    const osmium::TagList& tag_list = make_tag_list(buffer, {
        { "highway", "primary" },
        { "name", "Main Street" },
        { "source", "GPS" }
    });

    SECTION("KeyFilter matches some tags") {
        filter.add(true, "highway")
              .add(true, "railway");

        const std::vector<bool> results = { true, false, false };

        check_filter(tag_list, filter, results);
    }

    SECTION("KeyFilter iterator filters tags") {
        filter.add(true, "highway")
              .add(true, "source");

        osmium::tags::KeyFilter::iterator it{filter, tag_list.begin(),
                                                     tag_list.end()};

        const osmium::tags::KeyFilter::iterator end{filter, tag_list.end(),
                                                            tag_list.end()};

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

TEST_CASE("KeyValueFilter") {
    osmium::memory::Buffer buffer{10240};

    SECTION("KeyValueFilter matches some tags") {
        osmium::tags::KeyValueFilter filter{false};

        filter.add(true, "highway", "residential")
              .add(true, "highway", "primary")
              .add(true, "railway");

        const osmium::TagList& tag_list = make_tag_list(buffer, {
            { "highway", "primary" },
            { "railway", "tram" },
            { "source", "GPS" }
        });

        const std::vector<bool> results = {true, true, false};

        check_filter(tag_list, filter, results);
    }

    SECTION("KeyValueFilter ordering matters") {
        osmium::tags::KeyValueFilter filter1(false);
        filter1.add(true, "highway")
               .add(false, "highway", "road");

        osmium::tags::KeyValueFilter filter2(false);
        filter2.add(false, "highway", "road")
               .add(true, "highway");

        const osmium::TagList& tag_list1 = make_tag_list(buffer, {
            { "highway", "road" },
            { "name", "Main Street" }
        });

        const osmium::TagList& tag_list2 = make_tag_list(buffer, {
            { "highway", "primary" },
            { "name", "Main Street" }
        });

        check_filter(tag_list1, filter1, {true, false});
        check_filter(tag_list1, filter2, {false, false});
        check_filter(tag_list2, filter2, {true, false});
    }

    SECTION("KeyValueFilter matches against taglist with any") {
        osmium::tags::KeyValueFilter filter{false};

        filter.add(true, "highway", "primary")
              .add(true, "name");

        const osmium::TagList& tag_list = make_tag_list(buffer, {
            { "highway", "primary" },
            { "railway", "tram" },
            { "source", "GPS" }
        });

        REQUIRE(     osmium::tags::match_any_of(tag_list, filter));
        REQUIRE_FALSE(osmium::tags::match_all_of(tag_list, filter));
        REQUIRE_FALSE(osmium::tags::match_none_of(tag_list, filter));
    }

    SECTION("KeyValueFilter matches against taglist with_all") {
        osmium::tags::KeyValueFilter filter{false};

        filter.add(true, "highway", "primary")
              .add(true, "name");

        const osmium::TagList& tag_list = make_tag_list(buffer, {
            { "highway", "primary" },
            { "name", "Main Street" }
        });

        REQUIRE(      osmium::tags::match_any_of(tag_list, filter));
        REQUIRE(      osmium::tags::match_all_of(tag_list, filter));
        REQUIRE_FALSE(osmium::tags::match_none_of(tag_list, filter));
    }

    SECTION("KeyValueFilter matches against taglist with none") {
        osmium::tags::KeyValueFilter filter{false};

        filter.add(true, "highway", "road")
              .add(true, "source");

        const osmium::TagList& tag_list = make_tag_list(buffer, {
            { "highway", "primary" },
            { "name", "Main Street" }
        });

        REQUIRE_FALSE(osmium::tags::match_any_of(tag_list, filter));
        REQUIRE_FALSE(osmium::tags::match_all_of(tag_list, filter));
        REQUIRE(      osmium::tags::match_none_of(tag_list, filter));
    }

    SECTION("KeyValueFilter matches against taglist with any called with rvalue") {
        const osmium::TagList& tag_list = make_tag_list(buffer, {
            { "highway", "primary" },
            { "railway", "tram" },
            { "source", "GPS" }
        });

        REQUIRE(osmium::tags::match_any_of(tag_list,
                                           osmium::tags::KeyValueFilter()
                                               .add(true, "highway", "primary")
                                               .add(true, "name")));
    }

}

TEST_CASE("RegexFilter matches some tags") {
    osmium::memory::Buffer buffer{10240};

    osmium::tags::RegexFilter filter{false};
    filter.add(true, "highway", std::regex{".*_link"});

    const osmium::TagList& tag_list1 = make_tag_list(buffer, {
        { "highway", "primary_link" },
        { "source", "GPS" }
    });
    const osmium::TagList& tag_list2 = make_tag_list(buffer, {
        { "highway", "primary" },
        { "source", "GPS" }
    });

    check_filter(tag_list1, filter, {true, false});
    check_filter(tag_list2, filter, {false, false});
}

TEST_CASE("RegexFilter matches some tags with lvalue regex") {
    osmium::memory::Buffer buffer{10240};
    osmium::tags::RegexFilter filter{false};
    std::regex r{".*straße"};
    filter.add(true, "name", r);

    const osmium::TagList& tag_list = make_tag_list(buffer, {
        { "highway", "primary" },
        { "name", "Hauptstraße" }
    });

    check_filter(tag_list, filter, {false, true});
}

TEST_CASE("KeyPrefixFilter matches some keys") {
    osmium::memory::Buffer buffer{10240};

    osmium::tags::KeyPrefixFilter filter{false};
    filter.add(true, "name:");

    const osmium::TagList& tag_list = make_tag_list(buffer, {
        { "highway", "primary" },
        { "name:de", "Hauptstraße" }
    });

    check_filter(tag_list, filter, {false, true});

}

TEST_CASE("Generic Filter with regex matches some keys") {
    osmium::memory::Buffer buffer{10240};

    osmium::tags::Filter<std::regex> filter{false};
    filter.add(true, std::regex{"restriction.+conditional"});

    const osmium::TagList& tag_list = make_tag_list(buffer, {
        { "highway", "primary" },
        { "restrictionconditional", "only_right_turn @ (Mo-Fr 07:00-14:00)" },
        { "restriction:conditional", "only_right_turn @ (Mo-Fr 07:00-14:00)" },
        { "restriction:psv:conditional", "only_right_turn @ (Mo-Fr 07:00-14:00)" }
    });

    check_filter(tag_list, filter, {false, false, true, true});

}
