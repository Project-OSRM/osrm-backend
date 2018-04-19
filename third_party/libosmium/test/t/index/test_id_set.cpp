#include "catch.hpp"

#include <osmium/index/id_set.hpp>
#include <osmium/osm/types.hpp>

TEST_CASE("Basic functionality of IdSetDense") {
    osmium::index::IdSetDense<osmium::unsigned_object_id_type> s;

    REQUIRE_FALSE(s.get(17));
    REQUIRE_FALSE(s.get(28));
    REQUIRE(s.empty());
    REQUIRE(s.size() == 0); // NOLINT(readability-container-size-empty)

    s.set(17);
    REQUIRE(s.get(17));
    REQUIRE_FALSE(s.get(28));
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.size() == 1);

    s.set(28);
    REQUIRE(s.get(17));
    REQUIRE(s.get(28));
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.size() == 2);

    s.set(17);
    REQUIRE(s.get(17));
    REQUIRE(s.size() == 2);

    REQUIRE_FALSE(s.check_and_set(17));
    REQUIRE(s.get(17));
    REQUIRE(s.size() == 2);

    s.unset(17);
    REQUIRE_FALSE(s.get(17));
    REQUIRE(s.size() == 1);

    REQUIRE(s.check_and_set(32));
    REQUIRE(s.get(32));
    REQUIRE(s.size() == 2);

    s.clear();
    REQUIRE(s.empty());
}

TEST_CASE("Iterating over IdSetDense") {
    osmium::index::IdSetDense<osmium::unsigned_object_id_type> s;
    s.set(7);
    s.set(35);
    s.set(35);
    s.set(20);
    s.set(1ull << 33u);
    s.set(21);
    s.set((1ull << 27u) + 13u);

    REQUIRE(s.size() == 6);

    auto it = s.begin();
    REQUIRE(it != s.end());
    REQUIRE(*it == 7);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 20);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 21);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 35);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == (1ull << 27u) + 13u);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 1ull << 33u);
    ++it;
    REQUIRE(it == s.end());
}

TEST_CASE("Test with larger Ids") {
    osmium::index::IdSetDense<osmium::unsigned_object_id_type> s;

    const osmium::unsigned_object_id_type start = 25;
    const osmium::unsigned_object_id_type end = 100000000;
    const osmium::unsigned_object_id_type step = 123456;

    for (osmium::unsigned_object_id_type i = start; i < end; i += step) {
        s.set(i);
    }

    for (osmium::unsigned_object_id_type i = start; i < end; i += step) {
        REQUIRE(s.get(i));
        REQUIRE_FALSE(s.get(i + 1));
    }
}

TEST_CASE("Large gap") {
    osmium::index::IdSetDense<osmium::unsigned_object_id_type> s;

    s.set(3);
    s.set(1u << 30u);

    REQUIRE(s.get(1u << 30u));
    REQUIRE_FALSE(s.get(1u << 29u));
}

TEST_CASE("Basic functionality of IdSetSmall") {
    osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s;

    REQUIRE_FALSE(s.get(17));
    REQUIRE_FALSE(s.get(28));
    REQUIRE(s.empty());

    s.set(17);
    REQUIRE(s.get(17));
    REQUIRE_FALSE(s.get(28));
    REQUIRE_FALSE(s.empty());

    s.set(28);
    REQUIRE(s.get(17));
    REQUIRE(s.get(28));
    REQUIRE_FALSE(s.empty());

    s.clear();
    REQUIRE(s.empty());
}

TEST_CASE("Iterating over IdSetSmall") {
    osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s;
    s.set(7);
    s.set(35);
    s.set(35);
    s.set(20);
    s.set(1ull << 33u);
    s.set(21);
    s.set((1ull << 27u) + 13u);

    // needs to be called before size() and iterator will work properly
    s.sort_unique();

    REQUIRE(s.size() == 6);

    auto it = s.begin();
    REQUIRE(it != s.end());
    REQUIRE(*it == 7);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 20);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 21);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 35);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == (1ull << 27u) + 13u);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 1ull << 33u);
    ++it;
    REQUIRE(it == s.end());
}

