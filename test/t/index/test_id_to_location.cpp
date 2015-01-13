#include "catch.hpp"

#include <osmium/osm/types.hpp>
#include <osmium/osm/location.hpp>

#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_table.hpp>
#include <osmium/index/map/stl_map.hpp>
#include <osmium/index/map/stl_vector.hpp>
#include <osmium/index/map/mmap_vector_anon.hpp>
#include <osmium/index/map/mmap_vector_file.hpp>

template <typename TIndex>
void test_func_all(TIndex& index) {
    osmium::unsigned_object_id_type id1 = 12;
    osmium::unsigned_object_id_type id2 = 3;
    osmium::Location loc1(1.2, 4.5);
    osmium::Location loc2(3.5, -7.2);

    REQUIRE_THROWS_AS(index.get(id1), osmium::not_found);

    index.set(id1, loc1);
    index.set(id2, loc2);

    index.sort();

    REQUIRE_THROWS_AS(index.get(5), osmium::not_found);
    REQUIRE_THROWS_AS(index.get(100), osmium::not_found);
}

template <typename TIndex>
void test_func_real(TIndex& index) {
    osmium::unsigned_object_id_type id1 = 12;
    osmium::unsigned_object_id_type id2 = 3;
    osmium::Location loc1(1.2, 4.5);
    osmium::Location loc2(3.5, -7.2);

    index.set(id1, loc1);
    index.set(id2, loc2);

    index.sort();

    REQUIRE(loc1 == index.get(id1));
    REQUIRE(loc2 == index.get(id2));

    REQUIRE_THROWS_AS(index.get(5), osmium::not_found);
    REQUIRE_THROWS_AS(index.get(100), osmium::not_found);

    index.clear();

    REQUIRE_THROWS_AS(index.get(id1), osmium::not_found);
}

TEST_CASE("IdToLocation") {

SECTION("Dummy") {
    typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_type;

    index_type index1;

    REQUIRE(0 == index1.size());
    REQUIRE(0 == index1.used_memory());

    test_func_all<index_type>(index1);

    REQUIRE(0 == index1.size());
    REQUIRE(0 == index1.used_memory());
}

SECTION("DenseMapMem") {
    typedef osmium::index::map::DenseMapMem<osmium::unsigned_object_id_type, osmium::Location> index_type;

    index_type index1;
    index1.reserve(1000);
    test_func_all<index_type>(index1);

    index_type index2;
    index2.reserve(1000);
    test_func_real<index_type>(index2);
}

#ifdef __linux__
SECTION("DenseMapMmap") {
    typedef osmium::index::map::DenseMapMmap<osmium::unsigned_object_id_type, osmium::Location> index_type;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}
#else
# pragma message "not running 'DenseMapMmap' test case on this machine"
#endif

SECTION("DenseMapFile") {
    typedef osmium::index::map::DenseMapFile<osmium::unsigned_object_id_type, osmium::Location> index_type;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}

SECTION("SparseTable") {
    typedef osmium::index::map::SparseTable<osmium::unsigned_object_id_type, osmium::Location> index_type;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}

SECTION("StlMap") {
    typedef osmium::index::map::StlMap<osmium::unsigned_object_id_type, osmium::Location> index_type;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}

SECTION("SparseMapMem") {
    typedef osmium::index::map::SparseMapMem<osmium::unsigned_object_id_type, osmium::Location> index_type;

    index_type index1;

    REQUIRE(0 == index1.size());
    REQUIRE(0 == index1.used_memory());

    test_func_all<index_type>(index1);

    REQUIRE(2 == index1.size());

    index_type index2;
    test_func_real<index_type>(index2);
}

}

