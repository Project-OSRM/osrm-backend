#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/builder/builder_helper.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/tag.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("create tag list") {
    osmium::memory::Buffer buffer{10240};

    SECTION("with TagListBuilder from char*") {
        {
            osmium::builder::TagListBuilder builder(buffer);
            builder.add_tag("highway", "primary");
            builder.add_tag("name", "Main Street");
        }
        buffer.commit();
    }

    SECTION("with TagListBuilder from pair<const char*, const char*>") {
        {
            osmium::builder::TagListBuilder builder(buffer);
            builder.add_tag(std::pair<const char*, const char*>{"highway", "primary"});
            builder.add_tag("name", "Main Street");
        }
        buffer.commit();
    }

    SECTION("with TagListBuilder from pair<const char* const, const char*>") {
        {
            osmium::builder::TagListBuilder builder(buffer);
            builder.add_tag(std::pair<const char* const, const char*>{"highway", "primary"});
            builder.add_tag("name", "Main Street");
        }
        buffer.commit();
    }

    SECTION("with TagListBuilder from pair<const char*, const char* const>") {
        {
            osmium::builder::TagListBuilder builder(buffer);
            builder.add_tag(std::pair<const char*, const char* const>{"highway", "primary"});
            builder.add_tag("name", "Main Street");
        }
        buffer.commit();
    }

    SECTION("with TagListBuilder from pair<const char* const, const char* const>") {
        {
            osmium::builder::TagListBuilder builder(buffer);
            builder.add_tag(std::pair<const char* const, const char* const>{"highway", "primary"});
            builder.add_tag("name", "Main Street");
        }
        buffer.commit();
    }

    SECTION("with TagListBuilder from char* with length") {
        {
            osmium::builder::TagListBuilder builder(buffer);
            builder.add_tag("highway", strlen("highway"), "primary", strlen("primary"));
            builder.add_tag("nameXX", 4, "Main Street", 11);
        }
        buffer.commit();
    }

    SECTION("with TagListBuilder from std::string") {
        {
            osmium::builder::TagListBuilder builder(buffer);
            builder.add_tag(std::string("highway"), std::string("primary"));
            const std::string source = "name";
            std::string gps = "Main Street";
            builder.add_tag(source, gps);
        }
        buffer.commit();
    }

    SECTION("with add_tag_list from pair<const char*, const char*>") {
        osmium::builder::add_tag_list(buffer,
            _tag(std::pair<const char*, const char*>{"highway", "primary"}),
            _tag("name", "Main Street")
        );
    }

    SECTION("with add_tag_list from pair<const char* const, const char*>") {
        osmium::builder::add_tag_list(buffer,
            _tag(std::pair<const char* const, const char*>{"highway", "primary"}),
            _tag("name", "Main Street")
        );
    }

    SECTION("with add_tag_list from pair<const char*, const char* const>") {
        osmium::builder::add_tag_list(buffer,
            _tag(std::pair<const char*, const char* const>{"highway", "primary"}),
            _tag("name", "Main Street")
        );
    }

    SECTION("with add_tag_list from pair<const char* const, const char* const>") {
        osmium::builder::add_tag_list(buffer,
            _tag(std::pair<const char* const, const char* const>{"highway", "primary"}),
            _tag("name", "Main Street")
        );
    }

    SECTION("with add_tag_list from vector of pairs (const/const)") {
        std::vector<std::pair<const char* const, const char* const>> v{
            { "highway", "primary" },
            { "name", "Main Street" }
        };
        osmium::builder::add_tag_list(buffer, _tags(v));
    }

    SECTION("with add_tag_list from vector of pairs (const/nc)") {
        std::vector<std::pair<const char* const, const char*>> v{
            { "highway", "primary" },
            { "name", "Main Street" }
        };
        osmium::builder::add_tag_list(buffer, _tags(v));
    }

    SECTION("with add_tag_list from vector of pairs (nc/const)") {
        std::vector<std::pair<const char*, const char* const>> v{
            { "highway", "primary" },
            { "name", "Main Street" }
        };
        osmium::builder::add_tag_list(buffer, _tags(v));
    }

    SECTION("with add_tag_list from vector of pairs (nc/nc)") {
        std::vector<std::pair<const char*, const char*>> v{
            { "highway", "primary" },
            { "name", "Main Street" }
        };
        osmium::builder::add_tag_list(buffer, _tags(v));
    }

    SECTION("with add_tag_list from initializer list") {
        osmium::builder::add_tag_list(buffer, _tags({
            { "highway", "primary" },
            { "name", "Main Street" }
        }));
    }

    SECTION("with add_tag_list from _tag") {
        osmium::builder::add_tag_list(buffer,
            _tag("highway", "primary"),
            _tag("name", "Main Street")
        );
    }

    SECTION("with add_tag_list from map") {
        std::map<const char*, const char*> m{
            { "highway", "primary" },
            { "name", "Main Street" }
        };
        osmium::builder::add_tag_list(buffer, _tags(m));
    }

    const osmium::TagList& tl = *buffer.select<osmium::TagList>().cbegin();
    REQUIRE(osmium::item_type::tag_list == tl.type());
    REQUIRE(2 == tl.size());

    REQUIRE(tl.has_key("highway"));
    REQUIRE_FALSE(tl.has_key("unknown"));
    REQUIRE(tl.has_tag("highway", "primary"));
    REQUIRE_FALSE(tl.has_tag("highway", "false"));
    REQUIRE_FALSE(tl.has_tag("foo", "bar"));

    auto it = tl.begin();
    REQUIRE(std::string("highway")     == it->key());
    REQUIRE(std::string("primary")     == it->value());
    ++it;
    REQUIRE(std::string("name")        == it->key());
    REQUIRE(std::string("Main Street") == it->value());
    ++it;
    REQUIRE(it == tl.end());

    REQUIRE(std::string("primary") == tl.get_value_by_key("highway"));
    REQUIRE(nullptr == tl.get_value_by_key("foo"));
    REQUIRE(std::string("default") == tl.get_value_by_key("foo", "default"));

    REQUIRE(std::string("Main Street") == tl["name"]);
}

TEST_CASE("empty keys and values are okay") {
    osmium::memory::Buffer buffer{10240};

    const auto pos = osmium::builder::add_tag_list(buffer,
        _tag("empty value", ""),
        _tag("", "empty key")
    );
    const osmium::TagList& tl = buffer.get<osmium::TagList>(pos);

    REQUIRE(osmium::item_type::tag_list == tl.type());
    REQUIRE(2 == tl.size());

    auto it = tl.begin();
    REQUIRE(std::string("empty value") == it->key());
    REQUIRE(std::string("")            == it->value());
    ++it;
    REQUIRE(std::string("")            == it->key());
    REQUIRE(std::string("empty key")   == it->value());
    ++it;
    REQUIRE(it == tl.end());

    REQUIRE(std::string("") == tl.get_value_by_key("empty value"));
    REQUIRE(std::string("empty key") == tl.get_value_by_key(""));
}

TEST_CASE("tag key or value is too long") {
    osmium::memory::Buffer buffer{10240};
    osmium::builder::TagListBuilder builder{buffer};

    const char kv[2000] = "";
    builder.add_tag(kv, 1, kv, 1000);
    REQUIRE_THROWS(builder.add_tag(kv, 1500, kv, 1));
    REQUIRE_THROWS(builder.add_tag(kv, 1, kv, 1500));
}

