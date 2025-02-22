#include "catch.hpp"

#include <osmium/index/detail/tmpfile.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/node_locations_map.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/file.hpp>

#include <vector>

using dense_file_array = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;

template <class TSparseIndex>
void test_index() {
    const int fd = osmium::detail::create_tmp_file();
    constexpr const size_t value_size = sizeof(typename TSparseIndex::element_type::second_type);
    constexpr const size_t buffer_size = (10L * 1024L * 1024L) / value_size;

    REQUIRE(osmium::file_size(fd) == 0);

    std::vector<osmium::NodeRef> refs = {
        osmium::NodeRef{1,                   osmium::Location{1.2, 4.5}},
        osmium::NodeRef{6,                   osmium::Location{3.5, -7.2}},
        osmium::NodeRef{2 * buffer_size,     osmium::Location{10.2, 64.5}},
        osmium::NodeRef{2 * buffer_size + 1, osmium::Location{39.5, -71.2}},
        osmium::NodeRef{3 * buffer_size - 1, osmium::Location{-1.2, 54.6}},
        osmium::NodeRef{3 * buffer_size,     osmium::Location{-171.2, 9.3}},
        osmium::NodeRef{3 * buffer_size + 1, osmium::Location{-171.21, 9.26}},
        osmium::NodeRef{3 * buffer_size + 2, osmium::Location{-171.22, 9.25}},
        osmium::NodeRef{3 * buffer_size + 3, osmium::Location{-171.24, 9.23}},
        osmium::NodeRef{3 * buffer_size + 4, osmium::Location{-171.25, 9.22}},
        osmium::NodeRef{3 * buffer_size + 5, osmium::Location{-171.26, 9.21}}
    };

    TSparseIndex sparse_index;
    for (const auto& r : refs) {
        sparse_index.set(r.ref(), r.location());
    }
    sparse_index.sort();
    sparse_index.dump_as_array(fd);

    const dense_file_array dense_index{fd};
    const auto max_id_in_refs = std::max_element(refs.begin(), refs.end())->ref();

    // Array index should be as large as necessary.
    REQUIRE(osmium::file_size(fd) >= max_id_in_refs * sizeof(osmium::Location));

    // check beyond largest ID
    REQUIRE_THROWS_AS(dense_index.get(max_id_in_refs + 1), osmium::not_found);

    // check if written values can be retrieved
    for (const auto& r : refs) {
        REQUIRE(dense_index.get(r.ref()) == r.location());
    }

    // check if all other values are invalid locations
    size_t invalid_count = 0;
    for (osmium::object_id_type id = 0; id <= max_id_in_refs; ++id) {
        if (!dense_index.get_noexcept(id).valid()) {
            ++invalid_count;
        }
    }
    REQUIRE(invalid_count == max_id_in_refs - refs.size() + 1);
}


#ifdef __linux__
using sparse_mmap_array = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;

TEST_CASE("Dump SparseMmapArray as array and load it as DenseFileArray") {
    test_index<sparse_mmap_array>();
}
#else
# pragma message("not running 'SparseMmapArray' test case on this machine")
#endif

using sparse_mem_array = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;

TEST_CASE("Dump SparseMemArray as array and load it as DenseFileArray") {
    test_index<sparse_mem_array>();
}

