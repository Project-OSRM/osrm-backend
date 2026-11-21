#include "extractor/node_location_index.hpp"

#include <boost/test/unit_test.hpp>

using osrm::extractor::NodeLocationIndex;

BOOST_AUTO_TEST_SUITE(node_location_index)

BOOST_AUTO_TEST_CASE(keeps_exact_locations_across_blocks_and_missing_ids)
{
    NodeLocationIndex index;
    for (int i = 0; i < 257; ++i)
        index.set(1000 + 2 * i, osmium::Location{-13000000 + i, 520000000 - i});

    const osmium::unsigned_object_id_type next_block = 65536;
    const osmium::Location last{-1800000000, -900000000};
    index.set(next_block, last);
    index.sort();

    BOOST_CHECK_EQUAL(index.size(), 258u);
    for (int i = 0; i < 257; ++i)
    {
        const osmium::Location expected{-13000000 + i, 520000000 - i};
        BOOST_CHECK(index.get(1000 + 2 * i) == expected);
    }
    BOOST_CHECK(index.get(next_block) == last);
    BOOST_CHECK(index.get_noexcept(1001).is_undefined());
    BOOST_CHECK(index.get_noexcept(2 * next_block).is_undefined());
    BOOST_CHECK_THROW(index.get(1001), osmium::not_found);

    index.clear();
    BOOST_CHECK_EQUAL(index.size(), 0u);
    BOOST_CHECK(index.get_noexcept(next_block).is_undefined());
}

BOOST_AUTO_TEST_CASE(keeps_an_uncompressed_block_when_encoding_would_expand_it)
{
    NodeLocationIndex raw;
    NodeLocationIndex packed;
    for (int i = 0; i < 128; ++i)
    {
        const auto sign = i % 2 == 0 ? 1 : -1;
        raw.set(i + 1, osmium::Location{sign * 1800000000, sign * 900000000});
        packed.set(i + 1, osmium::Location{13000000 + i, 520000000 - i});
    }
    raw.set(65536, osmium::Location{1, 1});
    packed.set(65536, osmium::Location{1, 1});

    BOOST_CHECK_GT(raw.used_memory(), packed.used_memory());
    for (int i = 0; i < 128; ++i)
    {
        const auto sign = i % 2 == 0 ? 1 : -1;
        const osmium::Location expected{sign * 1800000000, sign * 900000000};
        BOOST_CHECK(raw.get(i + 1) == expected);
    }
}

BOOST_AUTO_TEST_CASE(preserves_legacy_lookup_for_unsorted_and_duplicate_node_ids)
{
    using ID = osmium::unsigned_object_id_type;
    NodeLocationIndex index;
    osmium::index::map::FlexMem<ID, osmium::Location> legacy;
    const auto set = [&](ID id, osmium::Location location)
    {
        index.set(id, location);
        legacy.set(id, location);
    };

    for (int i = 0; i < 257; ++i)
        set(1000 + i, osmium::Location{-23000000 + i, -520000000 + i});
    set(65536, osmium::Location{63000000, 511000000});
    set(1001, osmium::Location{53000000, -522000000});
    set(1, osmium::Location{23, 42});
    index.sort();
    legacy.sort();

    BOOST_CHECK_EQUAL(index.size(), legacy.size());
    BOOST_CHECK_EQUAL(index.used_memory(), legacy.used_memory());
    for (ID id = 1000; id < 1257; ++id)
        BOOST_CHECK(index.get(id) == legacy.get(id));
    BOOST_CHECK(index.get(1) == legacy.get(1));
    BOOST_CHECK(index.get(65536) == legacy.get(65536));

    index.clear();
    index.set(1, osmium::Location{-23, 42});
    BOOST_CHECK_EQUAL(index.size(), 1u);
    BOOST_CHECK(index.get(1) == (osmium::Location{-23, 42}));
}

BOOST_AUTO_TEST_SUITE_END()
