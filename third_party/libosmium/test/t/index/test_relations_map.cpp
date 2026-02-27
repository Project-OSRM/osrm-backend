#include "catch.hpp"

#include <osmium/index/relations_map.hpp>

#include <type_traits>

static_assert(!std::is_default_constructible<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should not be default constructible");
static_assert(!std::is_copy_constructible<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should not be copy constructible");
static_assert(!std::is_copy_constructible<osmium::index::RelationsMapStash>::value, "RelationsMapStash should not be copy constructible");
static_assert(!std::is_copy_assignable<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should not be copy assignable");
static_assert(!std::is_copy_assignable<osmium::index::RelationsMapStash>::value, "RelationsMapStash should not be copy assignable");
static_assert(std::is_move_constructible<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should be move constructible");
static_assert(std::is_move_constructible<osmium::index::RelationsMapStash>::value, "RelationsMapStash should be move constructible");
static_assert(std::is_move_assignable<osmium::index::RelationsMapIndex>::value, "RelationsMapIndex should be move assignable");
static_assert(std::is_move_assignable<osmium::index::RelationsMapStash>::value, "RelationsMapStash should be move assignable");

TEST_CASE("RelationsMapStash lvalue") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());
    REQUIRE(stash.size() == 0); // NOLINT(readability-container-size-empty)

    stash.add(1, 2);
    stash.add(2, 3);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 2);

    const auto sizes = stash.sizes();
    REQUIRE(sizes.first == 2);
    REQUIRE(sizes.second == 0);

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

TEST_CASE("RelationsMapStash 64bit") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());
    REQUIRE(stash.size() == 0); // NOLINT(readability-container-size-empty)

    const uint64_t maxsmall = (1ULL << 32ULL) - 1;
    const uint64_t large = 1ULL << 33ULL;

    stash.add(1, 2);
    stash.add(2, maxsmall);
    stash.add(3, large + 1);
    stash.add(4, large + 2);
    stash.add(5, large + 3);
    stash.add(large + 5, 5);
    stash.add(large + 6, 6);
    stash.add(large + 7, large + 8);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 8);

    const auto sizes = stash.sizes();
    REQUIRE(sizes.first == 2);
    REQUIRE(sizes.second == 6);

    const auto index = stash.build_member_to_parent_index();

    REQUIRE_FALSE(index.empty());
    REQUIRE(index.size() == 8);

    int count = 0;

    index.for_each(1, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 2);
        ++count;
    });
    index.for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == maxsmall);
        ++count;
    });
    index.for_each(3, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 1);
        ++count;
    });
    index.for_each(4, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 2);
        ++count;
    });
    index.for_each(5, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 3);
        ++count;
    });
    index.for_each(large + 5, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 5);
        ++count;
    });
    index.for_each(large + 6, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 6);
        ++count;
    });
    index.for_each(large + 7, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 8);
        ++count;
    });


    REQUIRE(count == 8);
}

TEST_CASE("RelationsMapStash duplicates will be removed in index") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());

    const uint64_t large = 1ULL << 33ULL;

    stash.add(1, 2);
    stash.add(2, 3);
    stash.add(1, 2); // duplicate
    stash.add(2, 2);
    stash.add(1, 2); // another duplicate
    stash.add(5, large + 1);
    stash.add(5, 4);
    stash.add(5, large + 1); // also duplicate
    stash.add(large + 2, 1);
    stash.add(large + 2, 4);
    stash.add(large + 2, 1); // also duplicate

    REQUIRE(stash.size() == 11);

    const auto sizes = stash.sizes();
    REQUIRE(sizes.first == 6);
    REQUIRE(sizes.second == 5);

    SECTION("member to parent") {
        const auto index = stash.build_member_to_parent_index();
        REQUIRE_FALSE(index.empty());
        REQUIRE(index.size() == 7);
    }

    SECTION("parent to member") {
        const auto index = stash.build_parent_to_member_index();
        REQUIRE_FALSE(index.empty());
        REQUIRE(index.size() == 7);
    }
}

TEST_CASE("RelationsMapStash n:m results") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());

    const uint64_t large = 1ULL << 33ULL;

    stash.add(3, large + 1);
    stash.add(3, large + 2);
    stash.add(4, large + 1);
    stash.add(4, large + 2);
    stash.add(4, large + 1);
    stash.add(5, large + 2);

    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 6);

    const auto sizes = stash.sizes();
    REQUIRE(sizes.first == 0);
    REQUIRE(sizes.second == 6);

    const auto index = stash.build_member_to_parent_index();

    REQUIRE_FALSE(index.empty());
    REQUIRE(index.size() == 5);

    int count = 0;

    index.for_each(2, [&](osmium::unsigned_object_id_type /*id*/) {
        REQUIRE(false);
    });
    index.for_each(3, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(((id == large + 1) || (id == large + 2)));
        ++count;
    });
    index.for_each(4, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(((id == large + 1) || (id == large + 2)));
        ++count;
    });
    index.for_each(5, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 2);
        ++count;
    });

    REQUIRE(count == 5);
}

namespace {

osmium::index::RelationsMapIndex func() {
    osmium::index::RelationsMapStash stash;

    stash.add(1, 2);
    stash.add(2, 3);

    return stash.build_member_to_parent_index();
}

} // anonymous namespace

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

TEST_CASE("RelationsMapStash reverse index with 64bit values") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());

    const uint64_t large = 1ULL << 33ULL;

    stash.add(1, 2);
    stash.add(2, 3);
    stash.add(3, large + 1);
    stash.add(4, large + 2);
    stash.add(5, large + 3);
    stash.add(large + 5, 5);
    stash.add(large + 6, 6);
    stash.add(large + 7, large + 8);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 8);

    const auto sizes = stash.sizes();
    REQUIRE(sizes.first == 2);
    REQUIRE(sizes.second == 6);

    const auto index = stash.build_parent_to_member_index();

    REQUIRE_FALSE(index.empty());
    REQUIRE(index.size() == 8);

    int count = 0;

    index.for_each(1, [&](osmium::unsigned_object_id_type /*id*/) {
        REQUIRE(false);
    });
    index.for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 1);
        ++count;
    });
    index.for_each(3, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 2);
        ++count;
    });
    index.for_each(large + 1, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 3);
        ++count;
    });
    index.for_each(large + 2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 4);
        ++count;
    });
    index.for_each(large + 3, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 5);
        ++count;
    });
    index.for_each(5, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 5);
        ++count;
    });
    index.for_each(6, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 6);
        ++count;
    });
    index.for_each(large + 8, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == large + 7);
        ++count;
    });


    REQUIRE(count == 8);
}

TEST_CASE("RelationsMapStash both indexes") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());

    stash.add(1, 2);
    stash.add(2, 3);
    REQUIRE_FALSE(stash.empty());
    REQUIRE(stash.size() == 2);

    const auto indexes = stash.build_indexes();

    REQUIRE_FALSE(indexes.empty());
    REQUIRE(indexes.size() == 2);

    int count = 0;
    indexes.member_to_parent().for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 3);
        ++count;
    });
    indexes.parent_to_member().for_each(2, [&](osmium::unsigned_object_id_type id) {
        REQUIRE(id == 1);
        ++count;
    });
    REQUIRE(count == 2);
}

TEST_CASE("RelationsMapStash small and large") {
    osmium::index::RelationsMapStash stash;
    REQUIRE(stash.empty());

    stash.add(1, 2);
    stash.add(2, 3);
    stash.add(4, (1ULL << 32ULL) - 1);
    stash.add((1ULL << 32ULL) - 1, 5);

    REQUIRE(stash.sizes().first == 4);
    REQUIRE(stash.sizes().second == 0);

    stash.add(6, (1ULL << 32ULL) + 1);
    stash.add((1ULL << 32ULL) + 1, 7);

    REQUIRE(stash.sizes().first == 4);
    REQUIRE(stash.sizes().second == 2);

    stash.add(8, (1ULL << 63ULL));
    stash.add((1ULL << 63ULL), 9);

    REQUIRE(stash.sizes().first == 4);
    REQUIRE(stash.sizes().second == 4);
}
