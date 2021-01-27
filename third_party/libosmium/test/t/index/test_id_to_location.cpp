#include "catch.hpp"

#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/dense_mem_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mem_map.hpp>
#include <osmium/index/map/sparse_mem_table.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/node_locations_map.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

#include <memory>
#include <string>
#include <vector>

static_assert(osmium::index::empty_value<osmium::Location>() == osmium::Location{}, "Empty value for location is wrong");

template <typename TIndex>
void test_func_all(TIndex& index) {
    const osmium::unsigned_object_id_type id1 = 12;
    const osmium::unsigned_object_id_type id2 = 3;
    const osmium::Location loc1{1.2, 4.5};
    const osmium::Location loc2{3.5, -7.2};

    REQUIRE_THROWS_AS(index.get(id1), const osmium::not_found&);

    index.set(id1, loc1);
    index.set(id2, loc2);

    index.sort();

    REQUIRE_THROWS_AS(index.get(0), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(1), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(5), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(100), const osmium::not_found&);
    REQUIRE_THROWS_WITH(index.get(0), "id 0 not found");
    REQUIRE_THROWS_WITH(index.get(1), "id 1 not found");

    REQUIRE(index.get_noexcept(0) == osmium::Location{});
    REQUIRE(index.get_noexcept(1) == osmium::Location{});
    REQUIRE(index.get_noexcept(5) == osmium::Location{});
    REQUIRE(index.get_noexcept(100) == osmium::Location{});
}

template <typename TIndex>
void test_func_real(TIndex& index) {
    const osmium::unsigned_object_id_type id1 = 12;
    const osmium::unsigned_object_id_type id2 = 3;
    const osmium::Location loc1{1.2, 4.5};
    const osmium::Location loc2{3.5, -7.2};

    index.set(id1, loc1);
    index.set(id2, loc2);

    index.sort();

    REQUIRE(loc1 == index.get(id1));
    REQUIRE(loc2 == index.get(id2));

    REQUIRE(loc1 == index.get_noexcept(id1));
    REQUIRE(loc2 == index.get_noexcept(id2));

    REQUIRE_THROWS_AS(index.get(0), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(1), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(5), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(100), const osmium::not_found&);

    REQUIRE(index.get_noexcept(0) == osmium::Location{});
    REQUIRE(index.get_noexcept(1) == osmium::Location{});
    REQUIRE(index.get_noexcept(5) == osmium::Location{});
    REQUIRE(index.get_noexcept(100) == osmium::Location{});

    index.clear();

    REQUIRE_THROWS_AS(index.get(id1), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(id2), const osmium::not_found&);

    REQUIRE_THROWS_AS(index.get(0), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(1), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(5), const osmium::not_found&);
    REQUIRE_THROWS_AS(index.get(100), const osmium::not_found&);

    REQUIRE(index.get_noexcept(id1) == osmium::Location{});
    REQUIRE(index.get_noexcept(id2) == osmium::Location{});
    REQUIRE(index.get_noexcept(0) == osmium::Location{});
    REQUIRE(index.get_noexcept(1) == osmium::Location{});
    REQUIRE(index.get_noexcept(5) == osmium::Location{});
    REQUIRE(index.get_noexcept(100) == osmium::Location{});
}

TEST_CASE("Map Id to location: Dummy") {
    using index_type = osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;

    REQUIRE(0 == index1.size());
    REQUIRE(0 == index1.used_memory());

    test_func_all<index_type>(index1);

    REQUIRE(0 == index1.size());
    REQUIRE(0 == index1.used_memory());
}

TEST_CASE("Map Id to location: DenseMemArray") {
    using index_type = osmium::index::map::DenseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;
    index1.reserve(1000);
    test_func_all<index_type>(index1);

    index_type index2;
    index2.reserve(1000);
    test_func_real<index_type>(index2);
}

#ifdef __linux__
TEST_CASE("Map Id to location: DenseMmapArray") {
    using index_type = osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}
#else
# pragma message("not running 'DenseMmapArray' test case on this machine")
#endif

TEST_CASE("Map Id to location: DenseFileArray") {
    using index_type = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}

#ifdef OSMIUM_WITH_SPARSEHASH

TEST_CASE("Map Id to location: SparseMemTable") {
    using index_type = osmium::index::map::SparseMemTable<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}

#endif

TEST_CASE("Map Id to location: SparseMemMap") {
    using index_type = osmium::index::map::SparseMemMap<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}

TEST_CASE("Map Id to location: SparseMemArray") {
    using index_type = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;

    REQUIRE(0 == index1.size());
    REQUIRE(0 == index1.used_memory());

    test_func_all<index_type>(index1);

    REQUIRE(2 == index1.size());

    index_type index2;
    test_func_real<index_type>(index2);
}

#ifdef __linux__
TEST_CASE("Map Id to location: SparseMmapArray") {
    using index_type = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}
#else
# pragma message("not running 'SparseMmapArray' test case on this machine")
#endif

TEST_CASE("Map Id to location: FlexMem sparse") {
    using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1;
    test_func_all<index_type>(index1);

    index_type index2;
    test_func_real<index_type>(index2);
}

TEST_CASE("Map Id to location: FlexMem dense") {
    using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

    index_type index1{true};
    test_func_all<index_type>(index1);

    index_type index2{true};
    test_func_real<index_type>(index2);
}

TEST_CASE("Map Id to location: FlexMem switch") {
    using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;

    const osmium::Location loc1{1.1, 1.2};
    const osmium::Location loc2{2.2, -9.4};

    index_type index;

    REQUIRE(index.size() == 0);

    index.set(17, loc1);
    index.set(99, loc2);

    REQUIRE_FALSE(index.is_dense());
    REQUIRE(index.size() == 2);
    REQUIRE(index.get_noexcept(0) == osmium::Location{});
    REQUIRE(index.get_noexcept(1) == osmium::Location{});
    REQUIRE(index.get_noexcept(17) == loc1);
    REQUIRE(index.get_noexcept(99) == loc2);
    REQUIRE(index.get_noexcept(2000000000) == osmium::Location{});

    index.switch_to_dense();

    REQUIRE(index.is_dense());
    REQUIRE(index.size() >= 2);
    REQUIRE(index.get_noexcept(0) == osmium::Location{});
    REQUIRE(index.get_noexcept(1) == osmium::Location{});
    REQUIRE(index.get_noexcept(17) == loc1);
    REQUIRE(index.get_noexcept(99) == loc2);
    REQUIRE(index.get_noexcept(2000000000) == osmium::Location{});
}

TEST_CASE("Map Id to location: Dynamic map choice") {
    using map_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    const std::vector<std::string> map_type_names = map_factory.map_types();
    REQUIRE(map_type_names.size() >= 6);

    REQUIRE_THROWS_AS(map_factory.create_map(""), const osmium::map_factory_error&);
    REQUIRE_THROWS_AS(map_factory.create_map("does not exist"), const osmium::map_factory_error&);
    REQUIRE_THROWS_WITH(map_factory.create_map(""), "Need non-empty map type name");
    REQUIRE_THROWS_WITH(map_factory.create_map("does not exist"), "Support for map type 'does not exist' not compiled into this binary");

    for (const auto& map_type_name : map_type_names) {
        std::unique_ptr<map_type> index1 = map_factory.create_map(map_type_name);
        index1->reserve(1000);
        test_func_all<map_type>(*index1);

        std::unique_ptr<map_type> index2 = map_factory.create_map(map_type_name);
        index2->reserve(1000);
        test_func_real<map_type>(*index2);
    }
}

