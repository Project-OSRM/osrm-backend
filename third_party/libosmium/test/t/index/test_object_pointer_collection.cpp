#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/object_pointer_collection.hpp>
#include <osmium/osm/object_comparisons.hpp>
#include <osmium/visitor.hpp>

using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)

TEST_CASE("Create ObjectPointerCollection") {
    osmium::memory::Buffer buffer{1024, osmium::memory::Buffer::auto_grow::yes};

    osmium::builder::add_node(buffer,
        _id(3),
        _version(3)
    );

    osmium::builder::add_node(buffer,
        _id(1),
        _version(2)
    );

    osmium::builder::add_node(buffer,
        _id(1),
        _version(4)
    );

    osmium::ObjectPointerCollection collection;
    REQUIRE(collection.empty());
    REQUIRE(collection.size() == 0); // NOLINT(readability-container-size-empty)

    osmium::apply(buffer, collection);

    REQUIRE_FALSE(collection.empty());
    REQUIRE(collection.size() == 3);

    auto it = collection.cbegin();
    REQUIRE(it->id() == 3);
    REQUIRE(it->version() == 3);
    ++it;
    REQUIRE(it->id() == 1);
    REQUIRE(it->version() == 2);
    ++it;
    REQUIRE(it->id() == 1);
    REQUIRE(it->version() == 4);
    ++it;
    REQUIRE(it == collection.cend());

    collection.sort(osmium::object_order_type_id_version{});

    REQUIRE(collection.size() == 3);

    it = collection.cbegin();
    REQUIRE(it->id() == 1);
    REQUIRE(it->version() == 2);
    ++it;
    REQUIRE(it->id() == 1);
    REQUIRE(it->version() == 4);
    ++it;
    REQUIRE(it->id() == 3);
    REQUIRE(it->version() == 3);
    ++it;
    REQUIRE(it == collection.cend());

    collection.sort(osmium::object_order_type_id_reverse_version{});

    it = collection.cbegin();
    REQUIRE(it->id() == 1);
    REQUIRE(it->version() == 4);
    ++it;
    REQUIRE(it->id() == 1);
    REQUIRE(it->version() == 2);
    ++it;
    REQUIRE(it->id() == 3);
    REQUIRE(it->version() == 3);
    ++it;
    REQUIRE(it == collection.cend());

    collection.clear();

    REQUIRE(collection.empty());
}

