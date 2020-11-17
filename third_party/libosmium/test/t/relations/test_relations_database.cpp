#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/relations/relations_database.hpp>
#include <osmium/storage/item_stash.hpp>

#include <vector>

osmium::memory::Buffer fill_buffer() {
    using namespace osmium::builder::attr; // NOLINT(google-build-using-namespace)
    osmium::memory::Buffer buffer{1024 * 1024, osmium::memory::Buffer::auto_grow::yes};

    osmium::builder::add_relation(buffer,
        _id(1),
        _member(osmium::item_type::way, 1, "outer")
    );

    osmium::builder::add_relation(buffer,
        _id(2),
        _member(osmium::item_type::way, 1, "outer"),
        _member(osmium::item_type::way, 2, "outer")
    );

    osmium::builder::add_relation(buffer,
        _id(3),
        _member(osmium::item_type::way, 1, "outer"),
        _member(osmium::item_type::way, 2, "inner"),
        _member(osmium::item_type::way, 3, "inner")
    );

    return buffer;
}

TEST_CASE("Fill relation database") {
    const auto buffer = fill_buffer();

    osmium::ItemStash stash;
    osmium::relations::RelationsDatabase rdb{stash};

    REQUIRE(rdb.size() == 0);
    REQUIRE(rdb.used_memory() < 100);

    for (const auto& relation : buffer.select<osmium::Relation>()) {
        auto handle = rdb.add(relation);
        handle.set_members(relation.cmembers().size());
        handle.decrement_members();
        REQUIRE(handle.has_all_members() == (relation.id() == 1));
    }

    REQUIRE(rdb.size() == 3);

    int n = 0;
    rdb.for_each_relation([&](const osmium::relations::RelationHandle& rel_handle) {
        ++n;
        REQUIRE(rel_handle->members().size() == (*rel_handle).id());
    });
    REQUIRE(n == 3);
}

TEST_CASE("Check need members and handle ops") {
    const auto buffer = fill_buffer();

    osmium::ItemStash stash;
    osmium::relations::RelationsDatabase rdb{stash};

    for (const auto& relation : buffer.select<osmium::Relation>()) {
        auto handle = rdb.add(relation);
        REQUIRE(*handle == relation);
        REQUIRE(handle->id() == relation.id());
        REQUIRE(handle.pos() + 1 == relation.positive_id());
        REQUIRE(rdb[handle.pos()].pos() == handle.pos());

        for (auto i = relation.id(); i > 0; --i) {
            handle.increment_members();
        }

        handle.decrement_members();
        REQUIRE(handle.has_all_members() == (relation.id() == 1));
        if (handle.has_all_members()) {
            handle.remove();
        }
    }

    REQUIRE(rdb.size() == 3);

    std::vector<const osmium::Relation*> rels;
    rdb.for_each_relation([&](const osmium::relations::RelationHandle& rel_handle) {
        rels.push_back(&*rel_handle);
    });

    REQUIRE(rels.size() == 2);

    osmium::object_id_type n = 2;
    for (const auto* rel : rels) {
        REQUIRE(rel->id() == n);
        ++n;
    }

    REQUIRE(rdb[1]->id() == 2);
    REQUIRE(rdb[2]->id() == 3);

    rdb[1].remove();

    REQUIRE(rdb.count_relations() == 1);
}

