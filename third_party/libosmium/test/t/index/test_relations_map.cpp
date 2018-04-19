#include "catch.hpp"

#include <osmium/index/relations_map.hpp>

#include <type_traits>

static_assert(!std::is_default_constructible<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should not be default constructible");
static_assert(!std::is_copy_constructible<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should not be copy constructible");
static_assert(!std::is_copy_constructible<osmium::index::RelationsMapStash>::value, "RelationsMapStash should not be copy constructible");
static_assert(!std::is_copy_assignable<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should not be copy assignable");
static_assert(!std::is_copy_assignable<osmium::index::RelationsMapStash>::value, "RelationsMapStash should not be copy assignable");

TEST_CASE("RelationsMapStash lvalue") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());
    REQUIRE(stash.size() == 0); // NOLINT(readability-container-size-empty)

    stash.add(1, 2);
    stash.add(2, 3);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 2);

    const auto index = stash.build_member_to_parent_index();

    REQUIRE_FALSE(index.empty());
    REQUIRE(index.size() == 2);

    int count = 0;
    index.for_each(1, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 2);
        ++count;
    });
    REQUIRE(count == 1);
}

osmium::index::RelationsMapIndex func() {
    osmium::index::RelationsMapStash stash;

    stash.add(1, 2);
    stash.add(2, 3);

    return stash.build_member_to_parent_index();
}

TEST_CASE("RelationsMapStash rvalue") {
    const osmium::index::RelationsMapIndex index{func()};

    int count = 0;
    index.for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 3);
        ++count;
    });
    REQUIRE(count == 1);
}

TEST_CASE("RelationsMapStash reverse index") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());

    stash.add(1, 2);
    stash.add(2, 3);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 2);

    const auto index = stash.build_parent_to_member_index();

    REQUIRE_FALSE(index.empty());
    REQUIRE(index.size() == 2);

    int count = 0;
    index.for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 1);
        ++count;
    });
    index.for_each(3, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 2);
        ++count;
    });
    REQUIRE(count == 2);
}

TEST_CASE("RelationsMapStash both indexes") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());

    stash.add(1, 2);
    stash.add(2, 3);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 2);

    const auto index = stash.build_indexes();

    REQUIRE_FALSE(index.empty());
    REQUIRE(index.size() == 2);

    int count = 0;
    index.member_to_parent().for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 3);
        ++count;
    });
    index.parent_to_member().for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 1);
        ++count;
    });
    REQUIRE(count == 2);
}

