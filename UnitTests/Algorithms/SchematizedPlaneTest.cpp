#include <boost/test/unit_test.hpp>
#include <boost/assert.hpp>

#include <iostream>

#include "../../DataStructures/SchematizedPlane.h"

BOOST_AUTO_TEST_SUITE(path_schematization)

static const unsigned TEST_DEGREE = 2;
static const double   TEST_MIN_LENGTH = 0.1;

unsigned preferedSchematizedAngleForRealAngle(const SchematizedPlane& plane, double angle)
{
    double dy = sin(angle);
    double dx = cos(angle);
    return plane.getPreferedDirection(SymbolicCoordinate(0, 0), SymbolicCoordinate(dx, dy));
}

BOOST_AUTO_TEST_CASE(next_angle_test)
{
    SchematizedPlane plane(TEST_DEGREE, TEST_MIN_LENGTH);

    BOOST_CHECK_EQUAL(plane.nextAngleAntiClockwise(0), 1);
    BOOST_CHECK_EQUAL(plane.nextAngleAntiClockwise(1), 2);
    BOOST_CHECK_EQUAL(plane.nextAngleAntiClockwise(7), 0);
    BOOST_CHECK_EQUAL(plane.nextAngleClockwise(0), 7);
    BOOST_CHECK_EQUAL(plane.nextAngleClockwise(1), 0);
    BOOST_CHECK_EQUAL(plane.nextAngleClockwise(2), 1);
}

BOOST_AUTO_TEST_CASE(direction_class_test)
{
    SchematizedPlane plane(TEST_DEGREE, TEST_MIN_LENGTH);
    const unsigned d = TEST_DEGREE;

    BOOST_CHECK(plane.isHorizontal(0));
    BOOST_CHECK(plane.isHorizontal(2*d));
    BOOST_CHECK(plane.isVertical(d));
    BOOST_CHECK(plane.isVertical(3*d));

    BOOST_ASSERT(d >= 2);

    BOOST_CHECK(plane.isReversed(0, 2*d));
    BOOST_CHECK(plane.isReversed(1, 1+2*d));
    BOOST_CHECK(plane.isReversed(d, 3*d));
    BOOST_CHECK(plane.isReversed(d+1, 3*d+1));
}

BOOST_AUTO_TEST_CASE(monotone_direction_transform)
{
    SchematizedPlane plane(TEST_DEGREE, TEST_MIN_LENGTH);

    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(1, MONOTONE_INCREASING_X), 1);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(2, MONOTONE_INCREASING_X), 2);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(3, MONOTONE_DECREASING_X), 1);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(5, MONOTONE_DECREASING_X), 7);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(1, MONOTONE_INCREASING_Y), 1);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(2, MONOTONE_INCREASING_Y), 0);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(2, MONOTONE_DECREASING_Y), 0);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(1, MONOTONE_DECREASING_Y), 7);
    BOOST_CHECK_EQUAL(plane.fromXMonotoneDirection(6, static_cast<PathMonotiticy>(MONOTONE_DECREASING_Y | MONOTONE_DECREASING_X)), 2);
}

BOOST_AUTO_TEST_CASE(prefered_direction)
{
    SchematizedPlane plane(TEST_DEGREE, TEST_MIN_LENGTH);

    SymbolicCoordinate a(0.0, 0.0);
    SymbolicCoordinate b(0.7, 0.7);
    SymbolicCoordinate c(-0.7, 0.7);
    SymbolicCoordinate d(-0.7, -0.7);
    SymbolicCoordinate e(0.7, -0.7);

    BOOST_CHECK_EQUAL(plane.getPreferedDirection(a, b), TEST_DEGREE/2);
    BOOST_CHECK_EQUAL(plane.getPreferedDirection(a, c), 3*TEST_DEGREE/2);
    BOOST_CHECK_EQUAL(plane.getPreferedDirection(a, d), 5*TEST_DEGREE/2);
    BOOST_CHECK_EQUAL(plane.getPreferedDirection(a, e), 7*TEST_DEGREE/2);

    double angle_inc = M_PI_2 / TEST_DEGREE;
    for (unsigned i = 0; i < 4*TEST_DEGREE; i++)
    {
        // devide each discrete angle in 4 sub angles
        double sub_inc = angle_inc / 3;
        BOOST_CHECK_EQUAL(preferedSchematizedAngleForRealAngle(plane, angle_inc*i + sub_inc), i);
        BOOST_CHECK_EQUAL(preferedSchematizedAngleForRealAngle(plane, angle_inc*i + 2*sub_inc), (i+1) % (4*TEST_DEGREE));
    }

}

BOOST_AUTO_TEST_CASE(min_length_extension)
{
    SchematizedPlane plane(TEST_DEGREE, TEST_MIN_LENGTH);

    for (unsigned angle_idx = 0; angle_idx < TEST_DEGREE*4; angle_idx++)
    {
        BOOST_CHECK_EQUAL(plane.extendToMinLenEdge(angle_idx, TEST_MIN_LENGTH), 0);

        double edge_dy = TEST_MIN_LENGTH/2.0;
        // edge in this direction has always negative dy
        if (angle_idx > 2*TEST_DEGREE)
        {
            edge_dy *= -1.0;
        }
        double new_dy = plane.extendToMinLenEdge(angle_idx, edge_dy);
        double new_dx = plane.getAngleXOffset(angle_idx, new_dy);
        double new_length = sqrt(new_dx * new_dx + new_dy * new_dy);

        // account for errors due to sin/tan
        BOOST_CHECK_LE(std::abs(new_length - TEST_MIN_LENGTH), std::numeric_limits<double>::epsilon()*10);
    }
}

BOOST_AUTO_TEST_SUITE_END()
