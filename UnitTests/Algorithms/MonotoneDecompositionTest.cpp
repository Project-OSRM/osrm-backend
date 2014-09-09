#include <boost/test/unit_test.hpp>
#include <boost/assert.hpp>
#include <boost/test/test_case_template.hpp>

#include <osrm/Coordinate.h>

#include <iostream>

#include "../../DataStructures/SymbolicCoordinate.h"
#include "../../DataStructures/SubPath.h"
#include "../../Algorithms/MonotoneDecomposition.h"
#include "TestUtils.h"

BOOST_AUTO_TEST_SUITE(monotone_decomposition)

void verify_monoticity(const SubPath& subpath, PathMonotiticy mono)
{
    if (mono & MONOTONE_INCREASING_X)
    {
        for (unsigned i = 0; i < subpath.nodes.size() - 1; i++)
        {
            BOOST_CHECK_LE(subpath.nodes[i].x, subpath.nodes[i+1].x);
        }
    }
    if (mono & MONOTONE_INCREASING_Y)
    {
        for (unsigned i = 0; i < subpath.nodes.size() - 1; i++)
        {
            BOOST_CHECK_LE(subpath.nodes[i].y, subpath.nodes[i+1].y);
        }
    }
    if (mono & MONOTONE_DECREASING_X)
    {
        for (unsigned i = 0; i < subpath.nodes.size() - 1; i++)
        {
            BOOST_CHECK_GE(subpath.nodes[i].x, subpath.nodes[i+1].x);
        }
    }
    if (mono & MONOTONE_DECREASING_Y)
    {
        for (unsigned i = 0; i < subpath.nodes.size() - 1; i++)
        {
            BOOST_CHECK_GE(subpath.nodes[i].y, subpath.nodes[i+1].y);
        }
    }
}

BOOST_AUTO_TEST_CASE(decomposition_test)
{
    TestRaster raster(0, 50, 0, 50, 10, 10);
    std::vector<SymbolicCoordinate> path = {
        /* x monotone increasing */
        raster.symbolic(0, 4),
        raster.symbolic(1, 3),
        raster.symbolic(2, 3),
        raster.symbolic(3, 4),
        /* y monotone increasing */
        raster.symbolic(2, 4),
        raster.symbolic(5, 4),
        raster.symbolic(4, 5),
        raster.symbolic(6, 6),
        /* y monotone decreasing */
        raster.symbolic(7, 5),
        raster.symbolic(6, 4),
        raster.symbolic(8, 3),
        raster.symbolic(8, 2),
        /* x monotone decreasing */
        raster.symbolic(7, 5),
        raster.symbolic(6, 4),
        raster.symbolic(6, 4),
        raster.symbolic(3, 6),
    };

    std::vector<SubPath> correct_subpaths = {
        SubPath({raster.symbolic(0, 4), raster.symbolic(1, 3), raster.symbolic(2, 3), raster.symbolic(3, 4)},
                 MONOTONE_INCREASING_X, 0, 3),
        SubPath({raster.symbolic(3, 4), raster.symbolic(2, 4), raster.symbolic(5, 4), raster.symbolic(4, 5), raster.symbolic(6, 6)},
                 MONOTONE_INCREASING_Y, 3, 7),
        SubPath({raster.symbolic(6, 6), raster.symbolic(7, 5), raster.symbolic(6, 4), raster.symbolic(8, 3), raster.symbolic(8, 2)},
                 MONOTONE_DECREASING_Y, 7, 11),
        SubPath({raster.symbolic(8, 2), raster.symbolic(7, 5), raster.symbolic(6, 4), raster.symbolic(6, 4), raster.symbolic(3, 6)},
                 MONOTONE_DECREASING_X, 11, 15),
    };

    std::vector<SubPath> subpaths;
    MonotoneDecomposition::axisMonotoneDecomposition(path, subpaths);

    BOOST_CHECK_EQUAL(correct_subpaths.size(), subpaths.size());

    for (unsigned i = 0; i < subpaths.size(); i++)
    {
        BOOST_CHECK_EQUAL(correct_subpaths[i].nodes.size(), subpaths[i].nodes.size());
        BOOST_TEST_MESSAGE("Subpath " << i << "(" << subpaths[i].monoticity  << ")");
        for(unsigned j = 0; j < subpaths[i].nodes.size(); j++)
        {
            BOOST_TEST_MESSAGE(subpaths[i].nodes[j]);
            BOOST_CHECK_EQUAL(correct_subpaths[i].nodes[j], subpaths[i].nodes[j]);
        }

        BOOST_CHECK_EQUAL(correct_subpaths[i].monoticity, subpaths[i].monoticity);
        BOOST_CHECK_EQUAL(correct_subpaths[i].input_start_idx, subpaths[i].input_start_idx);
        BOOST_CHECK_EQUAL(correct_subpaths[i].input_end_idx, subpaths[i].input_end_idx);

        verify_monoticity(subpaths[i], subpaths[i].monoticity);

        MonotoneDecomposition::transformToXMonotoneIncreasing(subpaths[i]);
        verify_monoticity(subpaths[i], MONOTONE_INCREASING_X);
        MonotoneDecomposition::transformFromXMonotoneIncreasing(subpaths[i]);
        verify_monoticity(subpaths[i], subpaths[i].monoticity);
    }
}

BOOST_AUTO_TEST_SUITE_END()
