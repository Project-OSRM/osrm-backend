
#include "catch.hpp"

#include <osmium/osm/types.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/index/detail/tmpfile.hpp>
#include <osmium/util/file.hpp>

#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/index/map/sparse_file_array.hpp>

#include <osmium/index/node_locations_map.hpp>

TEST_CASE("File based index") {

    int fd = osmium::detail::create_tmp_file();

    REQUIRE(osmium::util::file_size(fd) == 0);

    const osmium::unsigned_object_id_type id1 = 6;
    const osmium::unsigned_object_id_type id2 = 3;
    const osmium::Location loc1(1.2, 4.5);
    const osmium::Location loc2(3.5, -7.2);

    SECTION("dense index") {
        using index_type = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;
        constexpr const size_t S = sizeof(index_type::element_type);

        {
            index_type index(fd);

            REQUIRE(index.size() == 0);

            REQUIRE_THROWS_AS(index.get(  0), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  1), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  3), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  5), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  6), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  7), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(100), osmium::not_found);

            index.set(id1, loc1);
            REQUIRE(index.size() == 7);

            index.set(id2, loc2);
            REQUIRE(index.size() == 7);

            index.sort();

            REQUIRE(loc1 == index.get(id1));
            REQUIRE(loc2 == index.get(id2));

            REQUIRE_THROWS_AS(index.get(  0), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  1), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  5), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  7), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(100), osmium::not_found);

            REQUIRE(index.size() == 7);
            REQUIRE(std::distance(index.cbegin(), index.cend()) == 7);

            REQUIRE(osmium::util::file_size(fd) >= (6 * S));
        }

        {
            index_type index(fd);
            REQUIRE(osmium::util::file_size(fd) >= (6 * S));

            REQUIRE(index.size() == 7);

            REQUIRE(loc1 == index.get(id1));
            REQUIRE(loc2 == index.get(id2));

            REQUIRE_THROWS_AS(index.get(  0), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  1), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  5), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  7), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(100), osmium::not_found);

            REQUIRE(index.size() == 7);
            REQUIRE(std::distance(index.cbegin(), index.cend()) == 7);

            auto it = index.cbegin();
            REQUIRE(*it++ == osmium::Location{});
            REQUIRE(*it++ == osmium::Location{});
            REQUIRE(*it++ == osmium::Location{});
            REQUIRE(*it++ == loc2);
            REQUIRE(*it++ == osmium::Location{});
            REQUIRE(*it++ == osmium::Location{});
            REQUIRE(*it++ == loc1);
            REQUIRE(it++ == index.cend());
        }
    }

    SECTION("sparse index") {
        using index_type = osmium::index::map::SparseFileArray<osmium::unsigned_object_id_type, osmium::Location>;
        constexpr const size_t S = sizeof(index_type::element_type);

        {
            index_type index(fd);

            REQUIRE(index.size() == 0);

            REQUIRE_THROWS_AS(index.get(  0), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  1), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  3), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  5), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  6), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  7), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(100), osmium::not_found);

            index.set(id1, loc1);
            REQUIRE(index.size() == 1);

            index.set(id2, loc2);
            REQUIRE(index.size() == 2);

            index.sort();

            REQUIRE(loc1 == index.get(id1));
            REQUIRE(loc2 == index.get(id2));

            REQUIRE_THROWS_AS(index.get(  0), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  1), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  5), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  7), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(100), osmium::not_found);

            REQUIRE(index.size() == 2);
            REQUIRE(std::distance(index.cbegin(), index.cend()) == 2);

            REQUIRE(osmium::util::file_size(fd) >= (2 * S));
        }

        {
            index_type index(fd);
            REQUIRE(osmium::util::file_size(fd) >= (2 * S));

            REQUIRE(index.size() == 2);

            REQUIRE(loc1 == index.get(id1));
            REQUIRE(loc2 == index.get(id2));

            REQUIRE_THROWS_AS(index.get(  0), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  1), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  5), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(  7), osmium::not_found);
            REQUIRE_THROWS_AS(index.get(100), osmium::not_found);

            REQUIRE(index.size() == 2);
            REQUIRE(std::distance(index.cbegin(), index.cend()) == 2);
        }
    }
}

