#include "extractor/compressed_edge_container.hpp"
#include "util/typedefs.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(compressed_edge_container)

using namespace osrm;
using namespace osrm::extractor;

BOOST_AUTO_TEST_CASE(long_road_test)
{
    //   0   1    2    3
    // 0---1----2----3----4
    CompressedEdgeContainer container;

    // compress 0---1---2 to 0---2
    container.CompressEdge(0, 1, 1, 2, 1, 1, 11, 11);
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(0), 1);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(0), 1);

    // compress 2---3---4 to 2---4
    container.CompressEdge(2, 3, 3, 4, 1, 1, 11, 11);
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(2), 3);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(2), 3);

    // compress 0---2---4 to 0---4
    container.CompressEdge(0, 2, 2, 4, 2, 2, 22, 22);
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(0), 1);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(0), 3);
}

BOOST_AUTO_TEST_CASE(t_crossing)
{
    //   0   1   2   3
    // 0---1---2---3---4
    //         | 4
    //         5
    //         | 5
    //         6
    CompressedEdgeContainer container;

    // compress 0---1---2 to 0---2
    container.CompressEdge(0, 1, 1, 2, 1, 1, 11, 11);
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK(!container.HasEntryForID(4));
    BOOST_CHECK(!container.HasEntryForID(5));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(0), 1);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(0), 1);

    // compress 2---5---6 to 2---6
    container.CompressEdge(4, 5, 5, 6, 1, 1, 11, 11);
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(!container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK(container.HasEntryForID(4));
    BOOST_CHECK(!container.HasEntryForID(5));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(4), 5);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(4), 5);

    // compress 2---3---4 to 2---4
    container.CompressEdge(2, 3, 3, 4, 1, 1, 11, 11);
    BOOST_CHECK(container.HasEntryForID(0));
    BOOST_CHECK(!container.HasEntryForID(1));
    BOOST_CHECK(container.HasEntryForID(2));
    BOOST_CHECK(!container.HasEntryForID(3));
    BOOST_CHECK(container.HasEntryForID(4));
    BOOST_CHECK(!container.HasEntryForID(5));
    BOOST_CHECK_EQUAL(container.GetFirstEdgeTargetID(2), 3);
    BOOST_CHECK_EQUAL(container.GetLastEdgeSourceID(2), 3);
}

BOOST_AUTO_TEST_SUITE_END()
