#include "catch.hpp"

#include <osmium/index/detail/tmpfile.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/dense_mem_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

using dense_file_array = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;
using sparse_file_array = osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, osmium::Location>;

template <class TMemoryIndex, class TFileIndex>
void test_index(std::function<void(TMemoryIndex&, const int)> dump_method) {
    const int fd = osmium::detail::create_tmp_file();
    REQUIRE(osmium::file_size(fd) == 0);
    const osmium::unsigned_object_id_type id1 = 12;
    const osmium::unsigned_object_id_type id2 = 3;
    const osmium::unsigned_object_id_type id3 = 7;
    const osmium::Location loc1{1.2, 4.5};
    const osmium::Location loc2{3.5, -7.2};
    const osmium::Location loc3{-12.7, 14.5};

    TMemoryIndex index;
    index.set(id1, loc1);
    index.set(id2, loc2);
    index.set(id3, loc3);

    // implementation of TMemoryIndex::sort should be empty if it is a dense index
    index.sort();
    dump_method(index, fd);

    REQUIRE(osmium::file_size(fd) >= (3 * sizeof(typename TFileIndex::element_type)));

    // load index from file
    TFileIndex file_index{fd};

    // test retrievals
    REQUIRE(loc1 == file_index.get(id1));
    REQUIRE(loc2 == file_index.get(id2));
    REQUIRE(loc3 == file_index.get(id3));
    REQUIRE_THROWS_AS(file_index.get(5), osmium::not_found);
    REQUIRE_THROWS_AS(file_index.get(6), osmium::not_found);
    REQUIRE_THROWS_AS(file_index.get(200), osmium::not_found);
}

#ifdef __linux__
using dense_mmap_array = osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;

TEST_CASE("Dump DenseMmapArray, load as DenseFileArray") {
    auto dump_method = [](dense_mmap_array& index, const int fd) { index.dump_as_array(fd);};
    test_index<dense_mmap_array, dense_file_array>(dump_method);
}
#else
# pragma message("not running 'DenseMmapArray' test case on this machine")
#endif

using dense_mem_array = osmium::index::map::DenseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

TEST_CASE("Dump DenseMemArray, load as DenseFileArray") {
    auto dump_method = [](dense_mem_array& index, const int fd) { index.dump_as_array(fd);};
    test_index<dense_mem_array, dense_file_array>(dump_method);
}

#ifdef __linux__
using sparse_mmap_array = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;

TEST_CASE("Dump SparseMmapArray, load as SparseFileArray") {
    auto dump_method = [](sparse_mmap_array& index, const int fd) { index.dump_as_list(fd);};
    test_index<sparse_mmap_array, sparse_file_array>(dump_method);
}
#else
# pragma message("not running 'SparseMmapArray' test case on this machine")
#endif

using sparse_mem_array = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

TEST_CASE("Dump SparseMemArray, load as SparseFileArray") {
    auto dump_method = [](sparse_mem_array& index, const int fd) { index.dump_as_list(fd);};
    test_index<sparse_mem_array, sparse_file_array>(dump_method);
}
