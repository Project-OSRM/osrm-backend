
#include "catch.hpp"

#include <type_traits>

#include <osmium/index/relations_map.hpp>

static_assert(std::is_default_constructible<osmium::index::RelationsMapIndex>::value == false, "RelationsMapIndex should not be default constructible");
static_assert(std::is_copy_constructible<osmium::index::RelationsMapIndex>::value == false, "RelationsMapIndex should not be copy constructible");
static_assert(std::is_copy_constructible<osmium::index::RelationsMapStash>::value == false, "RelationsMapStash should not be copy constructible");
static_assert(std::is_copy_assignable<osmium::index::RelationsMapIndex>::value == false, "RelationsMapIndex should not be copy assignable");
static_assert(std::is_copy_assignable<osmium::index::RelationsMapStash>::value == false, "RelationsMapStash should not be copy assignable");

TEST_CASE("RelationsMapStash lvalue") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());
    REQUIRE(stash.size() == 0);

    stash.add(1, 2);
    stash.add(2, 3);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 2);

    auto index= stash.build_index();

    REQUIRE_FALSE(index.empty());
    REQUIRE(index.size() == 2);

    index.for_each_parent(1, [](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 2);
    });
}

osmium::index::RelationsMapIndex func() {
    osmium::index::RelationsMapStash stash;

    stash.add(1, 2);
    stash.add(2, 3);

    return stash.build_index();
}

TEST_CASE("RelationsMapStash rvalue") {
    const osmium::index::RelationsMapIndex index{func()};

    index.for_each_parent(1, [](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 2);
    });
}

