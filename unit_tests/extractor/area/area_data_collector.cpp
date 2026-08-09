#include "extractor/area/area_data_collector.hpp"

#include "extractor/area/typedefs.hpp"
#include "extractor/area_routing_data.hpp"
#include "extractor/profile_properties.hpp"

#include <boost/test/unit_test.hpp>

#include <osmium/osm/location.hpp>
#include <osmium/osm/node_ref.hpp>

#include <thread>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_data_collector_test)

using namespace osrm;
using namespace osrm::extractor;
using namespace osrm::extractor::area;

namespace
{

osmium::NodeRef node(osmium::object_id_type id, double col, double row)
{ return osmium::NodeRef{id, osmium::Location{1.0 + col * 0.00045, 1.0 - row * 0.00045}}; }

/** A square whose outer ring carries the given ids. */
OsmiumPolygon square(const std::vector<osmium::object_id_type> &ring_ids)
{
    OsmiumPolygon poly;
    const double cols[] = {0, 4, 4, 0};
    const double rows[] = {4, 4, 0, 0};
    poly.outer().reserve(ring_ids.size());
    for (std::size_t i = 0; i < ring_ids.size(); ++i)
        poly.outer().push_back(node(ring_ids[i], cols[i % 4], rows[i % 4]));
    return poly;
}

std::vector<osmium::object_id_type> ring_ids(const PolygonRecord &record)
{
    std::vector<osmium::object_id_type> ids;
    ids.reserve(record.boundary_vertices.size());
    for (const auto &n : record.boundary_vertices)
        ids.push_back(n.ref());
    return ids;
}

} // namespace

BOOST_AUTO_TEST_CASE(area_routing_data_layout_is_stable)
{
    // The on-disk layout depends on this, so a change must be deliberate.
    BOOST_CHECK_EQUAL(sizeof(AreaPolygonSegment), 32u);

    AreaPolygonSegment segment;
    BOOST_CHECK_EQUAL(segment.u, SPECIAL_NODEID);
    BOOST_CHECK_EQUAL(segment.v, SPECIAL_NODEID);
    BOOST_CHECK_EQUAL(segment.num_vertices, 0u);
    BOOST_CHECK_EQUAL(segment.num_rings, 0u);
}

BOOST_AUTO_TEST_CASE(area_walking_speed_has_a_sane_default)
{
    ProfileProperties properties;
    // ~5 km/h, and in m/s -- the profiles' own `walking_speed` is in km/h
    BOOST_CHECK_CLOSE(properties.area_walking_speed, 1.4, 0.001);

    properties.area_walking_speed = 2.0;
    BOOST_CHECK_CLOSE(properties.area_walking_speed, 2.0, 0.001);
}

BOOST_AUTO_TEST_CASE(area_data_collector_records_ring_and_speed)
{
    AreaDataCollector collector;
    BOOST_CHECK_EQUAL(collector.size(), 0u);

    collector.record(square({1, 2, 3, 4}), 1.4);

    BOOST_REQUIRE_EQUAL(collector.size(), 1u);
    const auto &record = collector.polygons()[0];
    const std::vector<osmium::object_id_type> expected_ring{1, 2, 3, 4};
    BOOST_CHECK(ring_ids(record) == expected_ring);
    BOOST_CHECK_CLOSE(record.walking_speed, 1.4, 0.001);

    collector.clear();
    BOOST_CHECK_EQUAL(collector.size(), 0u);
}

// Areas are meshed from a parallel pipeline, so the arrival order is the scheduler's
// choice.  finalize() has to put them back into an order that depends only on the input,
// otherwise the extracted data differs between runs over the same file.
BOOST_AUTO_TEST_CASE(area_data_collector_is_deterministic_after_finalize)
{
    const std::vector<std::vector<osmium::object_id_type>> rings{
        {70, 71, 72, 73}, {10, 11, 12, 13}, {50, 51, 52, 53}, {30, 31, 32, 33}};

    const auto collect_in = [&rings](const std::vector<std::size_t> &order)
    {
        AreaDataCollector collector;
        for (const auto i : order)
            collector.record(square(rings[i]), 1.4);
        collector.finalize();
        std::vector<std::vector<osmium::object_id_type>> result;
        for (const auto &record : collector.polygons())
            result.push_back(ring_ids(record));
        return result;
    };

    const auto forwards = collect_in({0, 1, 2, 3});
    const auto backwards = collect_in({3, 2, 1, 0});
    const auto shuffled = collect_in({2, 0, 3, 1});

    BOOST_CHECK(forwards == backwards);
    BOOST_CHECK(forwards == shuffled);
    // and the order is the one the ids imply, not the one they arrived in
    BOOST_REQUIRE_EQUAL(forwards.size(), 4u);
    BOOST_CHECK_EQUAL(forwards[0][0], 10);
    BOOST_CHECK_EQUAL(forwards[3][0], 70);
}

// record() is called concurrently from the meshing pipeline.
BOOST_AUTO_TEST_CASE(area_data_collector_record_is_thread_safe)
{
    AreaDataCollector collector;
    constexpr int per_thread = 50;
    constexpr int threads = 4;

    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (int t = 0; t < threads; ++t)
    {
        workers.emplace_back(
            [&collector, t]
            {
                for (int i = 0; i < per_thread; ++i)
                {
                    const osmium::object_id_type base = t * 1000 + i * 4 + 1;
                    collector.record(square({base, base + 1, base + 2, base + 3}), 1.4);
                }
            });
    }
    for (auto &worker : workers)
        worker.join();

    BOOST_CHECK_EQUAL(collector.size(), threads * per_thread);

    collector.finalize();
    // every record survived intact, and they are strictly ordered
    for (std::size_t i = 1; i < collector.polygons().size(); ++i)
    {
        BOOST_CHECK_EQUAL(collector.polygons()[i].boundary_vertices.size(), 4u);
        BOOST_CHECK(ring_ids(collector.polygons()[i - 1]) < ring_ids(collector.polygons()[i]));
    }
}

// Snapping adds a virtual edge to every visibility-graph vertex the coordinate can see,
// and the obstacle corners are among them, so the rings have to survive the recording.
BOOST_AUTO_TEST_CASE(area_data_collector_records_the_obstacle_rings)
{
    OsmiumPolygon poly;
    poly.outer().push_back(node(1, 0, 8));
    poly.outer().push_back(node(2, 8, 8));
    poly.outer().push_back(node(3, 8, 0));
    poly.outer().push_back(node(4, 0, 0));
    OsmiumPolygon::ring_type obstacle;
    obstacle.push_back(node(5, 3, 5));
    obstacle.push_back(node(6, 3, 3));
    obstacle.push_back(node(7, 5, 3));
    obstacle.push_back(node(8, 5, 5));
    poly.inners().push_back(obstacle);

    AreaDataCollector collector;
    collector.record(poly, 1.4);

    BOOST_REQUIRE_EQUAL(collector.size(), 1u);
    const auto &record = collector.polygons()[0];
    BOOST_CHECK_EQUAL(record.boundary_vertices.size(), 4u);
    BOOST_REQUIRE_EQUAL(record.obstacle_rings.size(), 1u);
    BOOST_REQUIRE_EQUAL(record.obstacle_rings[0].size(), 4u);

    std::vector<osmium::object_id_type> corners;
    for (const auto &n : record.obstacle_rings[0])
        corners.push_back(n.ref());
    const std::vector<osmium::object_id_type> expected{5, 6, 7, 8};
    BOOST_CHECK(corners == expected);
}

BOOST_AUTO_TEST_SUITE_END()
