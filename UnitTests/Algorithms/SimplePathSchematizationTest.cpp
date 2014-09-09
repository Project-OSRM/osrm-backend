#include <boost/test/unit_test.hpp>
#include <boost/assert.hpp>
#include <boost/test/test_case_template.hpp>

#include <osrm/Coordinate.h>

#include <iostream>

#include "../../Algorithms/SimplePathSchematization.h"

BOOST_AUTO_TEST_SUITE(path_schematization)

static const unsigned TEST_DEGREE = 2;
static const double TEST_MIN_LENGTH = 0.1;

FixedPointCoordinate FPC(double lat, double lon)
{
    return FixedPointCoordinate(lat * COORDINATE_PRECISION, lon * COORDINATE_PRECISION);
}

BOOST_AUTO_TEST_CASE(integrated_schematization)
{
    SimplePathSchematization sps(2, 0.1);

    /*
     *                        (6) (8)-----(9)
     *                         | .  .
     *                         |   . .
     *          (3)----(4)----(5)    `(7)
     *          /
     *  (1)----(2)
     *   |
     *  (0)
     *
     *  The path will be split at (7) and the edge (6) -> (7)
     *  will be altered in post-processing because it will overlap
     *  with the edge (7)->(8)
     */
    std::vector<SegmentInformation> segment_info = {
        SegmentInformation(FPC(0.0, 0.0), 1, 0.0, 0.0, TurnInstruction::NoTurn, true, true, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.1, 0.0), 1, 0.0, 0.0, TurnInstruction::TurnRight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.1, 0.1), 1, 0.0, 0.0, TurnInstruction::TurnLeft, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.2, 0.2), 2, 0.0, 0.0, TurnInstruction::GoStraight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.2, 0.3), 2, 0.0, 0.0, TurnInstruction::GoStraight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.2, 0.4), 3, 0.0, 0.0, TurnInstruction::TurnLeft, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.3, 0.4), 4, 0.0, 0.0, TurnInstruction::TurnSharpRight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.2, 0.6), 5, 0.0, 0.0, TurnInstruction::TurnSharpLeft, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.3, 0.5), 6, 0.0, 0.0, TurnInstruction::TurnRight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.3, 0.7), 7, 0.0, 0.0, TurnInstruction::NoTurn, true, true, TRAVEL_MODE_DEFAULT),
    };

    std::vector<SegmentInformation> correct_info = {
        SegmentInformation(FPC(0,0), 1, 0.0, 0.0, TurnInstruction::NoTurn, true, true, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.069995,0), 1, 0.0, 0.0, TurnInstruction::TurnRight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.069995,0.069999), 1, 0.0, 0.0, TurnInstruction::TurnLeft, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.119494,0.119497), 2, 0.0, 0.0, TurnInstruction::GoStraight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.119494,0.189497), 3, 0.0, 0.0, TurnInstruction::TurnLeft, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.189496,0.189497), 4, 0.0, 0.0, TurnInstruction::TurnSharpRight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.189496,0.288492), 5, 0.0, 0.0, TurnInstruction::TurnSharpLeft, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.259492,0.218492), 6, 0.0, 0.0, TurnInstruction::TurnRight, true, false, TRAVEL_MODE_DEFAULT),
        SegmentInformation(FPC(0.259492,0.288492), 7, 0.0, 0.0, TurnInstruction::NoTurn, true, true, TRAVEL_MODE_DEFAULT),
    };

    std::vector<SegmentInformation> schematized_info;

    sps.schematize(segment_info, schematized_info);

    BOOST_CHECK_EQUAL(correct_info.size(), schematized_info.size());

    for (unsigned i = 0; i < correct_info.size(); i++)
    {
        BOOST_CHECK_EQUAL(correct_info[i].name_id, schematized_info[i].name_id);
        BOOST_CHECK_LE(std::fabs(correct_info[i].location.lat/COORDINATE_PRECISION
                               - schematized_info[i].location.lat/COORDINATE_PRECISION),
                       std::numeric_limits<float>::epsilon()*10);
        BOOST_CHECK_LE(std::fabs(correct_info[i].location.lon/COORDINATE_PRECISION
                               - schematized_info[i].location.lon/COORDINATE_PRECISION),
                       std::numeric_limits<float>::epsilon()*10);
    }
}

BOOST_AUTO_TEST_SUITE_END()
