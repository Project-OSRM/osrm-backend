#include <boost/test/unit_test.hpp>
#include <boost/assert.hpp>
#include <boost/test/test_case_template.hpp>

#include <osrm/Coordinate.h>

#include <iostream>

#include "../../DataStructures/SymbolicCoordinate.h"
#include "TestUtils.h"

BOOST_AUTO_TEST_SUITE(path_schematization)

BOOST_AUTO_TEST_CASE(symbolic_test)
{
    TestRaster raster(-50, 50, -50, 50, 100, 100);
    // note: this path must fill out the BB of the raster
    // otherwise we will compute different symbolic coordinates
    const std::vector<FixedPointCoordinate> fixed_path = {
        raster.real(0, 60),
        raster.real(10, 30),
        raster.real(25, 100),
        raster.real(100, 0)
    };

    std::vector<SymbolicCoordinate> symbolic_path;
    double scaling;
    FixedPointCoordinate origin;
    transformFixedPointToSymbolic(fixed_path, symbolic_path, scaling, origin,
        [](const FixedPointCoordinate& c1, FixedPointCoordinate& c2)
        {
            c2 = c1;
            return true;
        },
        [](const FixedPointCoordinate& c1)
        {
            return 0;
        }
    );

    for (unsigned i = 0; i < symbolic_path.size(); i++)
    {
        BOOST_CHECK_EQUAL(fixed_path[i], transformSymbolicToFixed(symbolic_path[i], scaling, origin));
    }

}

BOOST_AUTO_TEST_SUITE_END()
