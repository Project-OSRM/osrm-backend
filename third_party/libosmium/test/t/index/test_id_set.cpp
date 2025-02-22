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

TEST_CASE("Copying IdSetDense") {
    osmium::index::IdSetDense<osmium::unsigned_object_id_type> s1;
    osmium::index::IdSetDense<osmium::unsigned_object_id_type> s2;

    REQUIRE(s1.empty());
    REQUIRE(s2.empty());

    s1.set(17);
    s1.set(28);
    REQUIRE(s1.get(17));
    REQUIRE(s1.get(17));
    REQUIRE(s1.size() == 2);

    s2 = s1;
    REQUIRE(s1.get(17));
    REQUIRE(s1.get(28));
    REQUIRE(s1.size() == 2);
    REQUIRE(s2.get(17));
    REQUIRE(s2.get(28));
    REQUIRE(s2.size() == 2);

    const osmium::index::IdSetDense<osmium::unsigned_object_id_type> s3{s1};
    REQUIRE(s3.get(17));
    REQUIRE(s3.get(28));
    REQUIRE(s3.size() == 2);
}

TEST_CASE("Iterating over IdSetDense") {
    osmium::index::IdSetDense<osmium::unsigned_object_id_type> s;
    s.set(7);
    s.set(35);
    s.set(35);
    s.set(20);
    s.set(1ULL << 33U);
    s.set(21);
    s.set((1ULL << 27U) + 13U);

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
    REQUIRE(*it == (1ULL << 27U) + 13U);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 1ULL << 33U);
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
    s.set(1U << 30U);

    REQUIRE(s.get(1U << 30U));
    REQUIRE_FALSE(s.get(1U << 29U));
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
    const auto size = s.size();

    // Setting the same id as last time doesn't grow the size
    s.set(28);
    REQUIRE(s.get(17));
    REQUIRE(s.get(28));
    REQUIRE_FALSE(s.empty());
    REQUIRE(size == s.size());

    s.clear();
    REQUIRE(s.empty());
}

TEST_CASE("Copying IdSetSmall") {
    osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s1;
    osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s2;

    REQUIRE(s1.empty());
    REQUIRE(s2.empty());

    s1.set(17);
    s1.set(28);
    REQUIRE(s1.get(17));
    REQUIRE(s1.get(17));
    REQUIRE(s1.size() == 2);

    s2 = s1;
    REQUIRE(s1.get(17));
    REQUIRE(s1.get(28));
    REQUIRE(s1.size() == 2);
    REQUIRE(s2.get(17));
    REQUIRE(s2.get(28));
    REQUIRE(s2.size() == 2);

    const osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s3{s1};
    REQUIRE(s3.get(17));
    REQUIRE(s3.get(28));
    REQUIRE(s3.size() == 2);
}

TEST_CASE("Iterating over IdSetSmall") {
    osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s;
    s.set(7);
    s.set(35);
    s.set(35);
    s.set(20);
    s.set(1ULL << 33U);
    s.set(21);
    s.set((1ULL << 27U) + 13U);

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
    REQUIRE(*it == (1ULL << 27U) + 13U);
    ++it;
    REQUIRE(it != s.end());
    REQUIRE(*it == 1ULL << 33U);
    ++it;
    REQUIRE(it == s.end());
}

TEST_CASE("Merge two IdSetSmall") {
    osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s1;
    osmium::index::IdSetSmall<osmium::unsigned_object_id_type> s2;

    s1.set(23);
    s1.set(2);
    s1.set(7);
    s1.set(55);
    s1.set(42);
    s1.set(7);

    s2.set(2);
    s2.set(32);
    s2.set(8);
    s2.set(55);
    s2.set(1);

    s1.sort_unique();
    REQUIRE(s1.size() == 5);
    s2.sort_unique();
    REQUIRE(s2.size() == 5);
    s1.merge_sorted(s2);
    REQUIRE(s1.size() == 8);

    const auto ids = {1, 2, 7, 8, 23, 32, 42, 55};
    REQUIRE(std::equal(s1.cbegin(), s1.cend(), ids.begin()));
}

