#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/types.hpp>

#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

constexpr const std::size_t test_buffer_size = 1024UL * 10UL;

TEST_CASE("create node using builders: add node with only id") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const auto pos = osmium::builder::add_node(buffer, _id(22));

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 22);
    REQUIRE(node.version() == 0);
    REQUIRE(node.timestamp() == osmium::Timestamp{});
    REQUIRE(node.changeset() == 0);
    REQUIRE(node.uid() == 0);
    REQUIRE(node.user()[0] == '\0');
    REQUIRE(node.location() == osmium::Location{});
    REQUIRE(node.tags().empty());
}

TEST_CASE("create node using builders: add node with complete info but no tags") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const osmium::Location loc{3.14, 1.59};
    const auto pos = osmium::builder::add_node(buffer,
        _id(1),
        _version(17),
        _timestamp(osmium::Timestamp{"2015-01-01T10:20:30Z"}),
        _cid(21),
        _uid(222),
        _location(loc),
        _user("foo")
    );

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 1);
    REQUIRE(node.version() == 17);
    REQUIRE(node.timestamp() == osmium::Timestamp{"2015-01-01T10:20:30Z"});
    REQUIRE(node.changeset() == 21);
    REQUIRE(node.uid() == 222);
    REQUIRE(std::string{node.user()} == "foo");
    REQUIRE(node.location() == loc);
    REQUIRE(node.tags().empty());
    REQUIRE(std::distance(node.cbegin(), node.cend()) == 0);
}

TEST_CASE("create node using builders: visible/deleted flag") {
    osmium::memory::Buffer buffer{test_buffer_size};

    osmium::builder::add_node(buffer, _id(1), _deleted());
    osmium::builder::add_node(buffer, _id(2), _deleted(true));
    osmium::builder::add_node(buffer, _id(3), _deleted(false));
    osmium::builder::add_node(buffer, _id(4), _visible());
    osmium::builder::add_node(buffer, _id(5), _visible(true));
    osmium::builder::add_node(buffer, _id(6), _visible(false));

    auto it = buffer.select<osmium::Node>().cbegin();
    REQUIRE_FALSE(it++->visible());
    REQUIRE_FALSE(it++->visible());
    REQUIRE(it++->visible());
    REQUIRE(it++->visible());
    REQUIRE(it++->visible());
    REQUIRE_FALSE(it++->visible());
    REQUIRE(it == buffer.select<osmium::Node>().cend());
}

TEST_CASE("create node using builders: order of attributes doesn't matter") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const osmium::Location loc{3.14, 1.59};
    const auto pos = osmium::builder::add_node(buffer,
        _timestamp("2015-01-01T10:20:30Z"),
        _version(17),
        _cid(21),
        _uid(222),
        _user(std::string{"foo"}),
        _id(1),
        _location(3.14, 1.59)
    );

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 1);
    REQUIRE(node.version() == 17);
    REQUIRE(node.timestamp() == osmium::Timestamp{"2015-01-01T10:20:30Z"});
    REQUIRE(node.changeset() == 21);
    REQUIRE(node.uid() == 222);
    REQUIRE(std::string{node.user()} == "foo");
    REQUIRE(node.location() == loc);
    REQUIRE(node.tags().empty());
}

TEST_CASE("create node with tags using builders: add tags using _tag") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const std::pair<const char*, const char*> t1 = {"name", "Node Inn"};
    const std::pair<std::string, std::string> t2 = {"phone", "+1-123-555-4567"};

    const auto pos = osmium::builder::add_node(buffer,
        _id(2),
        _tag("amenity", "restaurant"),
        _tag(t1),
        _tag(t2),
        _tag(std::string{"cuisine"}, std::string{"italian"})
    );

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 2);
    REQUIRE(node.tags().size() == 4);
    REQUIRE(std::distance(node.cbegin(), node.cend()) == 1);

    auto it = node.tags().cbegin();
    REQUIRE(std::string{it->key()} == "amenity");
    REQUIRE(std::string{it->value()} == "restaurant");
    ++it;
    REQUIRE(std::string{it->key()} == "name");
    REQUIRE(std::string{it->value()} == "Node Inn");
    ++it;
    REQUIRE(std::string{it->key()} == "phone");
    REQUIRE(std::string{it->value()} == "+1-123-555-4567");
    ++it;
    REQUIRE(std::string{it->key()} == "cuisine");
    REQUIRE(std::string{it->value()} == "italian");
    ++it;
    REQUIRE(it == node.tags().cend());
}

TEST_CASE("create node with tags using builders: add tags using _tag with equal sign in single cstring") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const auto pos = osmium::builder::add_node(buffer,
        _id(2),
        _tag("amenity=restaurant"),
        _tag("name="),
        _tag("phone"),
        _tag(std::string{"cuisine=italian"})
    );

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 2);
    REQUIRE(node.tags().size() == 4);
    REQUIRE(std::distance(node.cbegin(), node.cend()) == 1);

    auto it = node.tags().cbegin();
    REQUIRE(std::string{it->key()} == "amenity");
    REQUIRE(std::string{it->value()} == "restaurant");
    ++it;
    REQUIRE(std::string{it->key()} == "name");
    REQUIRE(it->value()[0] == '\0');
    ++it;
    REQUIRE(std::string{it->key()} == "phone");
    REQUIRE(it->value()[0] == '\0');
    ++it;
    REQUIRE(std::string{it->key()} == "cuisine");
    REQUIRE(std::string{it->value()} == "italian");
    ++it;
    REQUIRE(it == node.tags().cend());
}

TEST_CASE("create node with tags using builders: add tags using _tags from initializer list") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const auto pos = osmium::builder::add_node(buffer,
        _id(3),
        _tags({{"amenity", "post_box"}})
    );

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 3);
    REQUIRE(node.tags().size() == 1);

    auto it = node.tags().cbegin();
    REQUIRE(std::string{it->key()} == "amenity");
    REQUIRE(std::string{it->value()} == "post_box");
    ++it;
    REQUIRE(it == node.tags().cend());
    REQUIRE(std::distance(node.cbegin(), node.cend()) == 1);
}

TEST_CASE("create node with tags using builders: add tags using _tags from TagList") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const auto pos1 = osmium::builder::add_node(buffer,
        _id(3),
        _tag("a", "d"),
        _tag("b", "e"),
        _tag("c", "f")
    );

    const auto& node1 = buffer.get<osmium::Node>(pos1);

    const auto pos2 = osmium::builder::add_node(buffer,
        _id(4),
        _tags(node1.tags())
    );

    const auto& node2 = buffer.get<osmium::Node>(pos2);

    REQUIRE(node2.id() == 4);
    REQUIRE(node2.tags().size() == 3);

    auto it = node2.tags().cbegin();
    REQUIRE(std::string{it++->key()} == "a");
    REQUIRE(std::string{it++->key()} == "b");
    REQUIRE(std::string{it++->key()} == "c");
    REQUIRE(it == node2.tags().cend());
    REQUIRE(std::distance(node2.cbegin(), node2.cend()) == 1);
}

TEST_CASE("create node with tags using builders: add tags using mixed tag sources") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const std::vector<pair_of_cstrings> tags = {
        {"t5", "t5"},
        {"t6", "t6"}
    };

    const auto pos = osmium::builder::add_node(buffer,
        _id(4),
        _tag("t1=t1"),
        _tags({{"t2", "t2"}, {"t3", "t3"}}),
        _tag("t4", "t4"),
        _tags(tags)
    );

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 4);
    REQUIRE(node.tags().size() == 6);

    auto it = node.tags().cbegin();
    REQUIRE(std::string{it->key()} == "t1");
    ++it;
    REQUIRE(std::string{it->key()} == "t2");
    ++it;
    REQUIRE(std::string{it->key()} == "t3");
    ++it;
    REQUIRE(std::string{it->key()} == "t4");
    ++it;
    REQUIRE(std::string{it->key()} == "t5");
    ++it;
    REQUIRE(std::string{it->key()} == "t6");
    ++it;
    REQUIRE(it == node.tags().cend());
    REQUIRE(std::distance(node.cbegin(), node.cend()) == 1);
}

TEST_CASE("create node with tags using builders: add tags using _t with string") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const auto pos = osmium::builder::add_node(buffer,
        _id(5),
        _t("amenity=post_box,,empty,also_empty=,operator=Deutsche Post")
    );

    const auto& node = buffer.get<osmium::Node>(pos);

    REQUIRE(node.id() == 5);
    REQUIRE(node.tags().size() == 4);

    auto it = node.tags().cbegin();
    REQUIRE(std::string{it->key()} == "amenity");
    REQUIRE(std::string{it->value()} == "post_box");
    ++it;
    REQUIRE(std::string{it->key()} == "empty");
    REQUIRE(it->value()[0] == '\0');
    ++it;
    REQUIRE(std::string{it->key()} == "also_empty");
    REQUIRE(it->value()[0] == '\0');
    ++it;
    REQUIRE(std::string{it->key()} == "operator");
    REQUIRE(std::string{it->value()} == "Deutsche Post");
    ++it;
    REQUIRE(it == node.tags().cend());
    REQUIRE(std::distance(node.cbegin(), node.cend()) == 1);
}

TEST_CASE("create way using builders") {
    osmium::memory::Buffer buffer{test_buffer_size};

    SECTION("add way without nodes") {
        const auto pos = osmium::builder::add_way(buffer,
            _id(999),
            _cid(21),
            _uid(222),
            _user("foo")
        );

        const auto& way = buffer.get<osmium::Way>(pos);

        REQUIRE(way.id() == 999);
        REQUIRE(way.version() == 0);
        REQUIRE(way.timestamp() == osmium::Timestamp{});
        REQUIRE(way.changeset() == 21);
        REQUIRE(way.uid() == 222);
        REQUIRE(std::string{way.user()} == "foo");
        REQUIRE(way.tags().empty());
        REQUIRE(way.nodes().empty());
        REQUIRE(std::distance(way.cbegin(), way.cend()) == 0);
    }

}

TEST_CASE("create way with nodes") {
    std::vector<osmium::NodeRef> nrvec = {
        { 1, osmium::Location{1.1, 0.1} },
        { 2, osmium::Location{2.2, 0.2} },
        { 4, osmium::Location{4.4, 0.4} },
        { 8, osmium::Location{8.8, 0.8} }
    };

    osmium::memory::Buffer wbuffer{test_buffer_size};
    osmium::builder::add_way(wbuffer,
        _id(1),
        _nodes({1, 2, 4, 8})
    );

    const osmium::NodeRefList& nodes = wbuffer.get<osmium::Way>(0).nodes();

    osmium::memory::Buffer buffer{test_buffer_size};

    SECTION("add nodes using an OSM object id or NodeRef") {
        osmium::builder::add_way(buffer,
            _id(1),
            _node(1),
            _node(2),
            _node(osmium::NodeRef{4}),
            _node(8)
        );

    }

    SECTION("add nodes using iterator list with object ids") {
        osmium::builder::add_way(buffer,
            _id(1),
            _nodes({1, 2, 4, 8})
        );
    }

    SECTION("add way with nodes in initializer_list of NodeRefs") {
        osmium::builder::add_way(buffer,
            _id(1),
            _nodes({
                { 1, {1.1, 0.1} },
                { 2, {2.2, 0.2} },
                { 4, {4.4, 0.4} },
                { 8, {8.8, 0.8} }
            })
        );
    }

    SECTION("add nodes using WayNodeList") {
        osmium::builder::add_way(buffer,
            _id(1),
            _nodes(nodes)
        );
    }

    SECTION("add nodes using vector of OSM object ids") {
        const std::vector<osmium::object_id_type> some_nodes = {
            1, 2, 4, 8
        };

        osmium::builder::add_way(buffer,
            _id(1),
            _nodes(some_nodes)
        );
    }

    SECTION("add nodes using vector of NodeRefs") {
        osmium::builder::add_way(buffer,
            _id(1),
            _nodes(nrvec)
        );
    }

    SECTION("add nodes using different means together") {
        osmium::builder::add_way(buffer,
            _id(1),
            _node(1),
            _nodes({2, 4}),
            _node(8)
        );
    }

    SECTION("add nodes using different means together") {
        osmium::builder::add_way(buffer,
            _id(1),
            _nodes(nodes.begin(), nodes.begin() + 1),
            _nodes({2, 4, 8})
        );
    }

    const auto& way = buffer.get<osmium::Way>(0);

    REQUIRE(way.id() == 1);
    REQUIRE(way.nodes().size() == 4);
    REQUIRE(std::distance(way.cbegin(), way.cend()) == 1);

    const auto* it = way.nodes().cbegin();

    REQUIRE(it->ref() == 1);
    if (it->location().valid()) {
        REQUIRE(*it == nrvec[0]);
    }
    it++;

    REQUIRE(it->ref() == 2);
    if (it->location().valid()) {
        REQUIRE(*it == nrvec[1]);
    }
    it++;

    REQUIRE(it->ref() == 4);
    if (it->location().valid()) {
        REQUIRE(*it == nrvec[2]);
    }
    it++;

    REQUIRE(it->ref() == 8);
    if (it->location().valid()) {
        REQUIRE(*it == nrvec[3]);
    }
    it++;

    REQUIRE(it == way.nodes().cend());
}

TEST_CASE("create relation using builders: create relation") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const osmium::builder::attr::member_type m{osmium::item_type::way, 113, "inner"};

    osmium::builder::add_relation(buffer,
        _id(123),
        _member(osmium::item_type::node, 123, ""),
        _member(osmium::item_type::node, 132),
        _member(osmium::item_type::way, 111, "outer"),
        _member(osmium::builder::attr::member_type{osmium::item_type::way, 112, "inner"}),
        _member(m)
    );

    const auto& relation = buffer.get<osmium::Relation>(0);

    REQUIRE(relation.id() == 123);
    REQUIRE(relation.members().size() == 5);
    REQUIRE(std::distance(relation.cbegin(), relation.cend()) == 1);

    auto it = relation.members().begin();

    REQUIRE(it->type() == osmium::item_type::node);
    REQUIRE(it->ref() == 123);
    REQUIRE(it->role()[0] == '\0');
    ++it;

    REQUIRE(it->type() == osmium::item_type::node);
    REQUIRE(it->ref() == 132);
    REQUIRE(it->role()[0] == '\0');
    ++it;

    REQUIRE(it->type() == osmium::item_type::way);
    REQUIRE(it->ref() == 111);
    REQUIRE(std::string{it->role()} == "outer");
    ++it;

    REQUIRE(it->type() == osmium::item_type::way);
    REQUIRE(it->ref() == 112);
    REQUIRE(std::string{it->role()} == "inner");
    ++it;

    REQUIRE(it->type() == osmium::item_type::way);
    REQUIRE(it->ref() == 113);
    REQUIRE(std::string{it->role()} == "inner");
    ++it;

    REQUIRE(it == relation.members().end());
}

TEST_CASE("create relation using builders: create relation member from existing relation member") {
    osmium::memory::Buffer buffer{test_buffer_size};

    osmium::builder::add_relation(buffer,
        _id(123),
        _member(osmium::item_type::way, 111, "outer"),
        _member(osmium::item_type::way, 112, "inner")
    );

    const auto& relation1 = buffer.get<osmium::Relation>(0);

    const auto pos = osmium::builder::add_relation(buffer,
        _id(124),
        _member(*relation1.members().begin()),
        _members(std::next(relation1.members().begin()), relation1.members().end())
    );

    const auto& relation = buffer.get<osmium::Relation>(pos);

    REQUIRE(relation.id() == 124);
    REQUIRE(relation.members().size() == 2);

    auto it = relation.members().begin();

    REQUIRE(it->type() == osmium::item_type::way);
    REQUIRE(it->ref() == 111);
    REQUIRE(std::string{it->role()} == "outer");
    ++it;

    REQUIRE(it->type() == osmium::item_type::way);
    REQUIRE(it->ref() == 112);
    REQUIRE(std::string{it->role()} == "inner");
    ++it;

    REQUIRE(it == relation.members().end());
}

TEST_CASE("create relation using builders: create relation with members from initializer list") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const auto pos = osmium::builder::add_relation(buffer,
        _id(123),
        _members({
            {osmium::item_type::node, 123, ""},
            {osmium::item_type::way, 111, "outer"}
        })
    );

    const auto& relation = buffer.get<osmium::Relation>(pos);

    REQUIRE(relation.id() == 123);
    REQUIRE(relation.members().size() == 2);
    REQUIRE(std::distance(relation.cbegin(), relation.cend()) == 1);

    auto it = relation.members().begin();
    REQUIRE(it->type() == osmium::item_type::node);
    REQUIRE(it->ref() == 123);
    REQUIRE(it->role()[0] == '\0');
    ++it;
    REQUIRE(it->type() == osmium::item_type::way);
    REQUIRE(it->ref() == 111);
    REQUIRE(std::string{it->role()} == "outer");
    ++it;
    REQUIRE(it == relation.members().end());
}

TEST_CASE("create relation using builders: create relation with members from iterators and some tags") {
    osmium::memory::Buffer buffer{test_buffer_size};

    const std::vector<member_type> members = {
        {osmium::item_type::node, 123},
        {osmium::item_type::way, 111, "outer"}
    };

    SECTION("using iterators") {
        osmium::builder::add_relation(buffer,
            _id(123),
            _members(members.begin(), members.end()),
            _tag("a", "x"),
            _tag("b", "y")
        );
    }
    SECTION("using container") {
        osmium::builder::add_relation(buffer,
            _id(123),
            _members(members),
            _tag("a", "x"),
            _tag("b", "y")
        );
    }

    const auto& relation = buffer.get<osmium::Relation>(0);

    REQUIRE(relation.id() == 123);
    REQUIRE(relation.members().size() == 2);
    REQUIRE(relation.tags().size() == 2);
    REQUIRE(std::distance(relation.cbegin(), relation.cend()) == 2);

    auto it = relation.members().begin();
    REQUIRE(it->type() == osmium::item_type::node);
    REQUIRE(it->ref() == 123);
    REQUIRE(it->role()[0] == '\0');
    ++it;
    REQUIRE(it->type() == osmium::item_type::way);
    REQUIRE(it->ref() == 111);
    REQUIRE(std::string{it->role()} == "outer");
    ++it;
    REQUIRE(it == relation.members().end());
}

TEST_CASE("create area using builders") {
    osmium::memory::Buffer buffer{test_buffer_size};

    SECTION("add area without rings") {
        const auto pos = osmium::builder::add_area(buffer,
            _id(999),
            _cid(21),
            _uid(222),
            _user("foo"),
            _tag("landuse", "residential")
        );

        const auto& area = buffer.get<osmium::Area>(pos);

        REQUIRE(area.id() == 999);
        REQUIRE(area.version() == 0);
        REQUIRE(area.timestamp() == osmium::Timestamp{});
        REQUIRE(area.changeset() == 21);
        REQUIRE(area.uid() == 222);
        REQUIRE(std::string{area.user()} == "foo");
        REQUIRE(area.tags().size() == 1);
        REQUIRE(std::distance(area.cbegin(), area.cend()) == 1);
    }

}

