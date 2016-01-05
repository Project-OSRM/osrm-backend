#include "engine/douglas_peucker.hpp"
#include "engine/segment_information.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <osrm/coordinate.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(douglas_peucker)

using namespace osrm;
using namespace osrm::engine;

SegmentInformation getTestInfo(int lat, int lon, bool necessary)
{
    return SegmentInformation(util::FixedPointCoordinate(lat, lon), 0, 0, 0, extractor::TurnInstruction::HeadOn,
                              necessary, false, 0);
}

BOOST_AUTO_TEST_CASE(all_necessary_test)
{
    /*
     *     x
     *    / \
     *   x   \
     *  /     \
     * x       x
     */
    std::vector<SegmentInformation> info = {getTestInfo(5, 5, true), getTestInfo(6, 6, true),
                                            getTestInfo(10, 10, true), getTestInfo(5, 15, true)};
    DouglasPeucker dp;
    for (unsigned z = 0; z < DOUGLAS_PEUCKER_THRESHOLDS.size(); z++)
    {
        dp.Run(info, z);
        for (const auto &i : info)
        {
            BOOST_CHECK_EQUAL(i.necessary, true);
        }
    }
}

BOOST_AUTO_TEST_CASE(remove_second_node_test)
{
    DouglasPeucker dp;
    for (unsigned z = 0; z < DOUGLAS_PEUCKER_THRESHOLDS.size(); z++)
    {
        /*
         *   x--x
         *   |   \
         * x-x    x
         *        |
         *        x
         */
        std::vector<SegmentInformation> info = {
            getTestInfo(5 * COORDINATE_PRECISION, 5 * COORDINATE_PRECISION, true),
            getTestInfo(5 * COORDINATE_PRECISION,
                        5 * COORDINATE_PRECISION + DOUGLAS_PEUCKER_THRESHOLDS[z], false),
            getTestInfo(10 * COORDINATE_PRECISION, 10 * COORDINATE_PRECISION, false),
            getTestInfo(10 * COORDINATE_PRECISION,
                        10 + COORDINATE_PRECISION + DOUGLAS_PEUCKER_THRESHOLDS[z] * 2, false),
            getTestInfo(5 * COORDINATE_PRECISION, 15 * COORDINATE_PRECISION, false),
            getTestInfo(5 * COORDINATE_PRECISION + DOUGLAS_PEUCKER_THRESHOLDS[z],
                        15 * COORDINATE_PRECISION, true),
        };
        BOOST_TEST_MESSAGE("Threshold (" << z << "): " << DOUGLAS_PEUCKER_THRESHOLDS[z]);
        dp.Run(info, z);
        BOOST_CHECK_EQUAL(info[0].necessary, true);
        BOOST_CHECK_EQUAL(info[1].necessary, false);
        BOOST_CHECK_EQUAL(info[2].necessary, true);
        BOOST_CHECK_EQUAL(info[3].necessary, true);
        BOOST_CHECK_EQUAL(info[4].necessary, false);
        BOOST_CHECK_EQUAL(info[5].necessary, true);
    }
}

BOOST_AUTO_TEST_SUITE_END()
