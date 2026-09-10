#include "extractor/driving_side_index.hpp"

#include <boost/test/unit_test.hpp>

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/way.hpp>

#include <tbb/parallel_for.h>

#include <atomic>
#include <vector>

BOOST_AUTO_TEST_SUITE(driving_side)

using namespace osrm::extractor;

namespace
{
osmium::Box box(double min_lat, double min_lon, double max_lat, double max_lon)
{ return {osmium::Location{min_lon, min_lat}, osmium::Location{max_lon, max_lat}}; }

// The Hong Kong boundary runs along the Shenzhen River, left-hand traffic to
// the south. The polygon edge crosses lon 114.02 at lat 22.5066.
constexpr double HK_BOUNDARY_LAT = 22.5066;
constexpr double HK_BOUNDARY_LON = 114.02;

const osmium::Way &makeWay(osmium::memory::Buffer &buffer,
                           osmium::object_id_type id,
                           const std::vector<osmium::Location> &locations)
{
    using namespace osmium::builder::attr;
    std::vector<osmium::NodeRef> nodes;
    nodes.reserve(locations.size());
    osmium::object_id_type node_id = id * 100;
    for (const auto &location : locations)
        nodes.emplace_back(node_id++, location);

    const auto pos = osmium::builder::add_way(buffer, _id(id), _nodes(nodes));
    return buffer.get<osmium::Way>(pos);
}
} // namespace

BOOST_AUTO_TEST_CASE(off_answers_nothing)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Off};
    BOOST_CHECK(!index.Enabled());
    BOOST_CHECK(!index.SettleExtent(box(51.50, -0.13, 51.51, -0.12)));
    BOOST_CHECK(!index.NeedsWayLookups());
}

BOOST_AUTO_TEST_CASE(a_left_hand_extract_is_settled_by_its_extent)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Auto};
    const auto side = index.SettleExtent(box(51.50, -0.13, 51.51, -0.12)); // London
    BOOST_REQUIRE(side.has_value());
    BOOST_CHECK_EQUAL(*side, true);
    BOOST_CHECK(!index.NeedsWayLookups());
}

BOOST_AUTO_TEST_CASE(a_right_hand_extract_is_settled_by_its_extent)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Auto};
    const auto side = index.SettleExtent(box(48.85, 2.34, 48.86, 2.36)); // Paris
    BOOST_REQUIRE(side.has_value());
    BOOST_CHECK_EQUAL(*side, false);
    BOOST_CHECK(!index.NeedsWayLookups());
}

BOOST_AUTO_TEST_CASE(tokyo_drives_on_the_left)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Auto};
    const auto side = index.SettleExtent(box(35.67, 139.64, 35.68, 139.66));
    BOOST_REQUIRE(side.has_value());
    BOOST_CHECK_EQUAL(*side, true);
}

BOOST_AUTO_TEST_CASE(an_extract_spanning_a_boundary_needs_way_lookups)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Auto};
    // A box around the Shenzhen River, not one over the Channel: the eight
    // vertex hull for Britain and Ireland covers both banks there, so it would
    // answer instead of straddling.
    BOOST_CHECK(!index.SettleExtent(box(22.497, 114.01, 22.516, 114.03)));
    BOOST_CHECK(index.NeedsWayLookups());
}

BOOST_AUTO_TEST_CASE(an_extract_without_an_extent_needs_way_lookups)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Auto};
    BOOST_CHECK(!index.SettleExtent(osmium::Box{}));
    BOOST_CHECK(index.NeedsWayLookups());
}

BOOST_AUTO_TEST_CASE(always_skips_the_extent_entirely)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Always};
    BOOST_CHECK(!index.SettleExtent(box(51.50, -0.13, 51.51, -0.12)));
    BOOST_CHECK(index.NeedsWayLookups());
}

BOOST_AUTO_TEST_CASE(ways_are_classified_on_either_side_of_a_boundary)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Always};
    index.SettleExtent(osmium::Box{});

    osmium::memory::Buffer buffer{1024, osmium::memory::Buffer::auto_grow::yes};

    const auto &north =
        makeWay(buffer,
                1,
                {osmium::Location{HK_BOUNDARY_LON - 0.002, HK_BOUNDARY_LAT + 0.005},
                 osmium::Location{HK_BOUNDARY_LON + 0.002, HK_BOUNDARY_LAT + 0.005}});
    const auto north_side = index.IsLeftHandTraffic(north);
    BOOST_REQUIRE(north_side.has_value());
    BOOST_CHECK_EQUAL(*north_side, false);

    const auto &south =
        makeWay(buffer,
                2,
                {osmium::Location{HK_BOUNDARY_LON - 0.002, HK_BOUNDARY_LAT - 0.005},
                 osmium::Location{HK_BOUNDARY_LON + 0.002, HK_BOUNDARY_LAT - 0.005}});
    const auto south_side = index.IsLeftHandTraffic(south);
    BOOST_REQUIRE(south_side.has_value());
    BOOST_CHECK_EQUAL(*south_side, true);
}

BOOST_AUTO_TEST_CASE(a_straddling_way_is_settled_on_its_last_node)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Always};
    index.SettleExtent(osmium::Box{});

    osmium::memory::Buffer buffer{1024, osmium::memory::Buffer::auto_grow::yes};

    // North to south, so the last node is in Hong Kong.
    const auto &southbound = makeWay(buffer,
                                     1,
                                     {osmium::Location{HK_BOUNDARY_LON, HK_BOUNDARY_LAT + 0.005},
                                      osmium::Location{HK_BOUNDARY_LON, HK_BOUNDARY_LAT - 0.005}});
    const auto southbound_side = index.IsLeftHandTraffic(southbound);
    BOOST_REQUIRE(southbound_side.has_value());
    BOOST_CHECK_EQUAL(*southbound_side, true);

    // The same geometry the other way round ends on the mainland.
    const auto &northbound = makeWay(buffer,
                                     2,
                                     {osmium::Location{HK_BOUNDARY_LON, HK_BOUNDARY_LAT - 0.005},
                                      osmium::Location{HK_BOUNDARY_LON, HK_BOUNDARY_LAT + 0.005}});
    const auto northbound_side = index.IsLeftHandTraffic(northbound);
    BOOST_REQUIRE(northbound_side.has_value());
    BOOST_CHECK_EQUAL(*northbound_side, false);
}

BOOST_AUTO_TEST_CASE(a_way_without_locations_cannot_be_classified)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Always};
    index.SettleExtent(osmium::Box{});

    osmium::memory::Buffer buffer{1024, osmium::memory::Buffer::auto_grow::yes};
    const auto &way = makeWay(buffer, 1, {osmium::Location{}, osmium::Location{}});
    BOOST_CHECK(!index.IsLeftHandTraffic(way));
}

// The index is a wasm instance per thread, and every query rewinds a scratch
// allocator inside it. Hammering it from every thread at once is what would
// show up either mistake: a shared instance, or scratch that keeps growing.
BOOST_AUTO_TEST_CASE(classification_is_thread_safe_and_does_not_grow)
{
    DrivingSideIndex index{DrivingSideIndex::Mode::Always};
    index.SettleExtent(osmium::Box{});

    osmium::memory::Buffer buffer{1024 * 64, osmium::memory::Buffer::auto_grow::yes};
    const auto &london = makeWay(
        buffer, 1, {osmium::Location{-0.1290, 51.5072}, osmium::Location{-0.1262, 51.5072}});
    const auto &paris =
        makeWay(buffer, 2, {osmium::Location{2.3500, 48.8566}, osmium::Location{2.3544, 48.8566}});

    std::atomic<std::size_t> left{0};
    std::atomic<std::size_t> right{0};
    std::atomic<std::size_t> unresolved{0};
    constexpr std::size_t iterations = 20000;

    // Counting rather than asserting inside the loop: Boost.Test's assertions
    // are not safe to call from several threads at once.
    tbb::parallel_for(std::size_t{0},
                      iterations,
                      [&](const std::size_t i)
                      {
                          const auto &way = (i % 2 == 0) ? london : paris;
                          const auto side = index.IsLeftHandTraffic(way);
                          if (!side)
                              ++unresolved;
                          else if (*side)
                              ++left;
                          else
                              ++right;
                      });

    BOOST_CHECK_EQUAL(unresolved.load(), 0u);
    BOOST_CHECK_EQUAL(left.load(), iterations / 2);
    BOOST_CHECK_EQUAL(right.load(), iterations / 2);
}

BOOST_AUTO_TEST_SUITE_END()
